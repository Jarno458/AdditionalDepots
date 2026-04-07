#include "AdditionalDepotsServerSubsystem.h"

#include "FGCentralStorageSubsystem.h"
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

	centralStorageSubsystem = AFGCentralStorageSubsystem::Get(GetWorld());
}

void AAdditionalDepotsServerSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void AAdditionalDepotsServerSubsystem::AddList(FName listIdentifier)
{
	//depotLists.Add(details.Identifier, details);
}

void AAdditionalDepotsServerSubsystem::SetDepotContent(FName listIdentifier, TArray<FItemAmount> items)
{
	if (!listIdentifier.IsValid())
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Warning, "LogAdditionalDepotsServerSubsystem::SetDepotContent() - Invalid list identifier!");
		return;
	}

	/*if (!depotLists.Contains(listIdentifier))
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Error, "LogAdditionalDepotsServerSubsystem::SetDepotContent(listIdentifier: {0}) - List identifier not found!", listIdentifier.ToString());
		return;
	}*/

	if (listIdentifier == GetDimensionalDepotIdentifier())
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Error, "LogAdditionalDepotsServerSubsystem::SetDepotContent({0}) Cannot set contents of dimensional storage, use the AFGCentralStorageSubsystem instead");
		return;
	}

	depotContents.FindOrAdd(listIdentifier);

	depotContents[listIdentifier].ItemAmounts.Empty();

	for (const FItemAmount& item : items)
		depotContents[listIdentifier].ItemAmounts.Add(item.ItemClass, item.Amount);
}

int32 AAdditionalDepotsServerSubsystem::AddItem(FName listIdentifier, FItemAmount item)
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

int32 AAdditionalDepotsServerSubsystem::RemoveItem(FName listIdentifier, FItemAmount item)
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

TArray<FName> AAdditionalDepotsServerSubsystem::GetListIdentifiers()
{
	TArray<FName> lists;
	//depotLists.GenerateKeyArray(lists);
	return lists;
}

TArray<FItemAmount> AAdditionalDepotsServerSubsystem::GetItems(FName listIdentifier)
{
	TArray<FItemAmount> items;

	if (!listIdentifier.IsValid() /*|| !depotLists.Contains(listIdentifier)*/)
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

#pragma optimize("", on)
