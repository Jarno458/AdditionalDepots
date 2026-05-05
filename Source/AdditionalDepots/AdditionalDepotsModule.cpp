// Copyright Epic Games, Inc. All Rights Reserved.

#include "AdditionalDepotsModule.h"

#include "FGCentralStorageContainer.h"
#include "FGCharacterPlayer.h"
#include "Patching/NativeHookManager.h"
#include "StructuredLog.h"

#define LOCTEXT_NAMESPACE "FAdditionalDepotsModule"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsModule);

void FAdditionalDepotsModule::StartupModule()
{
	UE_LOGFMT(LogAdditionalDepotsModule, Display, "FAdditionalDepotsModule::StartupModule()");

	if (!WITH_EDITOR)
	{
		SUBSCRIBE_METHOD(AFGCharacterPlayer::IsUploadInventoryEmpty, [](auto& Scope, const AFGCharacterPlayer* self) {
			UE_LOGFMT(LogAdditionalDepotsModule, Display, "AFGCharacterPlayer::IsUploadInventoryEmpty() overriding return");

			Scope.Override(true);
		});

		SUBSCRIBE_METHOD(AFGCentralStorageContainer::IsUploadInventoryEmpty, [](auto& Scope, AFGCentralStorageContainer* self) {
			UE_LOGFMT(LogAdditionalDepotsModule, Display, "AFGCentralStorageContainer::IsUploadInventoryEmpty() overriding return");

			Scope.Override(true);
		});
	}
}

void FAdditionalDepotsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAdditionalDepotsModule, AdditionalDepots)