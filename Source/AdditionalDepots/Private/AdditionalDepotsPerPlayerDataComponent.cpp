#include "AdditionalDepotsPerPlayerDataComponent.h"

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
					 
				DepotListPriorities.Add(FAdditionalDepotListPriority{listIdentifier, config.CanBeUsedWhenBuilding});
			}
		}
	}
}

void UAdditionalDepotsPerPlayerDataComponent::SetListPriorities(TArray<FAdditionalDepotListPriority> Array)
{
	//TODO might need locking so we don't update the array when we read it
	DepotListPriorities = Array;
}

TArray<FAdditionalDepotListPriority>& UAdditionalDepotsPerPlayerDataComponent::GetListPriorities()
{
	return DepotListPriorities;
}

void UAdditionalDepotsPerPlayerDataComponent::CopyComponentProperties_Implementation(UActorComponent* intoComponent)
{
	IFGPlayerStateComponentInterface::CopyComponentProperties_Implementation(intoComponent);

	UAdditionalDepotsPerPlayerDataComponent* target = Cast<UAdditionalDepotsPerPlayerDataComponent>(intoComponent);
	if (!IsValid(target))
		return;

	target->DepotListPriorities = DepotListPriorities;
}
