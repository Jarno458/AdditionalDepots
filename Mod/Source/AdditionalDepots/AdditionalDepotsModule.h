// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FGCentralStorageSubsystem.h"
#include "Hologram/FGHologram.h"
#include "FGPlayerState.h"
#include "FGWorkBench.h"
#include "ItemAmount.h"
#include "Patching/NativeHookManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdditionalDepotsModule, Log, All);

class FAdditionalDepotsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	FDelegateHandle hologramCheckCanAffordHandle;
	FDelegateHandle workbenchCanProduceHandle;
	FDelegateHandle inventoryGrabItemsFromInventoryAndCentralStorage;

	static void CheckCanAffordHook(TCallScope<void(*)(AFGHologram*, UFGInventoryComponent*)>& func, AFGHologram* hologram, const UFGInventoryComponent* inventory);
	static void CanProduceHook(TCallScope<bool(*)(const UFGWorkBench*, TSubclassOf<UFGRecipe>, UFGInventoryComponent*)>& func, const UFGWorkBench* workBench, TSubclassOf<UFGRecipe> recipe, const UFGInventoryComponent* inventory);
	static void GrabItemsFromInventoryAndCentralStorageHook(TCallScope<void(*)(UFGInventoryComponent*, AFGCentralStorageSubsystem*, bool, TSubclassOf<UFGItemDescriptor>, int32)>& func, UFGInventoryComponent* inventory, AFGCentralStorageSubsystem* centralStorageSubsystem, bool takeFromInventoryBeforeCentralStorage, TSubclassOf<UFGItemDescriptor> itemClass, int32 numItemsToRemove);

	static bool ServerCanAfford(const TArray<FItemAmount>& itemAmounts, const UFGInventoryComponent* inventory, const AFGPlayerState* playerState);
	static bool ClientCanAfford(const TArray<FItemAmount>& itemAmounts, const UFGInventoryComponent* inventory, const AFGPlayerState* playerState);
};
