// Copyright Kite & Lightning

#include "LivePP.h"
#include "Core.h"
#include "ModuleManager.h"
#include "IPluginManager.h"
#include "CoreDelegates.h"
#include "LogMacros.h"

DEFINE_LOG_CATEGORY(LogLPP)

#define LOCTEXT_NAMESPACE "FLivePPModule"

#include "AllowWindowsPlatformTypes.h"
#include "LPP_API.h"

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

//TODO: ikrimae: #ThirdParty-LivePP: Uncomment after testing. Not really sure if there's even a need for it. 
//                        Maybe to mark that hotreload happened and to prevent saving over uassets?
//LPP_PREPATCH_HOOK(lppPrePatchHookShim);
//LPP_POSTPATCH_HOOK(lppPostPatchHookShim);

void FLivePPModule::StartupModule()
{
    if (IsRunningCommandlet())
    { return;    }

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

    lpp::lppRegisterProcessGroup(static_cast<HMODULE>(lppHModule), "QuickStart");

    //Register all game modules
    {
        TArray<FModuleStatus> ModuleStatuses;
        FModuleManager::Get().QueryModules(ModuleStatuses);
        const FString gameBinaryDir = FPaths::ConvertRelativePathToFull(FModuleManager::Get().GetGameBinariesDirectory());

        for (const FModuleStatus& ModuleStatus : ModuleStatuses)
        {
            if (!ModuleStatus.bIsLoaded)
            { continue; }

            const bool bShouldHook =
                   EnumHasAllFlags(moduelFilter, ELPPHookFilter::All)
                || EnumHasAllFlags(moduelFilter, ELPPHookFilter::Game)        && (ModuleStatus.bIsGameModule                                               )
                || EnumHasAllFlags(moduelFilter, ELPPHookFilter::GameProject) && (FPaths::IsSamePath(gameBinaryDir, FPaths::GetPath(ModuleStatus.FilePath)))
                || EnumHasAllFlags(moduelFilter, ELPPHookFilter::CoreEngine)  && (CoreEngineModuleNames.Contains(ModuleStatus.Name)                        )
                || EnumHasAllFlags(moduelFilter, ELPPHookFilter::CoreEditor)  && (CoreEditorModuleNames.Contains(ModuleStatus.Name)                        )
                || EnumHasAllFlags(moduelFilter, ELPPHookFilter::CustomList)  && (CustomModuleNames.Contains(ModuleStatus.Name)                            );

            if (bShouldHook)
            {
                if (bUseAsyncModuleLoad)
                {
                        if (bHookImports) { lppAsyncLoadTokens.Add(lpp::lppEnableAllModulesAsync(static_cast<HMODULE>(lppHModule), *ModuleStatus.FilePath)); }
                    else                  { lppAsyncLoadTokens.Add(lpp::lppEnableModuleAsync(static_cast<HMODULE>(lppHModule), *ModuleStatus.FilePath)); }
                }
                else
                {
                        if (bHookImports) { lpp::lppEnableAllModulesSync(static_cast<HMODULE>(lppHModule), *ModuleStatus.FilePath); }
                    else                  { lpp::lppEnableModuleSync(static_cast<HMODULE>(lppHModule), *ModuleStatus.FilePath); }
                }
            }
        }

        //syncModuleLoadHandle = FCoreDelegates::OnBeginFrame.AddLambda([this] {
        //    
        //
        //
        //    //Dangerous but ¯\_(ツ)_/¯; make sure to keep as the last line in the lambda as removal
        //    //will destroy the stack of the lambda
        //    FCoreDelegates::OnBeginFrame.Remove(syncModuleLoadHandle);
        //});

        //// Defer Level Editor UI extensions until Level Editor has been loaded:
        //if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
        //{
        //    //Register post engine stuff        
        //}
        //else
        //{
        //    FModuleManager::Get().OnModulesChanged().AddLambda([this, ThePlugin](FName name, EModuleChangeReason reason)
        //    {
        //        if ((name == "LevelEditor") && (reason == EModuleChangeReason::ModuleLoaded))
        //        {
        //            //Register post engine stuff
        //        }
        //    });
        //}
    }

    //Set up syncpoint delegates
    {
        switch (lppHotReloadSyncPoint)
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

    // Usually don't need to do this but here incase we want to enable turning on/off L++
    FPlatformProcess::FreeDllHandle(lppHModule);
    lppHModule = nullptr;
}

#include "HideWindowsPlatformTypes.h"

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FLivePPModule, LivePP)