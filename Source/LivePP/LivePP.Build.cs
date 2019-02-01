// Copyright Kite & Lightning

using UnrealBuildTool;
using System;
using System.IO;

public class LivePP : ModuleRules
{
    void ValidateBuildTarget(ReadOnlyTargetRules Target)
    {
        
        var savedFGColor = System.Console.ForegroundColor;
        var savedBGColor = System.Console.BackgroundColor;
        System.Console.ForegroundColor = System.ConsoleColor.Magenta;

        System.Action<bool,string> checkAndWarn = (bCondition, buildFlag) => { if (!bCondition) { System.Console.WriteLine("LPP: Build Environment Flag is incorrect: {0} == false", buildFlag); } };

        //Ensure PDBs are generated
        checkAndWarn(!Target.bUsePDBFiles,                                  "!Target.bUsePDBFiles");
        checkAndWarn(!Target.bOmitPCDebugInfoInDevelopment,                 "!Target.bOmitPCDebugInfoInDevelopment");
        checkAndWarn(!Target.bDisableDebugInfo,                             "!Target.bDisableDebugInfo"            );
        checkAndWarn(!Target.bAllowLTCG,                                    "!Target.bAllowLTCG"                   );
        checkAndWarn(!(Target.bUseFastPDBLinking.GetValueOrDefault(false)), "!Target.bUseFastPDBLinking"           );
        checkAndWarn(!Target.bSupportEditAndContinue,                       "!Target.bSupportEditAndContinue");

        //NOTE: #ThirdParty-LivePP: Future L++ feature - Should be enabled if UHT hooking is implemented in L++
        //checkAndWarn(!Target.bDisableDebugInfoForGeneratedCode, "!Target.bDisableDebugInfoForGeneratedCode");

        //NOTE: UBT Extensions - These have to be placed inside of UBT in SetupEnvironment(). There's no longer a mechanism to control this from game modules or targets
        // Example of how to force all all game modules 
        //foreach (UEBuildModule Module in AllModules)
        //{
        //    if (!UnrealBuildTool.IsUnderAnEngineDirectory(Module.ModuleDirectory))
        //    {
        //        checkAndWarn(Module.Rules.bFasterWithoutUnity, "Module.Rules.bFasterWithoutUnity");
        //    }
        //}


        System.Console.ForegroundColor = savedFGColor;
        System.Console.BackgroundColor = savedBGColor;
    }

	public LivePP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        /* 
         **********************************************
         * IMPORTANT MANUAL STEP:                     *
         **********************************************
         * In 4.20, there will be a way to pass AdditionalLinkArguments or AdditionalCppArguments from 
         * game modules. Currently, there's no way to do it. Two approaches
         * 1. Force it on in VCToolChain.AppendLinkArguments()
         *      - You will manually have to rebuild the entire solution if you want to hotreload engine modules
         * 2. Extend UBT Target rules (this is the 4.20 change coming):
         *      -TargetRules class:
         *          [RequiresUniqueBuildEnvironment]
         *          public string AdditionalCompilerArguments;
         *          [RequiresUniqueBuildEnvironment]
         *      -ReadOnlyTargetRules class:
         *          public string AdditionalLinkerArguments;
         *          public string AdditionalCompilerArguments
         *          {
         *              get { return Inner.AdditionalCompilerArguments; }
         *          }
         *          
         *          public string AdditionalLinkerArguments
         *          {
         *              get { return Inner.AdditionalLinkerArguments; }
         *          }
         *      -UEBuildTarget.SetupGlobalEnvironment():
         *          GlobalCompileEnvironment.AdditionalArguments = Rules.AdditionalCompilerArguments;
         *          GlobalLinkEnvironment.AdditionalArguments = Rules.AdditionalLinkerArguments;
         *      -Your game target.cs file:
         *          AdditionalLinkerArguments = "/FUNCTIONPADMIN /Gw";
         *
        */

        /* 
         **********************************************
         * Suggestions                                *
         **********************************************
         * These are other settings that you can play with. You have to approaches for faster iteration: 
         * 1. Force off UnityBuilds (slower full recompilation but faster L++ hotreloading)
         *      - bUseUnityBuild = false && bForceUnityBuild = false
         * 2. Ensure adaptive unity builds are working (UBT uses git status or if it detects perforce, the file's readonly attribute
         *    to determine if a source file has been touched. If so, it will pull that file out of the unity build. So you just have to
         *    ensure to touch the file before you launch UE4 with L++
         *      - Settings to use: bUseAdaptiveUnityBuild, bAdaptiveUnityDisablesPCH, MinGameModuleSourceFilesForUnityBuild
         * 3. Force all game modules to not use unity builds.
         *      - bFasterWithoutUnity = true
         *      
         *  The best workflow I've found is use adaptive unity builds for the engine while forcing all game modules to not use unity builds
        */
        ValidateBuildTarget(Target);
		
		PublicIncludePaths.AddRange(
			new string[] {
                //Path.Combine(ModuleDirectory, "Public"),
				// ... add public include paths required here ...
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
                Path.Combine(ModuleDirectory, "Private"),
				// ... add other private include paths required here ...
			}
		);

        //PrivateIncludePathModuleNames.AddRange(new string[] {
        //    "EditorModule",
        //});


        PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
                "LPPExternalLib",
			}
		);			
		
		PrivateDependencyModuleNames.AddRange(
			new string[] {
                "Engine",
                "CoreUObject",
                "Projects",
				// ... add private dependencies that you statically link with here ...	
			}
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}
