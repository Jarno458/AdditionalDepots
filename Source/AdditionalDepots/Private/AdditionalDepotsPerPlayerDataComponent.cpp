#include "AdditionalDepotsPerPlayerDataComponent.h"

#include "AdditionalDepotRCO.h"
#include "AdditionalDepotsReservedIdentifiers.h"
#include "AdditionalDepotsServerSubsystem.h"
#include "FGPlayerController.h"
#include "StructuredLog.h"
#include "UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsPerPlayerDataComponent)

UAdditionalDepotsPerPlayerDataComponent::UAdditionalDepotsPerPlayerDataComponent() {
	UE_LOG(LogAdditionalDepotsPerPlayerDataComponent, Display, TEXT("UAdditionalDepotsPerPlayerDataComponent::UAdditionalDepotsPerPlayerDataComponent()"));

	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

UAdditionalDepotsPerPlayerDataComponent* UAdditionalDepotsPerPlayerDataComponent::Get(AFGPlayerState* playerState)
{
	if (!IsValid(playerState))
	{
		UE_LOG(LogAdditionalDepotsPerPlayerDataComponent, Warning, TEXT("UAdditionalDepotsPerPlayerDataComponent::Get() - Invalid player state!"));
		return nullptr;
	}

	return playerState->FindComponentByClass<UAdditionalDepotsPerPlayerDataComponent>();
}

void UAdditionalDepotsPerPlayerDataComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAdditionalDepotsPerPlayerDataComponent, DepotListPriorities);
}

void UAdditionalDepotsPerPlayerDataComponent::BeginPlay() {
	Super::BeginPlay();

	UE_LOGFMT(LogAdditionalDepotsPerPlayerDataComponent, Display, "UAdditionalDepotsPerPlayerDataComponent::BeginPlay()");

	AFGPlayerState* playerState = Cast<AFGPlayerState>(GetOwner());
	if (!IsValid(playerState) || !playerState->HasAuthority())
		return;

	AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());

	if (!IsValid(serverSubsystem))
	{
		UE_LOG(LogAdditionalDepotsPerPlayerDataComponent, Error, TEXT("Failed to get server subsystem"));
		return;
	}

	TMap<FName, FAdditionalDepotListPriority> loadedEntries;
	for (FAdditionalDepotListPriority& DepotListPriority : DepotListPriorities)
		loadedEntries.Add(DepotListPriority.Identifier, DepotListPriority);

	// we always rebuild it based on the server subsystem data, so it correctly reflects when certain mods are added or deleted
	DepotListPriorities.Empty();

	if (loadedEntries.Contains(UAdditionalDepotsReservedIdentifiers::GetPlayerInventoryDepotIdentifier()))
		DepotListPriorities.Add(loadedEntries[UAdditionalDepotsReservedIdentifiers::GetPlayerInventoryDepotIdentifier()]);
	else
		DepotListPriorities.Add(FAdditionalDepotListPriority{ UAdditionalDepotsReservedIdentifiers::GetPlayerInventoryDepotIdentifier(), true });

	TArray<FName> currentListIdentifiers = serverSubsystem->GetListIdentifiers();

	for (const FName& listIdentifier : currentListIdentifiers)
	{
		if (loadedEntries.Contains(listIdentifier))
		{
			DepotListPriorities.Add(loadedEntries[listIdentifier]);
		}
		else
		{
			FAdditionalDepotConfiguration config = serverSubsystem->GetConfiguration(listIdentifier);

			DepotListPriorities.Add(FAdditionalDepotListPriority{ listIdentifier, config.CanBeUsedWhenBuilding });

		}
	}

	TArray<FName> contentKeys;
	depotContents.GenerateKeyArray(contentKeys);

	for (const FName& contentKey : contentKeys)
	{
		if (!currentListIdentifiers.Contains(contentKey))
		{
			depotContents.Remove(contentKey);
		}
	}
}

void UAdditionalDepotsPerPlayerDataComponent::SetListPriorities(TArray<FAdditionalDepotListPriority> Array)
{
	DepotListPriorities = Array;

	AFGPlayerState* playerState = Cast<AFGPlayerState>(GetOwner());

	if (!IsValid(playerState))
		return;

	AFGPlayerController* controller = playerState->GetOwningController();
	if (!IsValid(controller))
		return;

	if (!controller->HasAuthority())
	{
		UAdditionalDepotRCO* rco = Cast<UAdditionalDepotRCO>(controller->GetRemoteCallObjectOfClass(UAdditionalDepotRCO::StaticClass()));
		if (!IsValid(rco))
		{
			UE_LOG(LogAdditionalDepotsPerPlayerDataComponent, Error, TEXT("Failed to get UAdditionalDepotRCO"));
			return;
		}

		rco->ServerSetDepotPriority(Array);
	}
}

TArray<FAdditionalDepotListPriority>& UAdditionalDepotsPerPlayerDataComponent::GetListPriorities()
{
	return DepotListPriorities;
}

void UAdditionalDepotsPerPlayerDataComponent::CopyComponentProperties_Implementation(UActorComponent* intoComponent)
{
	UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Display, "LogAdditionalDepotsServerSubsystem::CopyComponentProperties_Implementation()");

	IFGPlayerStateComponentInterface::CopyComponentProperties_Implementation(intoComponent);

	UAdditionalDepotsPerPlayerDataComponent* target = Cast<UAdditionalDepotsPerPlayerDataComponent>(intoComponent);
	if (!IsValid(target))
		return;

	target->DepotListPriorities = DepotListPriorities;
	target->depotContents = depotContents;
}
