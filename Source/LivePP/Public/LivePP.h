// Copyright Kite & Lightning

#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"
#include <EnumClassFlags.h>

DECLARE_LOG_CATEGORY_EXTERN(LogLPP, Log, All);

class LIVEPP_API FLivePPModule : public IModuleInterface
{
public:
    virtual void StartupModule()            override;
    virtual void ShutdownModule()           override;
    virtual bool SupportsDynamicReloading() override { return true; }
    
    static FSimpleMulticastDelegate PrePatchHook;
    static FSimpleMulticastDelegate PostPatchHook;

private:
    FDelegateHandle onModulesChangedDelegate;

    void*	              lppHModule = nullptr;
    FDelegateHandle       syncPointHandle;
    TArray<void*>         lppAsyncLoadTokens;
};