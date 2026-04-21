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

#pragma optimize("", off)

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

int32 AAdditionalDepotsClientSubsystem::GetTotalAmountStoredAmountForItem(TSubclassOf<UFGItemDescriptor> itemClass)
{
	TArray<FName> depots = GetListIdentifiers();
	int64 totalAmount = 0;

	totalAmount += centralStorageSubsystem->GetNumItemsFromCentralStorage(itemClass);

	for (const TPair<FName, FAdditionalDepotsListDetailsData>& depot : depotLists)
	{
		if (!depot.Value.CanBeUsedWhenBuilding)
			continue;

		if (depotContents.Contains(depot.Key) && depotContents[depot.Key].ItemAmounts.Contains(itemClass))
		{
			totalAmount += static_cast<int64>(depotContents[depot.Key].ItemAmounts[itemClass]);
		}
	}

	return FMath::Clamp<int64>(totalAmount, 0, static_cast<int64>(MAX_int32));
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
		if (!depot.CanBeUsedWhenBuilding)
			continue;

		if (depot.Identifier == UAdditionalDepotsReservedIdentifiers::GetDimensionalDepotIdentifier())
		{
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
			if (depotContents.Contains(depot.Identifier) && depotContents[depot.Identifier].ItemAmounts.Contains(itemClass))
			{
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
	}

	OutTotalAmount = static_cast<int32>(FMath::Clamp<int64>(totalAmount, 0, static_cast<int64>(MAX_int32)));

	return amounts;
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
	depotLists[listIdentifier].CanDragItemsToInventory = config.CanBeUsedWhenBuilding;
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

#pragma optimize("", on)
