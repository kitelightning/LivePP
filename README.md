![](Resources/Icon128.png) 
# UE4 LivePP: C/C++ live coding
A UE4 plugin wrapper for the amazing Molecular Matter's Live++ Hot-Reloading Library. (https://molecular-matters.com/products_livepp.html).
One day we will get sub-second iteration in Unreal Engine 4.

![](doc/Screenshot.png) 

Documentation:
https://molecular-matters.com/docs/livepp/documentation.html

Status
----------
UE4 Plugin Features:

  - Slight validation for ensuring necessary Live++ flags are set (!Target.bAllowLTCG, !Target.bDisableDebugInfo, !Target.bOmitPCDebugInfoInDevelopment)
  
  - Support for PrePatchHook/PostPatchHook through engine delegates. Feature is commented out for now bc hasn't been tested
  
  - Support for syncing hotreload at engine's beginframe,, endframe, or to none. Hardcoded to none until more testing
  
  - Plugin loads in PostEngineInit(), loads LPP as an external module, and iterates over all loaded modules and registers for just game modules
  
  - Plugin disabled from running in commandlet mode
  

Tested Configuration:
  - LPP 1.2.5
  - UE4.19
  - Win64
  - In UE4 Editor
  - Plugin as a game project plugin

How to Use
----------
1. Modify UBT:
	In 4.20, there will be a way to pass AdditionalLinkArguments or AdditionalCppArguments from game modules. 
	Currently, there's no way to do it so you have to modify UBT to pass a necessary linker flag for L++. Two approaches:

		A. Force it on in VCToolChain.AppendLinkArguments()
			 - You will manually have to rebuild the entire solution if you want to hotreload engine modules

		B. Extend UBT Target rules (this is the 4.20 change coming):
			 -TargetRules.cs:TargetRules class:
				 [RequiresUniqueBuildEnvironment]
				 [XmlConfigFile(Category = "BuildConfiguration")]
				 public string AdditionalCompilerArguments;
				 [RequiresUniqueBuildEnvironment]
				 [XmlConfigFile(Category = "BuildConfiguration")]
				 public string AdditionalLinkerArguments;
			 -ReadOnlyTargetRules class:
				 public string AdditionalCompilerArguments
				 {
					 get { return Inner.AdditionalCompilerArguments; }
				 }
         
				 public string AdditionalLinkerArguments
				 {
					 get { return Inner.AdditionalLinkerArguments; }
				 }
			 -UEBuildTarget.SetupGlobalEnvironment():
				 GlobalCompileEnvironment.AdditionalArguments = Rules.AdditionalCompilerArguments;
				 GlobalLinkEnvironment.AdditionalArguments = Rules.AdditionalLinkerArguments;
			 -Your game target.cs file:
				 AdditionalCompilerArguments = "/Gw";
				 AdditionalLinkerArguments   = "/FUNCTIONPADMIN";
	 **NOTE:** UBT support for UniqueBuildEnvironment doesn't work  if you're game is not in a subdirectory of the UnrealEngine. Your best bet is to modify the buildconfiguration.xml(https://docs.unrealengine.com/en-US/Programming/UnrealBuildSystem/Configuration) file instead of yourgame.target.cs:
	 
		<BuildConfiguration>
		<AdditionalCompilerArguments> /Gw </AdditionalCompilerArguments>
		<AdditionalLinkerArguments> /FUNCTIONPADMIN </AdditionalLinkerArguments>
		</BuildConfiguration>

2. Clone this repo into your engine or game plugins directory (eg Plugins\LivePP)

3. Extract LPP.zip into this folder: [PluginsDir]\LivePP\Source\ThirdParty\LPPExternalLib
	The result should be a directory:
  
	[PluginsDir]\LivePP\Source\ThirdParty\LPPExternalLib\LivePP
  
	[PluginsDir]\LivePP\Source\ThirdParty\LPPExternalLib\LivePP\API\*
  
	[PluginsDir]\LivePP\Source\ThirdParty\LPPExternalLib\LivePP\x64\*
  
	[PluginsDir]\LivePP\Source\ThirdParty\LPPExternalLib\LivePP\x86\*
  
	etc


4. Optional suggestion:
	  These are other settings that you can play with. You have to approaches for faster iteration: 
  
	  A. Force off UnityBuilds (slower full recompilation but faster L++ hotreloading)
		 - bUseUnityBuild = false && bForceUnityBuild = false
     
	  B. Ensure adaptive unity builds are working (UBT uses git status or if it detects perforce, the file's readonly attribute
	   to determine if a source file has been touched. If so, it will pull that file out of the unity build. So you just have to
	   ensure to touch the file before you launch UE4 with L++
     
      - Settings to use: bUseAdaptiveUnityBuild, bAdaptiveUnityDisablesPCH, MinGameModuleSourceFilesForUnityBuild
     
	  C. Force all game modules to not use unity builds.
  
      - bFasterWithoutUnity = true
     
  The best workflow I've found is use adaptive unity builds for the engine while forcing all game modules to not use unity builds


  Troubleshooting:
  ----------
  Incredibuild 
  * Can't find compiler/must override cl/link.exe paths
		Incredibuild caches the compiler toolchain in a directory and then deletes it after a build so L++ won't be able to find it. 
		You can manually override the toolchain paths: C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Tools\MSVC\14.13.26128\bin\Hostx64\x64\cl.exe && C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Tools\MSVC\14.13.26128\bin\Hostx64\x64\link.exe
		
		Watch out for UE4 using 2015 toolchain (default) even if you are generating VS2017 projects.
		Turned out that even though he had a VS 2017 project and toold VS/UE to use the VS 2017 toolchain, somehow UBT insisted on using VS 2015. The overridden paths would point to his VS 2017 cl.exe, so the compiler would complain about the PCH being from a previous version of the compiler
