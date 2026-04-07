#include "AdditionalDepotsClientSubsystem.h"

#include "AdditionalDepotsDataTypes.h"
#include "AdditionalDepotsUtils.h"
#include "FGCentralStorageSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsClientSubsystem);

#pragma optimize("", off)

AAdditionalDepotsClientSubsystem::AAdditionalDepotsClientSubsystem() : Super() {
	PrimaryActorTick.bCanEverTick = false;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnClient;

	activeList = GetDimensionalDepotIdentifier();
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

	const TArray<TSubclassOf<UAdditionalDepotsListDetails>> lists = UAdditionalDepotsUtils::LoadAdditionalDepotLists();
	for (const TSubclassOf<UAdditionalDepotsListDetails>& list : lists)
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

void AAdditionalDepotsClientSubsystem::AddList(TSubclassOf<UAdditionalDepotsListDetails> details)
{
	if (!details)
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::AddList() - Details class is null!");
		return;
	}

	const UAdditionalDepotsListDetails* cdo = details.GetDefaultObject();
	if (!cdo || !cdo->Identifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsClientSubsystem, Warning, "AAdditionalDepotsClientSubsystem::AddList() - Details class has invalid Identifier!");
		return;
	}

	depotLists.Add(cdo->Identifier, FAdditionalDepotsListDetailsData(details));
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

#pragma optimize("", on)
