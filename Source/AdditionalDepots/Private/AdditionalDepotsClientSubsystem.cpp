#include "AdditionalDepotsClientSubsystem.h"

#include "FGCentralStorageSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsClientSubsystem);

#pragma optimize("", off)

AAdditionalDepotsClientSubsystem::AAdditionalDepotsClientSubsystem() : Super() {
	PrimaryActorTick.bCanEverTick = false;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnClient;

	activeList = GetDimensionalDepotIdentifier();

	FLinearColor DimensionalDepotColor(0.346704f, 0.152926f, 0.346704f);
	FAAdditionalDepotsListDetails dimensionalDepotDetails;
	dimensionalDepotDetails.Identifier = GetDimensionalDepotIdentifier();
	dimensionalDepotDetails.Name = TEXT("Dimensional Depot");
	dimensionalDepotDetails.MaxAmount = 1;
	dimensionalDepotDetails.MaxType = EFAAdditionalDepotsMaxType::Stacks;
	dimensionalDepotDetails.Color = DimensionalDepotColor;
	dimensionalDepotDetails.PersistInSaveGame = true;
	dimensionalDepotDetails.CanDragItemsToInventory = true;

	depotLists.Add(dimensionalDepotDetails.Identifier, dimensionalDepotDetails);
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

void AAdditionalDepotsClientSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
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

void AAdditionalDepotsClientSubsystem::AddList(const FAAdditionalDepotsListDetails& details)
{
	depotLists.Add(details.Identifier, details);
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

	if (!listIdentifier.IsValid() || !depotLists.Contains(listIdentifier))
		return items;

	if (listIdentifier == GetDimensionalDepotIdentifier())
	{
		centralStorageSubsystem->GetAllItemsFromCentralStorage(items);
	}
	else
	{
		for (const TPair<TSubclassOf<UFGItemDescriptor>, int32>& item : depotContents[listIdentifier].ItemAmounts)
			items.Add(FItemAmount(item.Key, item.Value));
	}

	return items;
}

FAAdditionalDepotsListDetails AAdditionalDepotsClientSubsystem::GetListDetails(FName listIdentifier)
{
	if (!listIdentifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::GetListDetails() - Invalid list identifier!");
		return FAAdditionalDepotsListDetails();
	}

	if (!depotLists.Contains(listIdentifier))
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::GetListDetails(listIdentifier: {0}) - List identifier not found!", listIdentifier.ToString());
		return FAAdditionalDepotsListDetails();
	}

	return depotLists[listIdentifier];
}

FAAdditionalDepotsItemDetails AAdditionalDepotsClientSubsystem::GetItemDetails(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass)
{
	if (!listIdentifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::GetItemDetails() - Invalid list identifier!");
		return FAAdditionalDepotsItemDetails();
	}

	if (!depotLists.Contains(listIdentifier) || !depotContents.Contains(listIdentifier))
		return FAAdditionalDepotsItemDetails();

	FAAdditionalDepotsListDetails listDetails = depotLists[listIdentifier];
	int32 amount = depotContents[listIdentifier].ItemAmounts[itemClass];

	return FAAdditionalDepotsItemDetails(amount, listDetails.MaxAmount, listDetails.MaxType, listDetails.Color);
}

#pragma optimize("", on)
