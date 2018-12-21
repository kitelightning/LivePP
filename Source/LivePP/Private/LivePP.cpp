// Copyright Kite & Lightning

#include "LivePP.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CoreDelegates.h"
#include "Logging/LogMacros.h"
#include "Misc/CommandLine.h"
#include "LivePPSettings.h"

DEFINE_LOG_CATEGORY(LogLPP)

#define LOCTEXT_NAMESPACE "FLivePPModule"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "LPP_API.h"
#include "Containers/StringConv.h"

FSimpleMulticastDelegate FLivePPModule::PrePatchHook;
FSimpleMulticastDelegate FLivePPModule::PostPatchHook;


namespace {
    void lppPrePatchHookShim()
    {
        FLivePPModule::PrePatchHook.Broadcast();
    }

    void lppPostPatchHookShim()
    {
        FLivePPModule::PostPatchHook.Broadcast();
    }
}

LPP_PREPATCH_HOOK(lppPrePatchHookShim);
LPP_POSTPATCH_HOOK(lppPostPatchHookShim);

void FLivePPModule::StartupModule()
{
    if (IsRunningCommandlet())
    { return; }

    const ULivePPSettings* lppCfg = GetDefault<ULivePPSettings>();

    bool bOverrideEnableLpp = true;
    FParse::Bool(FCommandLine::Get(), TEXT("lppEnable="), bOverrideEnableLpp);    
    if (!bOverrideEnableLpp || !lppCfg->bEnable)
    { return; }

    // Get the base directory of this plugin
    FString baseDir    = IPluginManager::Get().FindPlugin("LivePP").IsValid() ? IPluginManager::Get().FindPlugin("LivePP")->GetBaseDir() : "";
    FString lppDllPath = FPaths::Combine(*baseDir, 
#ifdef PLATFORM_64BITS
        TEXT("Source/ThirdParty/LPPExternalLib/LivePP/x64/LPP_x64.dll")
#else
        TEXT("Source/ThirdParty/LPPExternalLib/LivePP/x86/LPP_x86.dll")
#endif
    );

    //Load DLL
    lppHModule = !lppDllPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*lppDllPath) : nullptr;
    if (!lppHModule)
    {
        UE_LOG(LogLPP, Error, TEXT("Could not load external LPPExternalLib. Did you make sure to copy LPP SDK into ThirdParty/LPPExternalLib/LivePP?"));
        return;
    }

    if (!lpp::lppCheckVersion(static_cast<HMODULE>(lppHModule)))
    {
        // version mismatch detected
        UE_LOG(LogLPP, Error, TEXT("Could not load external LPPExternalLib. Version mismatch detected!"));
        FPlatformProcess::FreeDllHandle(lppHModule);
        lppHModule = nullptr;
        return;
    }

    lpp::lppRegisterProcessGroup(static_cast<HMODULE>(lppHModule), TCHAR_TO_ANSI(FPlatformProcess::ExecutableName()));

    //Register all game modules
    {
        const FString gameBinaryDir = FPaths::ConvertRelativePathToFull(FModuleManager::Get().GetGameBinariesDirectory());

        auto hookModule = [this,gameBinaryDir,lppCfg, moduleFilter = (ELPPHookFilter)lppCfg->moduleFilter](FName InModuleName, EModuleChangeReason InChangeReason) {
            if (InChangeReason != EModuleChangeReason::ModuleLoaded)
            { return; }

            FModuleStatus ModuleStatus;
            if (!FModuleManager::Get().QueryModule(InModuleName, ModuleStatus))
            { return; }

            const bool bShouldHook =
                   EnumHasAllFlags(moduleFilter, ELPPHookFilter::All)
                || EnumHasAllFlags(moduleFilter, ELPPHookFilter::Game)        && (ModuleStatus.bIsGameModule                                               )
                || EnumHasAllFlags(moduleFilter, ELPPHookFilter::GameProject) && (FPaths::IsSamePath(gameBinaryDir, FPaths::GetPath(ModuleStatus.FilePath)))
                || EnumHasAllFlags(moduleFilter, ELPPHookFilter::CoreEngine)  && (lppCfg->CoreEngineModuleNames.Contains(InModuleName)                )
                || EnumHasAllFlags(moduleFilter, ELPPHookFilter::CoreEditor)  && (lppCfg->CoreEditorModuleNames.Contains(InModuleName)                )
                || EnumHasAllFlags(moduleFilter, ELPPHookFilter::CustomList)  && (lppCfg->CustomModuleNames.Contains(InModuleName)                    );

            if (bShouldHook)
            {
                if (lppCfg->bUseAsyncModuleLoad)
                {
                        if (lppCfg->bHookImports) { lppAsyncLoadTokens.Add(lpp::lppEnableAllModulesAsync(static_cast<HMODULE>(lppHModule), *ModuleStatus.FilePath)); }
                    else                          { lppAsyncLoadTokens.Add(lpp::lppEnableModuleAsync(static_cast<HMODULE>(lppHModule), *ModuleStatus.FilePath)); }
                }
                else
                {
                        if (lppCfg->bHookImports) { lpp::lppEnableAllModulesSync(static_cast<HMODULE>(lppHModule), *ModuleStatus.FilePath); }
                    else                          { lpp::lppEnableModuleSync(static_cast<HMODULE>(lppHModule), *ModuleStatus.FilePath); }
                }
            }
        };
        
        TArray<FModuleStatus> ModuleStatuses;
        FModuleManager::Get().QueryModules(ModuleStatuses);
        for (const FModuleStatus& ModuleStatus : ModuleStatuses)
        {
            if (!ModuleStatus.bIsLoaded)
            { continue; }

            hookModule(FName(*ModuleStatus.Name), EModuleChangeReason::ModuleLoaded);
        }

        onModulesChangedDelegate = FModuleManager::Get().OnModulesChanged().AddLambda(hookModule);
    }


    //Set up syncpoint delegates
    {
        switch (lppCfg->lppHotReloadSyncPoint)
        {
        case ELPPSyncPointLocation::EngineBeginFrame:
            syncPointHandle = FCoreDelegates::OnBeginFrame.AddLambda([this] {
                if (lppAsyncLoadTokens.Num())
                {
                    for (void* lppToken : lppAsyncLoadTokens)
                    { lpp::lppWaitForToken(static_cast<HMODULE>(lppHModule), lppToken); }

                    lppAsyncLoadTokens.Empty();
                }
                lpp::lppSyncPoint(static_cast<HMODULE>(this->lppHModule));
            });
            break;
        case ELPPSyncPointLocation::EngineEndFrame:
            syncPointHandle = FCoreDelegates::OnEndFrame.AddLambda([this] {
                if (lppAsyncLoadTokens.Num())
                {
                    for (void* lppToken : lppAsyncLoadTokens)
                    { lpp::lppWaitForToken(static_cast<HMODULE>(lppHModule), lppToken); }

                    lppAsyncLoadTokens.Empty();
                }

                lpp::lppSyncPoint(static_cast<HMODULE>(this->lppHModule));
            });
            break;
        default:
        case ELPPSyncPointLocation::None:
            break;
        }
    }
}

void FLivePPModule::ShutdownModule()
{
    if (syncPointHandle.IsValid())
    {
        FCoreDelegates::OnBeginFrame.Remove(syncPointHandle);
        FCoreDelegates::OnEndFrame.Remove(syncPointHandle);
    }

    if (onModulesChangedDelegate.IsValid())
    {
        FModuleManager::Get().OnModulesChanged().Remove(onModulesChangedDelegate);
    }

    // Usually don't need to do this but here incase we want to enable turning on/off L++
    if (lppHModule)
    {
        FPlatformProcess::FreeDllHandle(lppHModule);
        lppHModule = nullptr;
    }
}

#include "Windows/HideWindowsPlatformTypes.h"

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FLivePPModule, LivePP)
