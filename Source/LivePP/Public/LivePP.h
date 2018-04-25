// Copyright Kite & Lightning

#pragma once

#include "ModuleManager.h"
#include <EnumClassFlags.h>

DECLARE_LOG_CATEGORY_EXTERN(LogLPP, Log, All);

enum class ELPPSyncPointLocation : uint8
{
    None,
    EngineBeginFrame,
    EngineEndFrame,
};

enum class ELPPHookFilter : uint8
{
    None        = 0,
    Game        = 1,
    GameProject = 2,
    CoreEngine  = 4,
    CoreEditor  = 8,

    CustomList = 128,
    All        = 255,
};
ENUM_CLASS_FLAGS(ELPPHookFilter);

class LIVEPP_API FLivePPModule : public IModuleInterface
{
public:
    FLivePPModule()
    {
        CoreEngineModuleNames = {
            TEXT("Core"),
            TEXT("CoreUObject"),
            TEXT("Engine"),
            TEXT("RenderCore"),
            TEXT("ShaderCore"),
            TEXT("RHI"),
            TEXT("InputCore"),
        };

        CoreEditorModuleNames = {
            TEXT("UnrealEd"),
            TEXT("Sequencer"),
        };

        CustomModuleNames = {
            TEXT("Slate"),
            TEXT("SlateCore"),
            TEXT("ApplicationCore")
        };
    }

    virtual void StartupModule()            override;
    virtual void ShutdownModule()           override;
    virtual bool SupportsDynamicReloading() override { return true; }

    static FSimpleMulticastDelegate PrePatchHook;
    static FSimpleMulticastDelegate PostPatchHook;

private:
    void*	              lppHModule = nullptr;
    FDelegateHandle       syncPointHandle;
    TArray<void*>         lppAsyncLoadTokens;

    //TODO: ikrimae:      #ThirdParty-LivePP: Expose to settings class and test
    ELPPHookFilter        moduelFilter          = ELPPHookFilter::Game | ELPPHookFilter::GameProject /*| ELPPHookFilter::CoreEngine*/;
    bool                  bHookImports          = true;
    bool                  bUseAsyncModuleLoad   = false;
    ELPPSyncPointLocation lppHotReloadSyncPoint = ELPPSyncPointLocation::EngineEndFrame;

    TArray<FString> CoreEngineModuleNames;
    TArray<FString> CoreEditorModuleNames;
    TArray<FString> CustomModuleNames;
};