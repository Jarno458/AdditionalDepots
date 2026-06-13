#include "AdditionalDepotsModule.h"

#include "FGCharacterPlayer.h"
#include "Patching/NativeHookManager.h"
#include "Logging/StructuredLog.h"
#include "FGInventoryComponent.h"
#include "AdditionalDepotsClientSubsystem.h"
#include "AdditionalDepotsServerSubsystem.h"
#include "AdditionalDepotsUtils.h"
#include "FGConstructDisqualifier.h"
#include "FGInventoryLibrary.h"
#include "FGPlayerController.h"

#define LOCTEXT_NAMESPACE "FAdditionalDepotsModule"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsModule);

void FAdditionalDepotsModule::StartupModule()
{
	if (!WITH_EDITOR)
	{
		hologramCheckCanAffordHandle = SUBSCRIBE_UOBJECT_METHOD(AFGHologram, CheckCanAfford, CheckCanAffordHook);
		workbenchCanProduceHandle = SUBSCRIBE_UOBJECT_METHOD(UFGWorkBench, CanProduce, CanProduceHook);
		inventoryGrabItemsFromInventoryAndCentralStorage = SUBSCRIBE_METHOD(UFGInventoryLibrary::GrabItemsFromInventoryAndCentralStorage, GrabItemsFromInventoryAndCentralStorageHook); //SUBSCRIBE_UOBJECT_METHOD wont compile

		//TODO - need to find a plan how to properly hook this (Known bug, checking "Show only affordable" in crafting menu, will have entire categories if it does not find any affordable recipes
		//static TArray< TSubclassOf< class UFGItemCategory > > GetCategoriesWithAffordableRecipes(AFGCharacterPlayer * playerPawn, TSubclassOf< UObject > forProducer);
	}
}

void FAdditionalDepotsModule::ShutdownModule()
{
	if (!WITH_EDITOR)
	{
		if (hologramCheckCanAffordHandle.IsValid())
			UNSUBSCRIBE_UOBJECT_METHOD(AFGHologram, CheckCanAfford, hologramCheckCanAffordHandle);
		if (workbenchCanProduceHandle.IsValid())
			UNSUBSCRIBE_UOBJECT_METHOD(UFGWorkBench, CanProduce, workbenchCanProduceHandle);
		if (inventoryGrabItemsFromInventoryAndCentralStorage.IsValid())
			UNSUBSCRIBE_METHOD(UFGInventoryLibrary::GrabItemsFromInventoryAndCentralStorage, inventoryGrabItemsFromInventoryAndCentralStorage);
	}
}

void FAdditionalDepotsModule::CheckCanAffordHook(TCallScope<void(*)(AFGHologram*, UFGInventoryComponent*)>& func, AFGHologram* hologram, const UFGInventoryComponent* inventory) {
	if (!IsValid(hologram) || !IsValid(inventory)) {
		UE_LOGFMT(LogAdditionalDepotsModule, Error, "AFGHologram::CheckCanAfford() - hologram or inventory is not valid! FALLBACK to original function");
		return;
	}

	if (inventory->GetNoBuildCost())
		return;

	AFGPlayerState* state;

	AFGCharacterPlayer* playerPawn = Cast<AFGCharacterPlayer>(hologram->GetConstructionInstigator());
	if (playerPawn)
		state = Cast<AFGPlayerState>(playerPawn->GetPlayerState());
	else
		state = UAdditionalDepotsUtils::TryGetPlayerStateFromInventory(inventory);

	if (!state)
		return;

	func.Cancel();

	TArray<FItemAmount> costs = hologram->GetCost(true);

	//don't use hologram->HasAuthority() as its true on pure clients
	if (inventory->HasAuthority())
	{
		if (!ServerCanAfford(costs, inventory, state))
			hologram->AddConstructDisqualifier(UFGCDUnaffordable::StaticClass());
	}
	else
	{
		if (!ClientCanAfford(costs, inventory, state))
			hologram->AddConstructDisqualifier(UFGCDUnaffordable::StaticClass());
	}
}

void FAdditionalDepotsModule::CanProduceHook(TCallScope<bool(*)(const UFGWorkBench*, TSubclassOf<UFGRecipe>, UFGInventoryComponent*)>& func, const UFGWorkBench* workBench, TSubclassOf<UFGRecipe> recipe, const UFGInventoryComponent* inventory)
{
	if (!IsValid(workBench) || !IsValid(inventory)) {
		UE_LOGFMT(LogAdditionalDepotsModule, Error, "UFGWorkBench::CanProduce() - workBench or inventory is not valid! FALLBACK to original function");
		return;
	}

	if (inventory->GetNoBuildCost())
		return;

	//TODO not sure if we need to care about having access to the recipe

	AFGPlayerState* state = inventory->GetOwningPlayerState();
	if (!state)
	{
		UE_LOGFMT(LogAdditionalDepotsModule, Error, "UFGWorkBench::CanProduce() - Could not get player state! FALLBACK to inventory + dimensional depot");
		return;
	}

	TArray<FItemAmount> costs = UFGRecipe::GetIngredients(inventory, recipe);

	if (workBench->HasAuthority())
		func.Override(ServerCanAfford(costs, inventory, state));
	else
		func.Override(ClientCanAfford(costs, inventory, state));
}

void FAdditionalDepotsModule::GrabItemsFromInventoryAndCentralStorageHook(
	TCallScope<void(*)(UFGInventoryComponent*, AFGCentralStorageSubsystem*, bool, TSubclassOf<UFGItemDescriptor>, int32)>& func,
	UFGInventoryComponent* inventory, AFGCentralStorageSubsystem* centralStorageSubsystem, bool takeFromInventoryBeforeCentralStorage, TSubclassOf<UFGItemDescriptor> itemClass, int32 numItemsToRemove)
{
	if (!IsValid(inventory) || !IsValid(centralStorageSubsystem)) {
		UE_LOGFMT(LogAdditionalDepotsModule, Error, "UFGInventoryLibrary::GrabItemsFromInventoryAndCentralStorage() - inventory or centralStorageSubsystem is not valid! FALLBACK to original function");
		return;
	}

	if (inventory->GetNoBuildCost())
		return;

	if (!centralStorageSubsystem->HasAuthority())
	{
		UE_LOGFMT(LogAdditionalDepotsModule, Error, "UFGInventoryLibrary::GrabItemsFromInventoryAndCentralStorage() - called from client");
		return;
	}

	AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(inventory->GetWorld());
	if (!serverSubsystem)
	{
		UE_LOGFMT(LogAdditionalDepotsModule, Error, "AFGHologram::CheckCanAfford() - Could not get server subsystem!");
		return;
	}

	AFGPlayerState* playerState = UAdditionalDepotsUtils::TryGetPlayerStateFromInventory(inventory);
	if (!playerState)
	{
		UE_LOGFMT(LogAdditionalDepotsModule, Error, "AFGHologram::CheckCanAfford() - Could not get player state for inventory!");
		return;
	}

	serverSubsystem->PayBuildingCost(centralStorageSubsystem, inventory, itemClass, numItemsToRemove, playerState);
}

bool FAdditionalDepotsModule::ServerCanAfford(const TArray<FItemAmount>& itemAmounts, const UFGInventoryComponent* inventory, const AFGPlayerState* playerState)
{
	AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(playerState->GetWorld());
	if (!serverSubsystem)
	{
		UE_LOGFMT(LogAdditionalDepotsModule, Error, "AFGHologram::CheckCanAfford() - Could not get server subsystem!");
		return false;
	}

	for (const FItemAmount& itemCost : itemAmounts)
	{
		int32 availableAmount = serverSubsystem->GetAmountForBuildingForItem(inventory, itemCost.ItemClass, playerState);
		if (availableAmount < itemCost.Amount)
			return false;
	}

	return true;
}

bool FAdditionalDepotsModule::ClientCanAfford(const TArray<FItemAmount>& itemAmounts, const UFGInventoryComponent* inventory, const AFGPlayerState* playerState)
{
	AAdditionalDepotsClientSubsystem* clientSubsystem = AAdditionalDepotsClientSubsystem::Get(playerState->GetWorld());

	if (!clientSubsystem)
	{
		UE_LOGFMT(LogAdditionalDepotsModule, Error, "AFGHologram::CheckCanAfford() - Could not get client subsystem!");
		return false;
	}

	for (const FItemAmount& itemCost : itemAmounts)
	{
		int32 availableAmount = clientSubsystem->GetAmountForBuildingForItem(inventory, itemCost.ItemClass, playerState);
		if (availableAmount < itemCost.Amount)
		{
			return false;
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAdditionalDepotsModule, AdditionalDepots)