#include "AdditionalDepotsServerSubsystem.h"

#include "AdditionalDepotsDataTypes.h"
#include "AdditionalDepotsUtils.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Logging/StructuredLog.h"
#include "AdditionalDepotsReplicatorSubsystem.h"

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

	if (listIdentifier == UAdditionalDepotsUtils::GetDimensionalDepotIdentifier())
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Error, "LogAdditionalDepotsServerSubsystem::SetDepotContent({0}) Cannot set contents of dimensional storage, use the AFGCentralStorageSubsystem instead");
		return;
	}

	depotContents.FindOrAdd(listIdentifier);

	depotContents[listIdentifier].ItemAmounts.Empty();

	for (const FItemAmount& item : items)
		depotContents[listIdentifier].ItemAmounts.Add(item.ItemClass, item.Amount);
}

int32 AAdditionalDepotsServerSubsystem::AddItem(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount)
{
	if (!listIdentifier.IsValid())
		return 0;

	depotContents.FindOrAdd(listIdentifier).ItemAmounts.FindOrAdd(itemClass);

	const int64 Current = depotContents[listIdentifier].ItemAmounts[itemClass];
	const int64 Delta = amount;
	const int64 NewValue64 = Current + Delta;
	const int32 Clamped = static_cast<int32>(FMath::Clamp<int64>(NewValue64, 0, INT32_MAX));

	depotContents[listIdentifier].ItemAmounts[itemClass] = Clamped;
	const int32 Added = static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(Clamped) - Current, 0, INT32_MAX));

	if (Added != 0)
		OnItemAdded.Broadcast(listIdentifier, itemClass, Added);

	return Added;
}

TArray<FItemAmount> AAdditionalDepotsServerSubsystem::AddItems(FName listIdentifier, TArray<FItemAmount> items)
{
	//TODO should only cause 1 replication
	for (const FItemAmount& Item : items)
		AddItem(listIdentifier, Item.ItemClass, Item.Amount);

	return TArray<FItemAmount>(); //TODO fix me
}

int32 AAdditionalDepotsServerSubsystem::RemoveItem(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount)
{
	if (!listIdentifier.IsValid())
		return 0;

	FMappedItemAmount* ListContents = depotContents.Find(listIdentifier);
	if (!ListContents)
		return 0;

	int32* ExistingPtr = ListContents->ItemAmounts.Find(itemClass);
	if (!ExistingPtr)
		return 0;

	const int32 Current = *ExistingPtr;
	const int32 NewValue = Current - amount;

	const int32 Clamped = FMath::Clamp<int32>(NewValue, 0, INT32_MAX);
	const int32 Removed = FMath::Clamp<int32>(Current - Clamped, 0, INT32_MAX);

	*ExistingPtr = Clamped;

	if (Removed != 0)
		OnItemRemoved.Broadcast(listIdentifier, itemClass, Removed);

	return Removed;
}

TArray<FItemAmount> AAdditionalDepotsServerSubsystem::RemoveItems(FName listIdentifier, TArray<FItemAmount> items)
{
	//TODO should only cause 1 replication
	for (const FItemAmount& Item : items)
		RemoveItem(listIdentifier, Item.ItemClass, Item.Amount);

	return TArray<FItemAmount>(); //TODO fix me
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

	if (listIdentifier == UAdditionalDepotsUtils::GetDimensionalDepotIdentifier())
	{
		UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Error, "LogAdditionalDepotsServerSubsystem::GetItems({0}) Cannot get contents of dimensional storage, use the AFGCentralStorageSubsystem instead");
		return items;
	}

	for (const TPair<TSubclassOf<UFGItemDescriptor>, int32>& item : depotContents[listIdentifier].ItemAmounts)
		items.Add(FItemAmount(item.Key, item.Value));

	return items;
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

	depotConfigurations.FindOrAdd(cdo->Identifier, FAdditionalDepotsDepotConfig(details));
}

#pragma optimize("", on)