#include "ScriptObjects.h"
#include "Log.h"
#include "BarbershopConfig.h"

void AddBarbershopScripts();

namespace
{
    class ModTwowBarbershopWorldScript : public WorldScript
    {
    public:
        ModTwowBarbershopWorldScript()
            : WorldScript("mod-twow-barbershop_world", { WORLDHOOK_ON_STARTUP })
        {
        }

        void OnStartup() override
        {
            sBarbershopConfig.Load();
            sLog.outString(">> [mod-twow-barbershop] module initialized successfully.");
        }
    };
}

void Addmod_twow_barbershopScripts()
{
    new ModTwowBarbershopWorldScript();
    AddBarbershopScripts();
}
