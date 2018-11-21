// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class OLALibrary : ModuleRules
{
	public OLALibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

        PublicIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory,"include"),
                Path.Combine(ModuleDirectory,"ola"),
                Path.Combine(ModuleDirectory,"plugins")

				// ... add other private include paths required here ...
			}
        );

        if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Add the import library
			PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "x64", "Release"));
			PublicAdditionalLibraries.Add("ExampleLibrary.lib");

			// Delay-load the DLL, so we can load it from the right place first
			PublicDelayLoadDLLs.Add("ExampleLibrary.dll");
		}
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "Mac", "Release", "libExampleLibrary.dylib"));
        }
	}
}
