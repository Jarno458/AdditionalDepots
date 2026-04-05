#include "AdditionalDepotsSubsystem.h"

#include "FGCentralStorageSubsystem.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsSubsystem);

#pragma optimize("", off)

AAdditionalDepotsSubsystem::AAdditionalDepotsSubsystem() : Super() {
	PrimaryActorTick.bCanEverTick = false;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;

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

AAdditionalDepotsSubsystem* AAdditionalDepotsSubsystem::Get(UWorld* world) {
	USubsystemActorManager* subsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(subsystemActorManager);

	return subsystemActorManager->GetSubsystemActor<AAdditionalDepotsSubsystem>();
}

AAdditionalDepotsSubsystem* AAdditionalDepotsSubsystem::Get(UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

void AAdditionalDepotsSubsystem::BeginPlay() {
	UE_LOGFMT(LogAdditionalDepotsSubsystem, Display, "AAdditionalDepotsSubsystem::BeginPlay()");
	Super::BeginPlay();

	centralStorageSubsystem = AFGCentralStorageSubsystem::Get(GetWorld());
}

void AAdditionalDepotsSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void AAdditionalDepotsSubsystem::SetActiveList(FName listIdentifier)
{
	if (!listIdentifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsSubsystem, Warning, "AAdditionalDepotsSubsystem::SetActiveList() - Invalid list identifier!");
		return;
	}

	if (!depotLists.Contains(listIdentifier))
	{
		UE_LOGFMT(LogAdditionalDepotsSubsystem, Warning, "AAdditionalDepotsSubsystem::SetActiveList(listIdentifier: {0}) - List identifier not found!", listIdentifier.ToString());
		return;
	}

	activeList = listIdentifier;
}

void AAdditionalDepotsSubsystem::OnCentralStorageConstruct(UListView* ListView)
{
	UE_LOGFMT(LogAdditionalDepotsSubsystem, Display, "AAdditionalDepotsSubsystem::OnCentralStorageConstruct(ListView: {0})", ListView->GetNumItems());
}

void AAdditionalDepotsSubsystem::AddList(const FAAdditionalDepotsListDetails& details)
{
	depotLists.Add(details.Identifier, details);

	/*if (FAAdditionalDepotsListDetails* Details = depotLists.Find(details.Identifier))
	{
		Details->OnSelected.Broadcast(Details->Identifier, *Details);
	}
	*/
}

void AAdditionalDepotsSubsystem::SetDepotContent(FName listIdentifier, TArray<FItemAmount> items)
{
	if (!listIdentifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsSubsystem, Warning, "AAdditionalDepotsSubsystem::SetDepotContent() - Invalid list identifier!");
		return;
	}

	if (!depotLists.Contains(listIdentifier))
	{
		UE_LOGFMT(LogAdditionalDepotsSubsystem, Error, "AAdditionalDepotsSubsystem::SetDepotContent(listIdentifier: {0}) - List identifier not found!", listIdentifier.ToString());
		return;
	}

	if (listIdentifier == GetDimensionalDepotIdentifier())
	{
		UE_LOGFMT(LogAdditionalDepotsSubsystem, Error, "AAdditionalDepotsSubsystem::SetDepotContent({0}) Cannot set contents of dimensional storage, use the AFGCentralStorageSubsystem instead");
		return;
	}

	depotContents.FindOrAdd(listIdentifier);

	depotContents[listIdentifier].ItemAmounts.Empty();

	for (const FItemAmount& item : items)
		depotContents[listIdentifier].ItemAmounts.Add(item.ItemClass, item.Amount);
}

int32 AAdditionalDepotsSubsystem::AddItem(FName listIdentifier, FItemAmount item)
{
	if (!listIdentifier.IsValid())
		return 0;

	depotContents.FindOrAdd(listIdentifier).ItemAmounts.FindOrAdd(item.ItemClass);

	const int64 Current = depotContents[listIdentifier].ItemAmounts[item.ItemClass];
	const int64 Delta = item.Amount;
	const int64 NewValue64 = Current + Delta;
	const int32 Clamped = static_cast<int32>(FMath::Clamp<int64>(NewValue64, 0, INT32_MAX));

	depotContents[listIdentifier].ItemAmounts[item.ItemClass] = Clamped;

	const int32 Added = static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(Clamped) - Current, 0, INT32_MAX));
	return Added;
}

int32 AAdditionalDepotsSubsystem::RemoveItem(FName listIdentifier, FItemAmount item)
{
	if (!listIdentifier.IsValid())
		return 0;

	FMappedItemAmount* ListContents = depotContents.Find(listIdentifier);
	if (!ListContents)
		return 0;

	int32* ExistingPtr = ListContents->ItemAmounts.Find(item.ItemClass);
	if (!ExistingPtr)
		return 0;

	const int32 Current = *ExistingPtr;
	const int32 NewValue = Current - item.Amount;

	const int32 Clamped = FMath::Clamp<int32>(NewValue, 0, INT32_MAX);
	const int32 Removed = FMath::Clamp<int32>(Current - Clamped, 0, INT32_MAX);

	*ExistingPtr = Clamped;

	OnItemsRemoved.Broadcast(listIdentifier, item.ItemClass, Removed);

	return Removed;
}

TArray<FName> AAdditionalDepotsSubsystem::GetListIdentifiers()
{
	TArray<FName> lists;
	depotLists.GenerateKeyArray(lists);
	return lists;
}

TArray<FItemAmount> AAdditionalDepotsSubsystem::GetItems(FName listIdentifier)
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

FAAdditionalDepotsListDetails AAdditionalDepotsSubsystem::GetListDetails(FName listIdentifier)
{
	if (!listIdentifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsSubsystem, Warning, "AAdditionalDepotsSubsystem::GetListDetails() - Invalid list identifier!");
		return FAAdditionalDepotsListDetails();
	}

	if (!depotLists.Contains(listIdentifier))
	{
		UE_LOGFMT(LogAdditionalDepotsSubsystem, Warning, "AAdditionalDepotsSubsystem::GetListDetails(listIdentifier: {0}) - List identifier not found!", listIdentifier.ToString());
		return FAAdditionalDepotsListDetails();
	}

	return depotLists[listIdentifier];
}

FAAdditionalDepotsItemDetails AAdditionalDepotsSubsystem::GetItemDetails(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass)
{
	if (!listIdentifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsSubsystem, Warning, "AAdditionalDepotsSubsystem::GetItemDetails() - Invalid list identifier!");
		return FAAdditionalDepotsItemDetails();
	}

	if (!depotLists.Contains(listIdentifier) || !depotContents.Contains(listIdentifier))
		return FAAdditionalDepotsItemDetails();

	FAAdditionalDepotsListDetails listDetails = depotLists[listIdentifier];
	int32 amount = depotContents[listIdentifier].ItemAmounts[itemClass];

	return FAAdditionalDepotsItemDetails(amount, listDetails.MaxAmount, listDetails.MaxType, listDetails.Color);
}

#pragma optimize("", on)
