#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"
#include "Qiantong/CoreVersion.h"

class FQiantongUEModule final : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
        FDefaultGameModuleImpl::StartupModule();
        const auto Version = Qiantong::CoreVersion();
        UE_LOG(LogTemp, Display, TEXT("QiantongUE loaded portable Core %.*hs"),
            static_cast<int>(Version.size()), Version.data());
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FQiantongUEModule, QiantongUE, "QiantongCore");
