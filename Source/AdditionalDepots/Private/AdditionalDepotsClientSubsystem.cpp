#include "AdditionalDepotsClientSubsystem.h"

#include "AdditionalDepotsDataTypes.h"
#include "AdditionalDepotsPerPlayerDataComponent.h"
#include "AdditionalDepotsReservedIdentifiers.h"
#include "AdditionalDepotsUtils.h"
#include "FGCentralStorageSubsystem.h"
#include "FGPlayerController.h"
#include "FGPlayerState.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsClientSubsystem);

AAdditionalDepotsClientSubsystem::AAdditionalDepotsClientSubsystem() : Super() {
	PrimaryActorTick.bCanEverTick = false;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnClient;

	activeList = UAdditionalDepotsReservedIdentifiers::GetDimensionalDepotIdentifier();
}

AAdditionalDepotsClientSubsystem* AAdditionalDepotsClientSubsystem::Get(UWorld* world) {
	USubsystemActorManager* subsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(subsystemActorManager);

	return subsystemActorManager->GetSubsystemActor<AAdditionalDepotsClientSubsystem>();
}

AAdditionalDepotsClientSubsystem* AAdditionalDepotsClientSubsystem::Get(UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

void AAdditionalDepotsClientSubsystem::BeginPlay() {
	UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Display, "AAdditionalDepotsClientSubsystem::BeginPlay()");
	Super::BeginPlay();

	centralStorageSubsystem = AFGCentralStorageSubsystem::Get(GetWorld());
}

void AAdditionalDepotsClientSubsystem::PostActorCreated()
{
	UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Display, "AAdditionalDepotsClientSubsystem::PostActorCreated()");
	Super::PostActorCreated();

	const TArray<TSubclassOf<UAdditionalDepotDefinition>> lists = UAdditionalDepotsUtils::LoadAdditionalDepotLists();
	for (const TSubclassOf<UAdditionalDepotDefinition>& list : lists)
		AddList(list);
}

void AAdditionalDepotsClientSubsystem::SetActiveList(FName listIdentifier)
{
	if (!listIdentifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::SetActiveList() - Invalid list identifier!");
		return;
	}

	if (!depotLists.Contains(listIdentifier))
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::SetActiveList(listIdentifier: {0}) - List identifier not found!", listIdentifier.ToString());
		return;
	}

	activeList = listIdentifier;
}

TArray<FName> AAdditionalDepotsClientSubsystem::GetListIdentifiers()
{
	TArray<FName> lists;
	depotLists.GenerateKeyArray(lists);
	return lists;
}

TArray<FName> AAdditionalDepotsClientSubsystem::GetNonEmptyListIdentifiers()
{
	TArray<FName> lists;

	for (const TPair<FName, FMappedItemAmount>& depot : depotContents)
	{
		if (depot.Value.ItemAmounts.Num() > 0)
		{
			lists.Add(depot.Key);
		}
	}

	TArray<FItemAmount> items;
	centralStorageSubsystem->GetAllItemsFromCentralStorage(items);

	if (items.Num() > 0)
		lists.Add(UAdditionalDepotsReservedIdentifiers::GetDimensionalDepotIdentifier());

	return lists;
}

TArray<FItemAmount> AAdditionalDepotsClientSubsystem::GetItems(FName listIdentifier)
{
	TArray<FItemAmount> items;

	if (!listIdentifier.IsValid())
		return items;

	if (listIdentifier == UAdditionalDepotsReservedIdentifiers::GetDimensionalDepotIdentifier())
	{
		centralStorageSubsystem->GetAllItemsFromCentralStorage(items);
		return items;
	}

	if (!depotContents.Contains(listIdentifier))
	{
		return items;
	}

	for (const TPair<TSubclassOf<UFGItemDescriptor>, int32>& item : depotContents[listIdentifier].ItemAmounts)
		items.Add(FItemAmount(item.Key, item.Value));

	return items;
}

FAdditionalDepotsListDetailsData AAdditionalDepotsClientSubsystem::GetListDetails(FName listIdentifier) const
{
	if (!listIdentifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::GetListDetails() - Invalid list identifier!");
		return FAdditionalDepotsListDetailsData();
	}

	if (!depotLists.Contains(listIdentifier))
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::GetListDetails(listIdentifier: {0}) - List identifier not found!", listIdentifier.ToString());
		return FAdditionalDepotsListDetailsData();
	}

	return depotLists[listIdentifier];
}

FAdditionalDepotsItemDetails AAdditionalDepotsClientSubsystem::GetItemDetails(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass) const
{
	if (!listIdentifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::GetItemDetails() - Invalid list identifier!");
		return FAdditionalDepotsItemDetails();
	}

	if (!depotLists.Contains(listIdentifier) || !depotContents.Contains(listIdentifier))
		return FAdditionalDepotsItemDetails();

	FAdditionalDepotsListDetailsData listDetails = depotLists[listIdentifier];

	int32 amount = depotContents[listIdentifier].ItemAmounts.Contains(itemClass) ? depotContents[listIdentifier].ItemAmounts[itemClass] : 0;

	return FAdditionalDepotsItemDetails(itemClass, amount, listDetails.MaxAmount, listDetails.MaxType, listDetails.Color);
}

bool AAdditionalDepotsClientSubsystem::HasAnyAvailableForBuildingForItem(APlayerState* state, TSubclassOf<UFGItemDescriptor> itemClass)
{
	UAdditionalDepotsPerPlayerDataComponent* playerData = Cast<UAdditionalDepotsPerPlayerDataComponent>(state->GetComponentByClass(UAdditionalDepotsPerPlayerDataComponent::StaticClass()));
	if (!playerData)
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Error, "AAdditionalDepotsClientSubsystem::HasAnyAvailableForBuildingForItem() - PlayerState does not have UAdditionalDepotsPerPlayerDataComponent!");
		return false;
	}

	for (const FAdditionalDepotListPriority& depot : playerData->GetListPriorities())
	{
		if (!depot.CanBeUsedWhenBuilding) // player specific setting
			continue;

		if (depot.Identifier == UAdditionalDepotsReservedIdentifiers::GetDimensionalDepotIdentifier())
		{
			if (!depotLists.Contains(depot.Identifier) || !depotLists[depot.Identifier].CanBeUsedWhenBuilding) //server configuration
				continue;

			int32 centralStorageAmount = centralStorageSubsystem->GetNumItemsFromCentralStorage(itemClass);

			if (centralStorageAmount > 0)
				return true;
		}
		else if (depot.Identifier == UAdditionalDepotsReservedIdentifiers::GetPlayerInventoryDepotIdentifier())
		{
			AFGPlayerState* playerState = Cast<AFGPlayerState>(state);
			AFGPlayerController* playerController = playerState->GetOwningController();
			AFGCharacterPlayer* playerCharacter = Cast<AFGCharacterPlayer>(playerController->GetControlledCharacter());

			if (!playerCharacter)
			{
				UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Error, "AAdditionalDepotsClientSubsystem::HasAnyAvailableForBuildingForItem() - Controlled character not found!");
				return false;
			}

			int32 amountInInventory = playerCharacter->GetInventory()->GetNumItems(itemClass);
			if (amountInInventory > 0)
				return true;
		}
		else
		{
			if (!depotLists.Contains(depot.Identifier) || !depotLists[depot.Identifier].CanBeUsedWhenBuilding //server configuration
				|| !depotContents.Contains(depot.Identifier) || !depotContents[depot.Identifier].ItemAmounts.Contains(itemClass))
				continue;

			int32 depotAmount = depotContents[depot.Identifier].ItemAmounts[itemClass];
			if (depotAmount > 0)
				return true;
		}
	}
	
	return false;
}

TArray<FAdditionalDepotsColorAmount> AAdditionalDepotsClientSubsystem::GetOrderedRelativeStorages(APlayerState* state, int cost, TSubclassOf<UFGItemDescriptor> itemClass, int32& OutTotalAmount)
{
	static constexpr FLinearColor inventoryColor = FLinearColor(0.783538f, 0.291771f, 0.057805f);

	UAdditionalDepotsPerPlayerDataComponent* playerData = Cast<UAdditionalDepotsPerPlayerDataComponent>(state->GetComponentByClass(UAdditionalDepotsPerPlayerDataComponent::StaticClass()));
	if (!playerData)
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Error, "AAdditionalDepotsClientSubsystem::GetOrderedRelativeStorages() - PlayerState does not have UAdditionalDepotsPerPlayerDataComponent!");
		return TArray<FAdditionalDepotsColorAmount>();
	}

	TArray<FAdditionalDepotsColorAmount> amounts;
	int64 totalAmount = 0;
	int remainingCost = cost;

	for (const FAdditionalDepotListPriority& depot : playerData->GetListPriorities())
	{
		if (!depot.CanBeUsedWhenBuilding) // player specific setting
			continue;

		if (depot.Identifier == UAdditionalDepotsReservedIdentifiers::GetDimensionalDepotIdentifier())
		{
			if (!depotLists.Contains(depot.Identifier) || !depotLists[depot.Identifier].CanBeUsedWhenBuilding) //server configuration
				continue;

			int32 centralStorageAmount = centralStorageSubsystem->GetNumItemsFromCentralStorage(itemClass);
			totalAmount += centralStorageAmount;

			if (centralStorageAmount > 0)
			{
				remainingCost -= centralStorageAmount;
				if (remainingCost < 0)
				{
					centralStorageAmount += remainingCost;
					remainingCost = 0;
				}

				amounts.Add(FAdditionalDepotsColorAmount(centralStorageAmount, depotLists[depot.Identifier].Color));
			}
		}
		else if (depot.Identifier == UAdditionalDepotsReservedIdentifiers::GetPlayerInventoryDepotIdentifier())
		{
			AFGPlayerState* playerState = Cast<AFGPlayerState>(state);
			AFGPlayerController* playerController = playerState->GetOwningController();
			AFGCharacterPlayer* playerCharacter = Cast<AFGCharacterPlayer>(playerController->GetControlledCharacter());

			if (!playerCharacter)
			{
				UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Error, "AAdditionalDepotsClientSubsystem::GetOrderedRelativeStorages() - Controlled character not found!");
				return TArray<FAdditionalDepotsColorAmount>();
			}

			int32 amountInInventory = playerCharacter->GetInventory()->GetNumItems(itemClass);

			totalAmount += amountInInventory;

			if (amountInInventory > 0)
			{
				remainingCost -= amountInInventory;
				if (remainingCost < 0)
				{
					amountInInventory += remainingCost;
					remainingCost = 0;
				}

				amounts.Add(FAdditionalDepotsColorAmount(amountInInventory, inventoryColor));
			}
		}
		else
		{
			if (!depotLists.Contains(depot.Identifier) || !depotLists[depot.Identifier].CanBeUsedWhenBuilding //server configuration
				|| !depotContents.Contains(depot.Identifier) || !depotContents[depot.Identifier].ItemAmounts.Contains(itemClass))
				continue;

			int32 depotAmount = depotContents[depot.Identifier].ItemAmounts[itemClass];

			totalAmount += depotAmount;

			remainingCost -= depotAmount;
			if (remainingCost < 0)
			{
				depotAmount += remainingCost;
				remainingCost = 0;
			}

			amounts.Add(FAdditionalDepotsColorAmount(depotAmount, depotLists[depot.Identifier].Color));
		}
	}

	OutTotalAmount = static_cast<int32>(FMath::Clamp<int64>(totalAmount, 0, static_cast<int64>(MAX_int32)));

	return amounts;
}

int32 AAdditionalDepotsClientSubsystem::GetAmountForBuildingForItem(const UFGInventoryComponent* inventory, TSubclassOf<UFGItemDescriptor> itemClass, const AFGPlayerState* playerState)
{
	int64 amount = 0;

	 if (!IsValid(playerState))
	 {
		 playerState = UAdditionalDepotsUtils::TryGetPlayerStateFromInventory(inventory);

		 if (!playerState)
		 {
			 UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Error, "AAdditionalDepotsClientSubsystem::GetAmountForBuildingForItem() - PlayerState not found!");
			 return 0;
		 }
	 }

	UAdditionalDepotsPerPlayerDataComponent* playerData = Cast<UAdditionalDepotsPerPlayerDataComponent>(playerState->GetComponentByClass(UAdditionalDepotsPerPlayerDataComponent::StaticClass()));
	if (!playerData)
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Error, "AAdditionalDepotsClientSubsystem::GetAmountForBuildingForItem() - PlayerState does not have UAdditionalDepotsPerPlayerDataComponent!");
		return 0;
	}

	for (const FAdditionalDepotListPriority& depot : playerData->GetListPriorities())
	{
		if (!depot.CanBeUsedWhenBuilding) // player specific setting
			continue;

		if (depot.Identifier == UAdditionalDepotsReservedIdentifiers::GetDimensionalDepotIdentifier())
		{
			if (!depotLists.Contains(depot.Identifier) || !depotLists[depot.Identifier].CanBeUsedWhenBuilding) //server configuration
				continue;

			amount += centralStorageSubsystem->GetNumItemsFromCentralStorage(itemClass);
		}
		else if (depot.Identifier == UAdditionalDepotsReservedIdentifiers::GetPlayerInventoryDepotIdentifier())
		{
			amount += inventory->GetNumItems(itemClass);
		}
		else
		{
			if (!depotLists.Contains(depot.Identifier) || !depotLists[depot.Identifier].CanBeUsedWhenBuilding //server configuration
				|| !depotContents.Contains(depot.Identifier) || !depotContents[depot.Identifier].ItemAmounts.Contains(itemClass))
				continue;

			amount += depotContents[depot.Identifier].ItemAmounts[itemClass];
		}
	}

	return  static_cast<int32>(FMath::Clamp<int64>(amount, 0, INT32_MAX));
}

int32 AAdditionalDepotsClientSubsystem::GetAmountForBuildingInDepotsForItem(const UFGInventoryComponent* inventory, TSubclassOf<UFGItemDescriptor> itemClass, const AFGPlayerState* playerState)
{
	int64 amount = 0;

	if (!IsValid(playerState))
	{
		playerState = UAdditionalDepotsUtils::TryGetPlayerStateFromInventory(inventory);

		if (!playerState)
		{
			UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Error, "AAdditionalDepotsClientSubsystem::GetAmountForBuildingForItem() - PlayerState not found!");
			return 0;
		}
	}

	UAdditionalDepotsPerPlayerDataComponent* playerData = Cast<UAdditionalDepotsPerPlayerDataComponent>(playerState->GetComponentByClass(UAdditionalDepotsPerPlayerDataComponent::StaticClass()));
	if (!playerData)
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Error, "AAdditionalDepotsClientSubsystem::GetAmountForBuildingForItem() - PlayerState does not have UAdditionalDepotsPerPlayerDataComponent!");
		return 0;
	}

	for (const FAdditionalDepotListPriority& depot : playerData->GetListPriorities())
	{
		if (!depot.CanBeUsedWhenBuilding) // player specific setting
			continue;

		if (depot.Identifier == UAdditionalDepotsReservedIdentifiers::GetDimensionalDepotIdentifier())
		{
			if (!depotLists.Contains(depot.Identifier) || !depotLists[depot.Identifier].CanBeUsedWhenBuilding) //server configuration
				continue;

			amount += centralStorageSubsystem->GetNumItemsFromCentralStorage(itemClass);
		}
		else if (depot.Identifier == UAdditionalDepotsReservedIdentifiers::GetPlayerInventoryDepotIdentifier())
		{
			//not taken into account for player inventory
		}
		else
		{
			if (!depotLists.Contains(depot.Identifier) || !depotLists[depot.Identifier].CanBeUsedWhenBuilding //server configuration
				|| !depotContents.Contains(depot.Identifier) || !depotContents[depot.Identifier].ItemAmounts.Contains(itemClass))
				continue;

			amount += depotContents[depot.Identifier].ItemAmounts[itemClass];
		}
	}

	return  static_cast<int32>(FMath::Clamp<int64>(amount, 0, INT32_MAX));
}

void AAdditionalDepotsClientSubsystem::AddItemData(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount)
{
	if (!listIdentifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::AddItemData() - Invalid list identifier!");
		return;
	}

	depotContents.FindOrAdd(listIdentifier).ItemAmounts.FindOrAdd(itemClass);
	depotContents[listIdentifier].ItemAmounts[itemClass] = amount;
}

void AAdditionalDepotsClientSubsystem::UpdateConfiguration(FName listIdentifier, const FAdditionalDepotConfiguration& config)
{
	if (!listIdentifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::UpdateConfiguration() - Invalid list identifier!");
		return;
	}

	if (!depotLists.Contains(listIdentifier))
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::UpdateConfiguration(listIdentifier: {0}) - List identifier not found!", listIdentifier.ToString());
		return;
	}

	depotLists[listIdentifier].MaxAmount = config.MaxAmount;
	depotLists[listIdentifier].MaxType = config.MaxType;
	depotLists[listIdentifier].CanDragItemsToInventory = config.CanDragItemsToInventory;
	depotLists[listIdentifier].CanBeUsedWhenBuilding = config.CanBeUsedWhenBuilding;
}

void AAdditionalDepotsClientSubsystem::AddList(TSubclassOf<UAdditionalDepotDefinition> details)
{
	if (!details)
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::AddList() - Details class is null!");
		return;
	}

	const UAdditionalDepotDefinition* cdo = details.GetDefaultObject();
	if (!cdo || !cdo->Identifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::AddList() - Details class has invalid Identifier!");
		return;
	}

	depotLists.Add(cdo->Identifier, FAdditionalDepotsListDetailsData(details));

	auto x = 20;
}
