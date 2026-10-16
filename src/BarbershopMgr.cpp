#include "BarbershopMgr.h"
#include "BarbershopConfig.h"

#include "Database/DatabaseEnv.h"
#include "GameObject.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "UpdateData.h"
#include "UpdateMask.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <sstream>

BarbershopMgr& BarbershopMgr::Instance()
{
    static BarbershopMgr instance;
    return instance;
}

namespace
{
    std::vector<std::string> SplitFields(std::string const& text)
    {
        std::vector<std::string> fields;
        size_t from = 0;
        for (;;)
        {
            size_t at = text.find(';', from);
            fields.push_back(text.substr(from, at == std::string::npos ? std::string::npos : at - from));
            if (at == std::string::npos)
                return fields;
            from = at + 1;
        }
    }

    bool IsInChairSitState(Player const* player)
    {
        if (!player)
            return false;
        uint8 state = player->GetStandState();
        return state >= UNIT_STAND_STATE_SIT_LOW_CHAIR && state <= UNIT_STAND_STATE_SIT_HIGH_CHAIR;
    }
}

bool BarbershopMgr::HasActiveSession(Player const* player) const
{
    if (!player || !m_count.load())
        return false;

    std::lock_guard<std::mutex> guard(m_lock);
    return m_sessions.find(player->GetGUIDLow()) != m_sessions.end();
}

Barbershop::Session* BarbershopMgr::GetSession(Player const* player)
{
    if (!player)
        return nullptr;
    auto itr = m_sessions.find(player->GetGUIDLow());
    return itr == m_sessions.end() ? nullptr : &itr->second;
}

void BarbershopMgr::SendValuesToSelf(Player* player, std::vector<std::pair<uint16, uint32>> const& fields) const
{
    if (!player || !player->GetSession() || fields.empty())
        return;

    // Fields must be in strictly ascending index order for the client UpdateMask
    std::vector<std::pair<uint16, uint32>> sortedFields = fields;
    std::sort(sortedFields.begin(), sortedFields.end(),
        [](std::pair<uint16, uint32> const& a, std::pair<uint16, uint32> const& b) {
            return a.first < b.first;
        });

    UpdateMask mask;
    mask.SetCount(player->GetValuesCount());
    for (auto const& field : sortedFields)
        mask.SetBit(field.first);

    UpdateData data;
    ByteBuffer& buf = data.AddUpdateBlockAndGetBuffer();
    buf << uint8(UPDATETYPE_VALUES);
    buf << player->GetPackGUID();
    buf << uint8(mask.GetBlockCount());
    buf.append(mask.GetMask(), mask.GetLength());
    for (auto const& field : sortedFields)
        buf << field.second;

    WorldPacket packet;
    data.BuildPacket(&packet);
    player->GetSession()->SendPacket(&packet);
}

Barbershop::Look BarbershopMgr::ReadLook(Player const* player)
{
    Barbershop::Look look;
    for (uint32 i = 0; i < Barbershop::CUSTOMIZATION_COUNT; ++i)
        look.values[i] = player->GetByteValue(Barbershop::CustomizationBytes[i][0], Barbershop::CustomizationBytes[i][1]);

    return look;
}

void BarbershopMgr::ApplyLook(Player* player, Barbershop::Look const& look)
{
    if (!player)
        return;

    bool changed = false;
    for (uint32 i = 0; i < Barbershop::CUSTOMIZATION_COUNT; ++i)
    {
        uint32 field = Barbershop::CustomizationBytes[i][0];
        uint32 byteOffset = Barbershop::CustomizationBytes[i][1];
        if (player->GetByteValue(field, byteOffset) != look.values[i])
        {
            player->SetByteValue(field, byteOffset, look.values[i]);
            changed = true;
        }
    }

    if (!changed)
        return;

    // Direct database write ensures immediate persistence across abrupt restarts
    CharacterDatabase.PExecute(
        "UPDATE `characters` SET `playerBytes` = %u, `playerBytes2` = %u WHERE `guid` = %u",
        player->GetUInt32Value(PLAYER_BYTES), player->GetUInt32Value(PLAYER_BYTES_2), player->GetGUIDLow());

    // Flash display ID briefly to force client model rebuild for everyone in view
    uint32 display = player->GetDisplayId();
    player->SetDisplayId(Barbershop::DisplayIdBox);
    player->DirectSendPublicValueUpdate(UNIT_FIELD_DISPLAYID);
    player->SetDisplayId(display);
}

void BarbershopMgr::ShowLook(Player* player, Barbershop::Look const& shown, Barbershop::Look const& look)
{
    if (!player)
        return;

    if (std::memcmp(look.values, shown.values, sizeof(look.values)) != 0)
    {
        uint32 fields[2] = { player->GetUInt32Value(PLAYER_BYTES), player->GetUInt32Value(PLAYER_BYTES_2) };
        for (uint32 i = 0; i < Barbershop::CUSTOMIZATION_COUNT; ++i)
        {
            uint32& field = fields[Barbershop::CustomizationBytes[i][0] == PLAYER_BYTES ? 0 : 1];
            uint32 shift = Barbershop::CustomizationBytes[i][1] * 8;
            field = (field & ~(0xFFu << shift)) | (uint32(look.values[i]) << shift);
        }

        // The same flash as ApplyLook, in the same order, sent only to this player's session
        SendValuesToSelf(player, { { UNIT_FIELD_DISPLAYID, Barbershop::DisplayIdBox } });
        SendValuesToSelf(player, {
            { UNIT_FIELD_DISPLAYID, player->GetDisplayId() },
            { PLAYER_BYTES, fields[0] },
            { PLAYER_BYTES_2, fields[1] }
        });
    }
}

uint8 BarbershopMgr::GetLimit(Player const* player, uint32 customization) const
{
    if (!player || customization >= Barbershop::CUSTOMIZATION_COUNT)
        return 0;

    uint32 race = player->GetRace();
    uint32 gender = player->GetGender();
    if (race >= 11 || gender >= 2)
        return 0;

    return Barbershop::RaceLimits[customization][race][gender];
}

bool BarbershopMgr::IsAllowed(Player const* player, Barbershop::Look const& original, uint32 customization, uint8 value) const
{
    if (!player || customization >= Barbershop::CUSTOMIZATION_COUNT)
        return false;

    // What the character already has is always allowed
    if (value == original.values[customization])
        return true;

    return value < GetLimit(player, customization);
}

uint32 BarbershopMgr::GetCost(Player const* player, Barbershop::Look const& from, Barbershop::Look const& to) const
{
    if (!player)
        return 0;

    uint64 level = player->GetLevel();
    uint64 base = level * level * sBarbershopConfig.GetCostBase();

    uint64 percent = 0;
    if (to.values[Barbershop::CUSTOMIZATION_STYLE] != from.values[Barbershop::CUSTOMIZATION_STYLE])
        percent += sBarbershopConfig.GetCostStyle();
    else if (to.values[Barbershop::CUSTOMIZATION_COLOR] != from.values[Barbershop::CUSTOMIZATION_COLOR])
        percent += sBarbershopConfig.GetCostColor();

    if (to.values[Barbershop::CUSTOMIZATION_FACIAL] != from.values[Barbershop::CUSTOMIZATION_FACIAL])
        percent += sBarbershopConfig.GetCostFacial();

    return static_cast<uint32>(std::min<uint64>(base * percent / 100, MAX_MONEY_AMOUNT));
}

void BarbershopMgr::Send(Player* player, std::string const& message) const
{
    if (player)
        player->SendAddonMessage(Barbershop::Prefix, message);
}

void BarbershopMgr::SendPreview(Player* player, Barbershop::Session const& session) const
{
    std::string message = "S2C_UPDATE_PREVIEW;" + std::to_string(GetCost(player, session.original, session.preview));
    for (uint32 i = 0; i < Barbershop::CUSTOMIZATION_COUNT; ++i)
        message += ";" + std::to_string(session.preview.values[i]);
    Send(player, message);
}

void BarbershopMgr::Open(Player* player, GameObject* chair)
{
    if (!sBarbershopConfig.IsEnabled())
        return;

    if (!player || !chair || !player->IsAlive() || player->IsInCombat())
        return;

    if (player->GetDisplayId() != player->GetNativeDisplayId())
    {
        if (player->GetSession())
            player->GetSession()->SendNotification("Return to your normal form first.");
        return;
    }

    std::lock_guard<std::mutex> guard(m_lock);

    if (m_sessions.count(player->GetGUIDLow()))
        return;

    Barbershop::Session session;
    session.chair = chair->GetObjectGuid();
    session.original = ReadLook(player);
    session.preview = session.original;
    m_sessions[player->GetGUIDLow()] = session;
    ++m_count;

    uint8 const* v = session.original.values;
    // Protocol strictly conforms to Turtle_Barbershop.lua in Turtle WoW client patch-4/patch-9:
    // S2C_OPEN_BARBER;cost;style;;color;;facial
    std::string openMsg = "S2C_OPEN_BARBER;0;" +
        std::to_string(v[Barbershop::CUSTOMIZATION_STYLE]) + ";;" +
        std::to_string(v[Barbershop::CUSTOMIZATION_COLOR]) + ";;" +
        std::to_string(v[Barbershop::CUSTOMIZATION_FACIAL]);

    Send(player, openMsg);
}

void BarbershopMgr::Close(Player* player, Barbershop::Session const& session, bool notify)
{
    if (notify && session.preview != session.original)
        ShowLook(player, session.preview, session.original);

    if (m_sessions.erase(player->GetGUIDLow()))
        --m_count;

    if (notify)
        Send(player, "S2C_CLOSE_BARBER");
}

bool BarbershopMgr::HandleAddonMessage(Player* player, std::string const& msg)
{
    if (!player)
        return false;

    std::vector<std::string> fields = SplitFields(msg);
    if (fields.empty())
        return false;

    std::string const& command = fields[0];

    std::lock_guard<std::mutex> guard(m_lock);

    Barbershop::Session* session = GetSession(player);
    if (!session)
    {
        if (command != "C2S_CLOSE")
            Send(player, "S2C_ERROR;" + std::to_string(Barbershop::BARBERSHOP_ERROR_NO_SESSION));
        return true;
    }

    for (uint32 customization = 0; customization < Barbershop::CUSTOMIZATION_COUNT; ++customization)
    {
        if (command != Barbershop::PreviewCommands[customization])
            continue;

        if (fields.size() < 2)
            return true;

        int value = std::atoi(fields[1].c_str());
        if (value < 0 || value > 255 || !IsAllowed(player, session->original, customization, static_cast<uint8>(value)))
        {
            Send(player, "S2C_ERROR;" + std::to_string(Barbershop::BARBERSHOP_ERROR_NOT_ALLOWED));
            SendPreview(player, *session);
            return true;
        }

        Barbershop::Look look = session->preview;
        look.values[customization] = static_cast<uint8>(value);
        if (look != session->preview)
        {
            ShowLook(player, session->preview, look);
            session->preview = look;
        }
        SendPreview(player, *session);
        return true;
    }

    if (command == "C2S_PURCHASE")
    {
        Barbershop::Look look = session->preview;
        uint32 cost = GetCost(player, session->original, look);
        if (player->GetMoney() < cost)
        {
            Send(player, "S2C_ERROR;" + std::to_string(Barbershop::BARBERSHOP_ERROR_NO_MONEY));
            return true;
        }

        if (cost)
            player->ModifyMoney(-static_cast<int32>(cost));

        ApplyLook(player, look);
        session->original = look;

        // S2C_PURCHASE_RESULT;1;style;color;facial
        std::string message = "S2C_PURCHASE_RESULT;1";
        for (uint32 i = 0; i < Barbershop::CUSTOMIZATION_COUNT; ++i)
            message += ";" + std::to_string(look.values[i]);
        Send(player, message);
        return true;
    }

    if (command == "C2S_RESET")
    {
        if (session->preview != session->original)
        {
            ShowLook(player, session->preview, session->original);
            session->preview = session->original;
        }
        SendPreview(player, *session);
        return true;
    }

    if (command == "C2S_CLOSE")
    {
        Close(player, *session, true);
        player->SetStandState(UNIT_STAND_STATE_STAND);
        return true;
    }

    return false;
}

void BarbershopMgr::Update(Player* player)
{
    if (!m_count.load() || !player)
        return;

    std::lock_guard<std::mutex> guard(m_lock);

    Barbershop::Session* session = GetSession(player);
    if (!session)
        return;

    bool stay = player->IsAlive() && !player->IsInCombat() && IsInChairSitState(player);
    if (stay)
    {
        GameObject* chair = player->IsInWorld() ? player->GetMap()->GetGameObject(session->chair) : nullptr;
        stay = chair && player->GetDistance(chair) <= sBarbershopConfig.GetMaxChairDistance();
    }

    if (!stay)
        Close(player, *session, true);
}

void BarbershopMgr::OnLogout(Player* player)
{
    if (!m_count.load() || !player)
        return;

    std::lock_guard<std::mutex> guard(m_lock);

    Barbershop::Session* session = GetSession(player);
    if (session)
        Close(player, *session, false);
}
