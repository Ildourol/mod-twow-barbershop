#ifndef MOD_TWOW_BARBERSHOP_MGR_H
#define MOD_TWOW_BARBERSHOP_MGR_H

#include "BarbershopCommon.h"
#include "ObjectGuid.h"

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class GameObject;
class Player;

class BarbershopMgr
{
public:
    static BarbershopMgr& Instance();

    void Open(Player* player, GameObject* chair);
    bool HandleAddonMessage(Player* player, std::string const& msg);

    void Update(Player* player);
    void OnLogout(Player* player);

    bool HasActiveSession(Player const* player) const;

private:
    BarbershopMgr() = default;

    Barbershop::Session* GetSession(Player const* player);
    void Close(Player* player, Barbershop::Session const& session, bool notify);

    Barbershop::Look ReadLook(Player const* player);
    void ApplyLook(Player* player, Barbershop::Look const& look);
    void ShowLook(Player* player, Barbershop::Look const& shown, Barbershop::Look const& look);

    uint32 GetCost(Player const* player, Barbershop::Look const& from, Barbershop::Look const& to) const;
    bool IsAllowed(Player const* player, Barbershop::Look const& original, uint32 customization, uint8 value) const;
    uint8 GetLimit(Player const* player, uint32 customization) const;

    void Send(Player* player, std::string const& message) const;
    void SendPreview(Player* player, Barbershop::Session const& session) const;
    void SendValuesToSelf(Player* player, std::vector<std::pair<uint16, uint32>> const& fields) const;

    mutable std::mutex m_lock;
    std::unordered_map<uint32, Barbershop::Session> m_sessions;
    std::atomic<uint32> m_count{0};
};

#define sBarbershopMgr BarbershopMgr::Instance()

#endif // MOD_TWOW_BARBERSHOP_MGR_H
