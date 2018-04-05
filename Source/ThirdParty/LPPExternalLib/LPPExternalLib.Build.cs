// Copyright Kite & Lightning

using System.IO;
using UnrealBuildTool;

public class LPPExternalLib : ModuleRules
{
    private string LppSdkPath
    {
        get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "LivePP")); }
    }

    private string BinariesPath(UnrealTargetPlatform Platform)
    {
        return Path.GetFullPath(Path.Combine(LppSdkPath, Platform == UnrealTargetPlatform.Win64 ? "x64" : "x86"));
    }

    public LPPExternalLib(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
		{
            PublicSystemIncludePaths.Add(Path.Combine(LppSdkPath, "API"));

            string archSuffix = Target.Platform == UnrealTargetPlatform.Win64 ? "x64" : "x86";
            RuntimeDependencies.Add(Path.Combine(BinariesPath(Target.Platform), string.Format("LPP_{0}.dll", archSuffix)));
            RuntimeDependencies.Add(Path.Combine(BinariesPath(Target.Platform), string.Format("LPP_{0}.exe", archSuffix)));
            RuntimeDependencies.Add(Path.Combine(BinariesPath(Target.Platform), "msdia140.dll"));
            //PublicDelayLoadDLLs.Add(string.Format("LPP_{0}.dll", archSuffix)));
		}
	}
}
