#include "BarbershopConfig.h"
#include "Config/Config.h"
#include "Log.h"

#include <sstream>
#include <algorithm>

BarbershopConfig& BarbershopConfig::Instance()
{
    static BarbershopConfig instance;
    return instance;
}

void BarbershopConfig::Load()
{
    m_enabled = sConfig.GetBoolDefault("Barbershop.Enable", true);

    m_costBase   = static_cast<uint32>(sConfig.GetIntDefault("Barbershop.Cost.Base", 5));
    m_costStyle  = static_cast<uint32>(sConfig.GetIntDefault("Barbershop.Cost.Style", 100));
    m_costColor  = static_cast<uint32>(sConfig.GetIntDefault("Barbershop.Cost.Color", 50));
    m_costFacial = static_cast<uint32>(sConfig.GetIntDefault("Barbershop.Cost.FacialHair", 75));
    m_maxChairDistance = sConfig.GetFloatDefault("Barbershop.MaxChairDistance", 5.0f);

    m_chairEntries.clear();
    std::string chairList = sConfig.GetStringDefault("Barbershop.ChairEntries", "105180");
    std::replace(chairList.begin(), chairList.end(), ',', ' ');
    std::stringstream ss(chairList);
    uint32 entry = 0;
    while (ss >> entry)
    {
        if (entry > 0)
            m_chairEntries.insert(entry);
    }

    if (m_chairEntries.empty())
        m_chairEntries.insert(105180);

    sLog.outString("[Barbershop] Configuration loaded: Enabled=%d, Chairs=%zu, CostBase=%u, MaxDistance=%.1f",
        m_enabled ? 1 : 0, m_chairEntries.size(), m_costBase, m_maxChairDistance);
}

bool BarbershopConfig::IsChairEntry(uint32 entry) const
{
    return m_chairEntries.find(entry) != m_chairEntries.end();
}
