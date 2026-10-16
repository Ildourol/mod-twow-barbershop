#include "BarbershopCommon.h"
#include "BarbershopConfig.h"
#include "BarbershopMgr.h"

#include "Chat.h"
#include "GameObject.h"
#include "Opcodes.h"
#include "Player.h"
#include "ScriptObjects.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <cstring>
#include <string>

class BarberChairScript : public GameObjectScript
{
public:
    BarberChairScript() : GameObjectScript("go_barber_chair") {}

    bool OnGossipHello(Player* player, GameObject* chair) override
    {
        if (!player || !chair)
            return false;

        if (!sBarbershopConfig.IsEnabled() || !sBarbershopConfig.IsChairEntry(chair->GetEntry()))
            return false;

        if (!player->IsAlive() || player->IsInCombat())
            return true;

        // Seat the player in the closest chair slot
        float slotX = 0.0f;
        float slotY = 0.0f;
        chair->GetClosestChairSlotPosition(player->GetPositionX(), player->GetPositionY(), slotX, slotY);
        player->NearTeleportTo(slotX, slotY, chair->GetPositionZ(), chair->GetOrientation(),
            TELE_TO_NOT_LEAVE_COMBAT | TELE_TO_NOT_UNSUMMON_PET);

        GameObjectInfo const* info = chair->GetGOInfo();
        uint8 chairHeight = info ? info->chair.height : 0;
        player->SetStandState(UNIT_STAND_STATE_SIT_LOW_CHAIR + chairHeight);

        // Open the barbershop session
        sBarbershopMgr.Open(player, chair);
        return true;
    }
};

class BarbershopServerScript : public ServerScript
{
public:
    BarbershopServerScript() : ServerScript("BarbershopServerScript", { SERVERHOOK_CAN_PACKET_RECEIVE }) {}

    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override
    {
        if (!sBarbershopConfig.IsEnabled() || !session || !session->GetPlayer())
            return true;

        if (packet.GetOpcode() == CMSG_MESSAGECHAT)
        {
            try
            {
                WorldPacket copy = packet;
                copy.rpos(0);

                uint32 type = 0;
                uint32 language = 0;
                copy >> type >> language;

                if (language == LANG_ADDON)
                {
                    std::string msg;
                    if (type == CHAT_MSG_WHISPER)
                    {
                        std::string to;
                        copy >> to >> msg;
                    }
                    else if (type == CHAT_MSG_CHANNEL)
                    {
                        std::string channel;
                        copy >> channel >> msg;
                    }
                    else
                    {
                        copy >> msg;
                    }

                    size_t tabPos = msg.find('\t');
                    std::string prefix = (tabPos != std::string::npos) ? msg.substr(0, tabPos) : msg;
                    std::string payload = (tabPos != std::string::npos) ? msg.substr(tabPos + 1) : "";

                    if (prefix == Barbershop::Prefix)
                    {
                        sBarbershopMgr.HandleAddonMessage(session->GetPlayer(), payload);
                        return false; // Intercept and consume packet, bypassing core guild drop for guildless players
                    }
                }
            }
            catch (ByteBufferException const&)
            {
            }
        }

        return true;
    }
};

class BarberPlayerScript : public PlayerScript
{
public:
    BarberPlayerScript()
        : PlayerScript("BarberPlayerScript",
            {
                PLAYERHOOK_ON_LOGOUT,
                PLAYERHOOK_ON_UPDATE,
                PLAYERHOOK_ON_ADDON_MESSAGE
            })
    {
    }

    void OnLogout(Player* player) override
    {
        sBarbershopMgr.OnLogout(player);
    }

    void OnUpdate(Player* player, uint32 /*diff*/) override
    {
        sBarbershopMgr.Update(player);
    }

    bool OnAddonMessage(Player* from, std::string const& msg) override
    {
        if (!from || !sBarbershopConfig.IsEnabled())
            return false;

        size_t tabPos = msg.find('\t');
        std::string prefix = (tabPos != std::string::npos) ? msg.substr(0, tabPos) : msg;
        std::string payload = (tabPos != std::string::npos) ? msg.substr(tabPos + 1) : "";

        if (prefix == Barbershop::Prefix)
        {
            return sBarbershopMgr.HandleAddonMessage(from, payload);
        }

        return false;
    }
};

void AddBarbershopScripts()
{
    new BarberChairScript();
    new BarberPlayerScript();
    new BarbershopServerScript();
}
