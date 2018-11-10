// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OLA.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "OLALibrary/ExampleLibrary.h"

#define LOCTEXT_NAMESPACE "FOLAModule"

DEFINE_LOG_CATEGORY(LogOla);

void FOLAModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Get the base directory of this plugin
	FString BaseDir = IPluginManager::Get().FindPlugin("OLA")->GetBaseDir();

	// Add on the relative location of the third party dll and load it
	FString LibraryPath;
#if PLATFORM_WINDOWS
	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Binaries/ThirdParty/OLALibrary/Win64/ExampleLibrary.dll"));
#elif PLATFORM_MAC
    LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/OLALibrary/Mac/Release/libExampleLibrary.dylib"));
#endif // PLATFORM_WINDOWS

	ExampleLibraryHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;

	if (ExampleLibraryHandle)
	{
		// Call the test function in the third party library that opens a message box
		ExampleLibraryFunction();
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "Failed to load example third party library"));
	}
}

void FOLAModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// Free the dll handle
	FPlatformProcess::FreeDllHandle(ExampleLibraryHandle);
	ExampleLibraryHandle = nullptr;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOLAModule, OLA)
