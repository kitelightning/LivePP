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
    FDelegateHandle       syncPointHandle;
    void*	              lppHModule            = nullptr;

    //TODO: ikrimae:      #LivePP: Expose to settings class and test
    ELPPHookFilter        moduelFilter          = ELPPHookFilter::Game /*ELPPHookFilter::Game | ELPPHookFilter::CoreEngine*/;
    bool                  bHookImports          = false /* true */;
    ELPPSyncPointLocation lppHotReloadSyncPoint = ELPPSyncPointLocation::None;

    TArray<FString> CoreEngineModuleNames;
    TArray<FString> CoreEditorModuleNames;
    TArray<FString> CustomModuleNames;
};