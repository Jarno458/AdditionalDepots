#include "AdditionalDepotsServerSubsystem.h"

#include "AdditionalDepotsDataTypes.h"
#include "AdditionalDepotsReservedIdentifiers.h"
#include "AdditionalDepotsUtils.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsServerSubsystem);

#pragma optimize("", off)

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

void AAdditionalDepotsServerSubsystem::SetDepotContent(FName listIdentifier, TArray<FItemAmount> items)
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

		FMappedItemAmount& depotContent = depotContents.FindOrAdd(listIdentifier);

		depotContent.ItemAmounts.Empty();

		for (const FItemAmount& item : items)
			depotContent.ItemAmounts.Add(item.ItemClass, item.Amount);

		depotContent.ItemAmounts.GenerateKeyArray(updatedItems);
	}

	BroadCastNewItemAmounts(listIdentifier, updatedItems);
}

int32 AAdditionalDepotsServerSubsystem::AddItem(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount)
{
	int32 added = AddItemInternal(listIdentifier, itemClass, amount);

	BroadCastNewItemAmounts(listIdentifier, { itemClass });

	return added;
}

TArray<FItemAmount> AAdditionalDepotsServerSubsystem::AddItems(FName listIdentifier, TArray<FItemAmount> items)
{
	TMap<TSubclassOf<UFGItemDescriptor>, int32> addedItems;

	for (const FItemAmount& Item : items)
	{
		int32 numberOfAdded = AddItem(listIdentifier, Item.ItemClass, Item.Amount);

		int32& addedItem = addedItems.FindOrAdd(Item.ItemClass, 0);
		addedItem += numberOfAdded;
	}

	TArray<TSubclassOf<UFGItemDescriptor>> updatedItems;
	addedItems.GenerateKeyArray(updatedItems);
	BroadCastNewItemAmounts(listIdentifier, updatedItems);

	TArray<FItemAmount> result;
	for (const TPair<TSubclassOf<UFGItemDescriptor>, int32>& pair : addedItems)
		result.Add(FItemAmount(pair.Key, pair.Value));

	return result;
}

int32 AAdditionalDepotsServerSubsystem::RemoveItem(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount)
{
	int32 removed = RemoveItemInternal(listIdentifier, itemClass, amount);

	BroadCastNewItemAmounts(listIdentifier, { itemClass });

	return removed;
}

TArray<FItemAmount> AAdditionalDepotsServerSubsystem::RemoveItems(FName listIdentifier, TArray<FItemAmount> items)
{
	TMap<TSubclassOf<UFGItemDescriptor>, int32> removedItems;

	for (const FItemAmount& Item : items)
	{
		int32 numberOfRemoved = RemoveItem(listIdentifier, Item.ItemClass, Item.Amount);

		int32& removedItem = removedItems.FindOrAdd(Item.ItemClass, 0);
		removedItem += numberOfRemoved;
	}

	TArray<TSubclassOf<UFGItemDescriptor>> updatedItems;
	removedItems.GenerateKeyArray(updatedItems);
	BroadCastNewItemAmounts(listIdentifier, updatedItems);

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

TArray<FItemAmount> AAdditionalDepotsServerSubsystem::GetItems(FName listIdentifier)
{
	TArray<FItemAmount> items;

	if (!listIdentifier.IsValid() || !depotContents.Contains(listIdentifier))
		return items;

	if (listIdentifier == UAdditionalDepotsReservedIdentifiers::GetDimensionalDepotIdentifier())
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Error, "LogAdditionalDepotsServerSubsystem::GetItems({0}) Cannot get contents of dimensional storage, use the AFGCentralStorageSubsystem instead", listIdentifier.ToString());
		return items;
	}

	FScopeLock lock(&depotLock);
	for (const TPair<TSubclassOf<UFGItemDescriptor>, int32>& item : depotContents[listIdentifier].ItemAmounts)
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
	depotContents.FindOrAdd(cdo->Identifier, FMappedItemAmount());
}

int32 AAdditionalDepotsServerSubsystem::AddItemInternal(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount, bool broadcast)
{
	if (!listIdentifier.IsValid())
		return 0;

	int32 Added;
	{
		FScopeLock lock(&depotLock);

		depotContents.FindOrAdd(listIdentifier).ItemAmounts.FindOrAdd(itemClass);

		const int64 Current = depotContents[listIdentifier].ItemAmounts[itemClass];
		const int64 Delta = amount;
		const int64 NewValue64 = Current + Delta;
		const int32 Clamped = static_cast<int32>(FMath::Clamp<int64>(NewValue64, 0, INT32_MAX));

		depotContents[listIdentifier].ItemAmounts[itemClass] = Clamped;
		Added = static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(Clamped) - Current, 0, INT32_MAX));
	}

	if (Added != 0 && IsInitialized() && broadcast)
		OnItemAdded.Broadcast(listIdentifier, itemClass, Added);

	return Added;
}

int32 AAdditionalDepotsServerSubsystem::RemoveItemInternal(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount, bool broadcast)
{
	if (!listIdentifier.IsValid())
		return 0;

	int32 Removed;
	{
		FScopeLock lock(&depotLock);

		FMappedItemAmount* ListContents = depotContents.Find(listIdentifier);
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
		OnItemRemoved.Broadcast(listIdentifier, itemClass, Removed);

	return Removed;
}

void AAdditionalDepotsServerSubsystem::BroadCastNewItemAmounts(FName listIdentifier, TArray<TSubclassOf<UFGItemDescriptor>> itemClasses)
{
	if (!initialized)
		return;

	TArray<FItemAmount> items;
	{
		FScopeLock lock(&depotLock);
		for (const TSubclassOf<UFGItemDescriptor>& itemClass : itemClasses)
		{
			if (depotContents.Contains(listIdentifier) && depotContents[listIdentifier].ItemAmounts.Contains(itemClass))
				items.Add(FItemAmount(itemClass, depotContents[listIdentifier].ItemAmounts[itemClass]));
			else
				items.Add(FItemAmount(itemClass, 0));
		}
	}

	OnItemAmountUpdated.Broadcast(listIdentifier, items);
}

void AAdditionalDepotsServerSubsystem::BroadCastNewConfiguration(FName listIdentifier)
{
	if (!initialized || !depotConfigurations.Contains(listIdentifier))
		return;

	OnConfigurationUpdated.Broadcast(listIdentifier, depotConfigurations[listIdentifier]);
}

void AAdditionalDepotsServerSubsystem::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Display, "AAdditionalDepotsServerSubsystem::PreSaveGame_Implementation(saveVersion: {0}, gameVersion: {1})", saveVersion, gameVersion);

	for (TPair<FName, FMappedItemAmount> DepotContent : depotContents)
	{
		if (persistInSave.Contains(DepotContent.Key) && persistInSave[DepotContent.Key])
		{
			FAAdditionalDepotsSaveableDepotContents Savable;
			Savable.ListIdentifier = DepotContent.Key;
			Savable.Contents = DepotContent.Value;

			saveableDepotContents.Add(Savable);
		}
	}

	for (TPair<FName, FAdditionalDepotConfiguration> DepotConfig : depotConfigurations)
	{
		FAAdditionalDepotsSaveableDepotConfiguration Savable;
		Savable.ListIdentifier = DepotConfig.Key;
		Savable.MaxAmount = DepotConfig.Value.MaxAmount;
		Savable.MaxType = DepotConfig.Value.MaxType;
		Savable.CanBeUsedWhenBuilding = DepotConfig.Value.CanBeUsedWhenBuilding;
		Savable.CanDragItemsToInventory = DepotConfig.Value.CanDragItemsToInventory;

		saveableDepotConfigs.Add(Savable);
	}
}

void AAdditionalDepotsServerSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Display, "AAdditionalDepotsServerSubsystem::PostLoadGame_Implementation(saveVersion: {0}, gameVersion: {1})", saveVersion, gameVersion);

	 if (!saveableDepotContents.IsEmpty())
	 {
		 for (const FAAdditionalDepotsSaveableDepotContents& Savable : saveableDepotContents)
			 depotContents.FindOrAdd(Savable.ListIdentifier) = Savable.Contents;

		 saveableDepotContents.Empty();
	 }

	 if (!saveableDepotConfigs.IsEmpty())
	 {
		 for (const FAAdditionalDepotsSaveableDepotConfiguration& Savable : saveableDepotConfigs)
			 depotConfigurations.FindOrAdd(Savable.ListIdentifier) =
			 FAdditionalDepotConfiguration(Savable.MaxAmount, Savable.MaxType, Savable.CanBeUsedWhenBuilding, Savable.CanDragItemsToInventory);

		 saveableDepotConfigs.Empty();
	 }
}

void AAdditionalDepotsServerSubsystem::PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Display, "AAdditionalDepotsServerSubsystem::PostSaveGame_Implementation(saveVersion: {0}, gameVersion: {1})", saveVersion, gameVersion);
}

#pragma optimize("", on)
