#include "AdditionalDepotsPerPlayerDataComponent.h"

#include "StructuredLog.h"
#include "UnrealNetwork.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsPerPlayerDataComponent);

#pragma optimize("", off)

UAdditionalDepotsPerPlayerDataComponent::UAdditionalDepotsPerPlayerDataComponent() {
	UE_LOG(LogAdditionalDepotsPerPlayerDataComponent, Display, TEXT("UAdditionalDepotsPerPlayerDataComponent::UAdditionalDepotsPerPlayerDataComponent()"));

	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicated(true);
}

void UAdditionalDepotsPerPlayerDataComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAdditionalDepotsPerPlayerDataComponent, DepotListsIdentifiers);
}

void UAdditionalDepotsPerPlayerDataComponent::BeginPlay() {
	Super::BeginPlay();

	UE_LOGFMT(LogAdditionalDepotsPerPlayerDataComponent, Display, "UAdditionalDepotsPerPlayerDataComponent::BeginPlay()");

	const APlayerState* state = CastChecked<APlayerState>(GetOwner());

	if (!IsValid(state))
		return;
}

void UAdditionalDepotsPerPlayerDataComponent::SetListPriorities(TArray<FAdditionalDepotListPriority> Array)
{
	DepotListsIdentifiers = Array;
}

void UAdditionalDepotsPerPlayerDataComponent::CopyComponentProperties_Implementation(UActorComponent* intoComponent)
{
	IFGPlayerStateComponentInterface::CopyComponentProperties_Implementation(intoComponent);

	UAdditionalDepotsPerPlayerDataComponent* target = CastChecked<UAdditionalDepotsPerPlayerDataComponent>(intoComponent);

	target->DepotListsIdentifiers = DepotListsIdentifiers;

}

#pragma optimize("", on)
