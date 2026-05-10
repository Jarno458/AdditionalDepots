// Copyright Epic Games, Inc. All Rights Reserved.

#include "AdditionalDepotsModule.h"

#include "FGCentralStorageSubsystem.h"
#include "FGCharacterPlayer.h"
#include "Patching/NativeHookManager.h"
#include "StructuredLog.h"

#define LOCTEXT_NAMESPACE "FAdditionalDepotsModule"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsModule);

void FAdditionalDepotsModule::StartupModule()
{
	UE_LOGFMT(LogAdditionalDepotsModule, Display, "FAdditionalDepotsModule::StartupModule()");

	//if (!WITH_EDITOR)
	{
		/*SUBSCRIBE_METHOD(AFGCentralStorageSubsystem::GetAllItemsFromCentralStorage, [](auto& Scope, AFGCentralStorageSubsystem* self, TArray<FItemAmount>& out_allItems) {
			UE_LOGFMT(LogAdditionalDepotsModule, Display, "AFGCentralStorageContainer::GetAllItemsFromCentralStorage() getting all items");

			Scope(self, out_allItems);

			if (out_allItems.Num() == 0)
			{
				UE_LOGFMT(LogAdditionalDepotsModule, Display, "AFGCentralStorageContainer::GetAllItemsFromCentralStorage() Store is empty, fakin it");

				//todo create dummy item i guess, and filter it out on the ui side
				//out_allItems.Add(FItemAmount());
			}
			else
			{
				UE_LOGFMT(LogAdditionalDepotsModule, Display, "AFGCentralStorageContainer::GetAllItemsFromCentralStorage() has %d items", out_allItems.Num());
			}
		});*/
	}
}

void FAdditionalDepotsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAdditionalDepotsModule, AdditionalDepots)