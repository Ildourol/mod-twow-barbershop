#ifndef MOD_TWOW_BARBERSHOP_CONFIG_H
#define MOD_TWOW_BARBERSHOP_CONFIG_H

#include "Common.h"
#include <unordered_set>
#include <string>

class BarbershopConfig
{
public:
    static BarbershopConfig& Instance();

    void Load();

    bool IsEnabled() const { return m_enabled; }
    bool IsChairEntry(uint32 entry) const;

    uint32 GetCostBase() const { return m_costBase; }
    uint32 GetCostStyle() const { return m_costStyle; }
    uint32 GetCostColor() const { return m_costColor; }
    uint32 GetCostFacial() const { return m_costFacial; }
    float GetMaxChairDistance() const { return m_maxChairDistance; }

private:
    BarbershopConfig() = default;

    bool m_enabled = true;
    std::unordered_set<uint32> m_chairEntries;

    uint32 m_costBase = 5;
    uint32 m_costStyle = 100;
    uint32 m_costColor = 50;
    uint32 m_costFacial = 75;
    float m_maxChairDistance = 5.0f;
};

#define sBarbershopConfig BarbershopConfig::Instance()

#endif // MOD_TWOW_BARBERSHOP_CONFIG_H
