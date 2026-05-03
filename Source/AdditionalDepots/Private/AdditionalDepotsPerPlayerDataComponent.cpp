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

	if (DepotListPriorities.IsEmpty())
	{
		UE_LOG(LogAdditionalDepotsPerPlayerDataComponent, Display, TEXT("DepotListPriorities initializing"));

		AFGPlayerState* playerState = Cast<AFGPlayerState>(GetOwner());
		AFGPlayerController* playerController = playerState->GetOwningController();

		if (IsValid(playerController) && playerController->HasAuthority())
		{
			AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());

			if (!IsValid(serverSubsystem))
			{
				UE_LOG(LogAdditionalDepotsPerPlayerDataComponent, Error, TEXT("Failed to get server subsystem"));
				return;
			}

			DepotListPriorities.Add(FAdditionalDepotListPriority{ UAdditionalDepotsReservedIdentifiers::GetPlayerInventoryDepotIdentifier(), true });

			for (const FName& listIdentifier : serverSubsystem->GetListIdentifiers())
			{
				FAdditionalDepotConfiguration config = serverSubsystem->GetConfiguration(listIdentifier);

				DepotListPriorities.Add(FAdditionalDepotListPriority{ listIdentifier, config.CanBeUsedWhenBuilding });
			}
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
			UE_LOG(LogAdditionalDepotsPerPlayerDataComponent, Error, TEXT("Failed to get RCO"));
			return;
		}

		rco->ServerSetDepotPriority(playerState, Array);
	}
}

TArray<FAdditionalDepotListPriority>& UAdditionalDepotsPerPlayerDataComponent::GetListPriorities()
{
	return DepotListPriorities;
}

void UAdditionalDepotsPerPlayerDataComponent::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Display, "UAdditionalDepotsPerPlayerDataComponent::PreSaveGame_Implementation(saveVersion: {0}, gameVersion: {1})", saveVersion, gameVersion);

	AFGPlayerState* playerState = Cast<AFGPlayerState>(GetOwner());
	if (!IsValid(playerState))
		return;

	AFGPlayerController* controller = playerState->GetOwningController();
	if (!IsValid(controller) || !controller->HasAuthority())
		return;

	AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());

	for (TPair<FName, FMappedItemAmount> DepotContent : depotContents)
	{
		if (serverSubsystem->IsPersistentInSave(DepotContent.Key))
		{
			FAAdditionalDepotsSaveableDepotContents Savable;
			Savable.ListIdentifier = DepotContent.Key;
			Savable.Contents = DepotContent.Value;

			saveableDepotContents.Add(Savable);
		}
	}
}

void UAdditionalDepotsPerPlayerDataComponent::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion)
{
	UE_LOGFMT(LogAdditionalDepotsServerSubsystem, Display, "UAdditionalDepotsPerPlayerDataComponent::PostLoadGame_Implementation(saveVersion: {0}, gameVersion: {1})", saveVersion, gameVersion);

	AFGPlayerState* playerState = Cast<AFGPlayerState>(GetOwner());
	if (!IsValid(playerState))
		return;

	AFGPlayerController* controller = playerState->GetOwningController();
	if (!IsValid(controller) || !controller->HasAuthority())
		return;

	if (!saveableDepotContents.IsEmpty())
	{
		for (const FAAdditionalDepotsSaveableDepotContents& Savable : saveableDepotContents)
			depotContents.FindOrAdd(Savable.ListIdentifier) = Savable.Contents;

		saveableDepotContents.Empty();
	}
}

void UAdditionalDepotsPerPlayerDataComponent::CopyComponentProperties_Implementation(UActorComponent* intoComponent)
{
	IFGPlayerStateComponentInterface::CopyComponentProperties_Implementation(intoComponent);

	UAdditionalDepotsPerPlayerDataComponent* target = Cast<UAdditionalDepotsPerPlayerDataComponent>(intoComponent);
	if (!IsValid(target))
		return;

	target->DepotListPriorities = DepotListPriorities;
	target->saveableDepotContents = saveableDepotContents;
}
