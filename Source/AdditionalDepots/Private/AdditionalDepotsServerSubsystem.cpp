#include "AdditionalDepotsServerSubsystem.h"

#include "AdditionalDepotsDataTypes.h"
#include "AdditionalDepotsPerPlayerDataComponent.h"
#include "AdditionalDepotsReservedIdentifiers.h"
#include "AdditionalDepotsUtils.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsServerSubsystem);

AAdditionalDepotsServerSubsystem::AAdditionalDepotsServerSubsystem() : Super() {
	PrimaryActorTick.bCanEverTick = false;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

AAdditionalDepotsServerSubsystem* AAdditionalDepotsServerSubsystem::Get(UWorld* world) {
	USubsystemActorManager* subsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(subsystemActorManager);

	return subsystemActorManager->GetSubsystemActor<AAdditionalDepotsServerSubsystem>();
}

AAdditionalDepotsServerSubsystem* AAdditionalDepotsServerSubsystem::Get(UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

void AAdditionalDepotsServerSubsystem::BeginPlay() {
	UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Display, "AAdditionalDepotsServerSubsystem::BeginPlay()");
	Super::BeginPlay();
}

void AAdditionalDepotsServerSubsystem::PostActorCreated()
{
	UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Display, "AAdditionalDepotsServerSubsystem::PostActorCreated()");

	Super::PostActorCreated();

	const TArray<TSubclassOf<UAdditionalDepotDefinition>> lists = UAdditionalDepotsUtils::LoadAdditionalDepotLists();
	for (const TSubclassOf<UAdditionalDepotDefinition>& list : lists)
		AddList(list);

	initialized = true;
}

void AAdditionalDepotsServerSubsystem::SetDepotContent(FName listIdentifier, TArray<FItemAmount> items, AFGPlayerState* playerState)
{
	if (!listIdentifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Warning, "LogAdditionalDepotsServerSubsystem::SetDepotContent() - Invalid list identifier!");
		return;
	}

	if (!depotConfigurations.Contains(listIdentifier))
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Error, "LogAdditionalDepotsServerSubsystem::SetDepotContent(listIdentifier: {0}) - Depot configuration not found!", listIdentifier.ToString());
		return;
	}

	if (listIdentifier == UAdditionalDepotsReservedIdentifiers::GetDimensionalDepotIdentifier())
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Error, "LogAdditionalDepotsServerSubsystem::SetDepotContent({0}) Cannot set contents of dimensional storage, use the AFGCentralStorageSubsystem instead");
		return;
	}

	TArray<TSubclassOf<UFGItemDescriptor>> updatedItems;
	{
		FScopeLock lock(&depotLock);

		TMap<FName, FMappedItemAmount>* depotContentsMap = GetDepotContent(listIdentifier, playerState);
		if (!depotContentsMap)
			return;

		FMappedItemAmount& depotContent = depotContentsMap->FindOrAdd(listIdentifier);

		TArray<TSubclassOf<UFGItemDescriptor>> removedItems;
		depotContent.ItemAmounts.GenerateKeyArray(removedItems);

		depotContent.ItemAmounts.Empty();

		for (const FItemAmount& item : items)
			depotContent.ItemAmounts.Add(item.ItemClass, item.Amount);

		depotContent.ItemAmounts.GenerateKeyArray(updatedItems);

		for (const TSubclassOf<UFGItemDescriptor>& removedItem : removedItems)
		{
			if (!updatedItems.Contains(removedItem))
				updatedItems.Add(removedItem);
		}
	}

	BroadCastNewItemAmounts(listIdentifier, updatedItems, playerState);
}

int32 AAdditionalDepotsServerSubsystem::AddItem(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount, AFGPlayerState* playerState)
{
	int32 added = AddItemInternal(listIdentifier, itemClass, amount, playerState);

	BroadCastNewItemAmounts(listIdentifier, { itemClass }, playerState);

	return added;
}

TArray<FItemAmount> AAdditionalDepotsServerSubsystem::AddItems(FName listIdentifier, TArray<FItemAmount> items, AFGPlayerState* playerState)
{
	TMap<TSubclassOf<UFGItemDescriptor>, int32> addedItems;

	for (const FItemAmount& Item : items)
	{
		int32 numberOfAdded = AddItem(listIdentifier, Item.ItemClass, Item.Amount, playerState);

		int32& addedItem = addedItems.FindOrAdd(Item.ItemClass, 0);
		addedItem += numberOfAdded;
	}

	TArray<TSubclassOf<UFGItemDescriptor>> updatedItems;
	addedItems.GenerateKeyArray(updatedItems);
	BroadCastNewItemAmounts(listIdentifier, updatedItems, playerState);

	TArray<FItemAmount> result;
	for (const TPair<TSubclassOf<UFGItemDescriptor>, int32>& pair : addedItems)
		result.Add(FItemAmount(pair.Key, pair.Value));

	return result;
}

int32 AAdditionalDepotsServerSubsystem::RemoveItem(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount, AFGPlayerState* playerState)
{
	int32 removed = RemoveItemInternal(listIdentifier, itemClass, amount, playerState);

	BroadCastNewItemAmounts(listIdentifier, { itemClass }, playerState);
	return removed;
}

TArray<FItemAmount> AAdditionalDepotsServerSubsystem::RemoveItems(FName listIdentifier, TArray<FItemAmount> items, AFGPlayerState* playerState)
{
	TMap<TSubclassOf<UFGItemDescriptor>, int32> removedItems;

	for (const FItemAmount& Item : items)
	{
		int32 numberOfRemoved = RemoveItem(listIdentifier, Item.ItemClass, Item.Amount, playerState);

		int32& removedItem = removedItems.FindOrAdd(Item.ItemClass, 0);
		removedItem += numberOfRemoved;
	}

	TArray<TSubclassOf<UFGItemDescriptor>> updatedItems;
	removedItems.GenerateKeyArray(updatedItems);
	BroadCastNewItemAmounts(listIdentifier, updatedItems, playerState);

	TArray<FItemAmount> result;
	for (const TPair<TSubclassOf<UFGItemDescriptor>, int32>& pair : removedItems)
		result.Add(FItemAmount(pair.Key, pair.Value));

	return result;
}

TArray<FName> AAdditionalDepotsServerSubsystem::GetListIdentifiers()
{
	TArray<FName> lists;
	depotConfigurations.GenerateKeyArray(lists);
	return lists;
}

TArray<FItemAmount> AAdditionalDepotsServerSubsystem::GetItems(FName listIdentifier, AFGPlayerState* playerState)
{
	TArray<FItemAmount> items;

	if (!listIdentifier.IsValid())
		return items;

	TMap<FName, FMappedItemAmount>* depotContentsMap = GetDepotContent(listIdentifier, playerState);
	if (!depotContentsMap)
		return items;

	if (!depotContentsMap || !depotContentsMap->Contains(listIdentifier))
		return items;

	if (listIdentifier == UAdditionalDepotsReservedIdentifiers::GetDimensionalDepotIdentifier())
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Error, "LogAdditionalDepotsServerSubsystem::GetItems({0}) Cannot get contents of dimensional storage, use the AFGCentralStorageSubsystem instead", listIdentifier.ToString());
		return items;
	}

	FScopeLock lock(&depotLock);
	for (const TPair<TSubclassOf<UFGItemDescriptor>, int32>& item : (*depotContentsMap)[listIdentifier].ItemAmounts)
		items.Add(FItemAmount(item.Key, item.Value));

	return items;
}

FAdditionalDepotConfiguration AAdditionalDepotsServerSubsystem::GetConfiguration(FName listIdentifier)
{
	if (!listIdentifier.IsValid() || !depotConfigurations.Contains(listIdentifier))
		return FAdditionalDepotConfiguration();

	return depotConfigurations[listIdentifier];
}

void AAdditionalDepotsServerSubsystem::UpdateCanDragToInventory(FName listIdentifier, bool canDrag)
{
	if (!listIdentifier.IsValid() || !depotConfigurations.Contains(listIdentifier))
		return;

	if (depotConfigurations[listIdentifier].CanDragItemsToInventory == canDrag)
		return;

	depotConfigurations[listIdentifier].CanDragItemsToInventory = canDrag;

	BroadCastNewConfiguration(listIdentifier);
}

void AAdditionalDepotsServerSubsystem::UpdateCanBeUsedForBuildingAndCrafting(FName listIdentifier, bool canBeUsed)
{
	if (!listIdentifier.IsValid() || !depotConfigurations.Contains(listIdentifier))
		return;

	if (depotConfigurations[listIdentifier].CanBeUsedWhenBuilding == canBeUsed)
		return;

	depotConfigurations[listIdentifier].CanBeUsedWhenBuilding = canBeUsed;

	BroadCastNewConfiguration(listIdentifier);
}

void AAdditionalDepotsServerSubsystem::UpdateMaxAmount(FName listIdentifier, EFAAdditionalDepotsMaxType maxType, int max)
{
	if (!listIdentifier.IsValid() || !depotConfigurations.Contains(listIdentifier))
		return;

	if (depotConfigurations[listIdentifier].MaxAmount == max && depotConfigurations[listIdentifier].MaxType == maxType)
		return;

	depotConfigurations[listIdentifier].MaxAmount = max;
	depotConfigurations[listIdentifier].MaxType = maxType;

	BroadCastNewConfiguration(listIdentifier);
}

bool AAdditionalDepotsServerSubsystem::IsPersistentInSave(FName listIdentifier)
{
	return listIdentifier.IsValid() && persistInSave.Contains(listIdentifier) && persistInSave[listIdentifier];
}

void AAdditionalDepotsServerSubsystem::AddList(TSubclassOf<UAdditionalDepotDefinition> details)
{
	if (!details)
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Warning, "LogAdditionalDepotsServerSubsystem::AddList() - Details class is null!");
		return;
	}

	const UAdditionalDepotDefinition* cdo = details.GetDefaultObject();
	if (!cdo || !cdo->Identifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Warning, "LogAdditionalDepotsServerSubsystem::AddList() - Details class has invalid Identifier!");
		return;
	}

	FAdditionalDepotConfiguration config;
	config.CanBeUsedWhenBuilding = cdo->CanBeUsedWhenBuilding;
	config.CanDragItemsToInventory = cdo->CanDragItemsToInventory;
	config.MaxAmount = cdo->MaxAmount;
	config.MaxType = cdo->MaxType;

	persistInSave.FindOrAdd(cdo->Identifier, cdo->PersistInSaveGame);
	depotConfigurations.FindOrAdd(cdo->Identifier, config);
	uniquePerPlayer.FindOrAdd(cdo->Identifier, cdo->IsPlayerSpecific);
	depotContents.FindOrAdd(cdo->Identifier);
}

int32 AAdditionalDepotsServerSubsystem::AddItemInternal(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount, AFGPlayerState* playerState, bool broadcast)
{
	if (!listIdentifier.IsValid())
		return 0;

	if (amount < 0)
	{
		return RemoveItemInternal(listIdentifier, itemClass, -amount, playerState, broadcast);
	}

	int32 Added;
	{
		FScopeLock lock(&depotLock);

		TMap<FName, FMappedItemAmount>* depotContentsMap = GetDepotContent(listIdentifier, playerState);
		if (!depotContentsMap)
			return 0;

		depotContentsMap->FindOrAdd(listIdentifier).ItemAmounts.FindOrAdd(itemClass);

		int32 maxForDepot;

		if (!depotConfigurations.Contains(listIdentifier) || depotConfigurations[listIdentifier].MaxType == EFAAdditionalDepotsMaxType::None) {
			maxForDepot = INT32_MAX;
		}
		else {
			maxForDepot = depotConfigurations[listIdentifier].MaxAmount;

			if (depotConfigurations[listIdentifier].MaxType == EFAAdditionalDepotsMaxType::Stacks)
				maxForDepot *= UFGItemDescriptor::GetStackSize(itemClass);
		}

		const int64 Current = (*depotContentsMap)[listIdentifier].ItemAmounts[itemClass];
		const int64 Delta = amount;
		const int64 NewValue64 = Current + Delta;
		const int32 Clamped = static_cast<int32>(FMath::Clamp<int64>(NewValue64, 0, maxForDepot));

		(*depotContentsMap)[listIdentifier].ItemAmounts[itemClass] = Clamped;
		Added = static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(Clamped) - Current, 0, INT32_MAX));
	}

	if (Added != 0 && IsInitialized() && broadcast)
		OnItemAdded.Broadcast(listIdentifier, itemClass, Added, playerState);

	return Added;
}

int32 AAdditionalDepotsServerSubsystem::RemoveItemInternal(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount, AFGPlayerState* playerState, bool broadcast)
{
	if (!listIdentifier.IsValid())
		return 0;

	if (amount < 0)
	{
		return AddItemInternal(listIdentifier, itemClass, -amount, playerState, broadcast);
	}

	int32 Removed;
	{
		FScopeLock lock(&depotLock);

		TMap<FName, FMappedItemAmount>* depotContentsMap = GetDepotContent(listIdentifier, playerState);
		if (!depotContentsMap || !depotContentsMap->Contains(listIdentifier))
			return 0;

		FMappedItemAmount* ListContents = depotContentsMap->Find(listIdentifier);
		if (!ListContents)
			return 0;

		int32* ExistingPtr = ListContents->ItemAmounts.Find(itemClass);
		if (!ExistingPtr)
			return 0;

		const int32 Current = *ExistingPtr;
		const int32 NewValue = Current - amount;

		const int32 Clamped = FMath::Clamp<int32>(NewValue, 0, INT32_MAX);
		Removed = FMath::Clamp<int32>(Current - Clamped, 0, INT32_MAX);

		*ExistingPtr = Clamped;
	}

	if (Removed != 0 && IsInitialized() && broadcast)
		OnItemRemoved.Broadcast(listIdentifier, itemClass, Removed, playerState);

	return Removed;
}

TMap<FName, FMappedItemAmount>* AAdditionalDepotsServerSubsystem::GetDepotContent(FName listIdentifier, AFGPlayerState* playerState)
{
	if (!listIdentifier.IsValid())
		return &depotContents;

	bool* isUniquePerPlayer = uniquePerPlayer.Find(listIdentifier);

	if (!isUniquePerPlayer || !(*isUniquePerPlayer))
		return &depotContents;

	if (!playerState)
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Error, "LogAdditionalDepotsServerSubsystem::GetDepotContent(listIdentifier: {0}) - Depot is unique per player but player state is not provided", listIdentifier.ToString());
		return nullptr;
	}

	UAdditionalDepotsPerPlayerDataComponent* perPlayerData = UAdditionalDepotsPerPlayerDataComponent::Get(playerState);
	if (!perPlayerData)
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Fatal, "LogAdditionalDepotsServerSubsystem::GetDepotContent(listIdentifier: {0}) - Depot is unique per player but player player data could not be retrieved", listIdentifier.ToString());
		return nullptr;
	}

	return &perPlayerData->depotContents;
}

void AAdditionalDepotsServerSubsystem::BroadCastNewItemAmounts(FName listIdentifier, TArray<TSubclassOf<UFGItemDescriptor>> itemClasses, AFGPlayerState* playerState)
{
	if (!initialized)
		return;

	TArray<FItemAmount> items;
	{
		FScopeLock lock(&depotLock);

		TMap<FName, FMappedItemAmount>* depotContentsMap = GetDepotContent(listIdentifier, playerState);
		if (!depotContentsMap)
			return;

		for (const TSubclassOf<UFGItemDescriptor>& itemClass : itemClasses)
		{
			if (depotContentsMap->Contains(listIdentifier) && (*depotContentsMap)[listIdentifier].ItemAmounts.Contains(itemClass))
				items.Add(FItemAmount(itemClass, (*depotContentsMap)[listIdentifier].ItemAmounts[itemClass]));
			else
				items.Add(FItemAmount(itemClass, 0));
		}
	}

	OnItemAmountUpdated.Broadcast(listIdentifier, items, playerState);
}

void AAdditionalDepotsServerSubsystem::BroadCastNewConfiguration(FName listIdentifier)
{
	if (!initialized || !depotConfigurations.Contains(listIdentifier))
		return;

	OnConfigurationUpdated.Broadcast(listIdentifier, depotConfigurations[listIdentifier]);
}

void AAdditionalDepotsServerSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Display, "AAdditionalDepotsServerSubsystem::PostLoadGame_Implementation(saveVersion: {0}, gameVersion: {1})", saveVersion, gameVersion);
	
	for (const TPair<FName, bool>& save : persistInSave)
	{
		if (!save.Value)
			depotContents.Remove(save.Key);
	}
}
