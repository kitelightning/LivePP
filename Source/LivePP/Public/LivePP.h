// Copyright Kite & Lightning

#pragma once

#include "ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLPP, Log, All);

enum class ELPPSyncPointLocation : uint8
{
    None,
    EngineBeginFrame,
    EngineEndFrame,
};

class LIVEPP_API FLivePPModule : public IModuleInterface
{
public:
    virtual void StartupModule()            override;
    virtual void ShutdownModule()           override;
    virtual bool SupportsDynamicReloading() override { return true; }

    static FSimpleMulticastDelegate PrePatchHook;
    static FSimpleMulticastDelegate PostPatchHook;

private:
    FDelegateHandle       syncPointHandle;
    void*	              lppHModule            = nullptr;
    //TODO: ikrimae: #LivePP: Expose to settings class and test
    ELPPSyncPointLocation lppHotReloadSyncPoint = ELPPSyncPointLocation::None;
};