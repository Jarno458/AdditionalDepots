#include "AdditionalDepotRCO.h"

#include "AdditionalDepotsPerPlayerDataComponent.h"
#include "AdditionalDepotsServerSubsystem.h"
#include "FGPlayerController.h"
#include "Logging/StructuredLog.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotRCO);

void UAdditionalDepotRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UAdditionalDepotRCO, RcoDummy);
}

void UAdditionalDepotRCO::ServerSetDepotPriority_Implementation(const TArray<FAdditionalDepotListPriority>& listPriorities)
{
	UE_LOGFMT(LogAdditionalDepotRCO, Display, "RCO Set new list priorities for player {0}", listPriorities.Num());

	 AFGPlayerController* controller = GetOwnerPlayerController();
	 if (!IsValid(controller))
		 return;

	 AFGPlayerState* playerState = controller->GetPlayerState<AFGPlayerState>();
	 if (!IsValid(playerState))
		 return; 

	 UAdditionalDepotsPerPlayerDataComponent* component = Cast<UAdditionalDepotsPerPlayerDataComponent>(playerState->GetComponentByClass(UAdditionalDepotsPerPlayerDataComponent::StaticClass()));

	 component->SetListPriorities(listPriorities);
}

void UAdditionalDepotRCO::ServerRemoveItem_Implementation(FName listIdentifier, FItemAmount itemAmount) {
	UE_LOGFMT(LogAdditionalDepotRCO, Display, "RCO Take Item from Server: {0}, item: {1}", listIdentifier.ToString(), itemAmount.Amount);

	AFGPlayerController* controller = GetOwnerPlayerController();
	if (!IsValid(controller))
		return;

	AFGPlayerState* playerState = controller->GetPlayerState<AFGPlayerState>();
	if (!IsValid(playerState))
		return;

	AAdditionalDepotsServerSubsystem::Get(GetWorld())->RemoveItem(listIdentifier, itemAmount.ItemClass, itemAmount.Amount, playerState);
}

void UAdditionalDepotRCO::ServerAddItem_Implementation(FName listIdentifier, FItemAmount itemAmount) {
	UE_LOGFMT(LogAdditionalDepotRCO, Display, "RCO Add Item to Server: {0}, item: {1}", listIdentifier.ToString(), itemAmount.Amount);

	AFGPlayerController* controller = GetOwnerPlayerController();
	if (!IsValid(controller))
		return;

	AFGPlayerState* playerState = controller->GetPlayerState<AFGPlayerState>();
	if (!IsValid(playerState))
		return;

	AAdditionalDepotsServerSubsystem::Get(GetWorld())->AddItem(listIdentifier, itemAmount.ItemClass, itemAmount.Amount, playerState);
}

void UAdditionalDepotRCO::ServerTryMoveItemToInventory_Implementation(FName listIdentifier, FItemAmount itemAmount, UFGInventoryComponent* inventory, int inventoryIndex)
{
	UE_LOGFMT(LogAdditionalDepotRCO, Display, "RCO Add Item to Server: {0}, item: {1}", listIdentifier.ToString(), itemAmount.Amount);

	AFGPlayerController* controller = GetOwnerPlayerController();
	if (!IsValid(controller))
		return;

	AFGPlayerState* playerState = controller->GetPlayerState<AFGPlayerState>();
	if (!IsValid(playerState))
		return;

	AAdditionalDepotsServerSubsystem* additionalDepotsServerSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());
	if (!IsValid(additionalDepotsServerSubsystem))	{
		UE_LOGFMT(LogAdditionalDepotRCO, Error, "RCO ServerTryMoveItemToInventory - Could not get server subsystem!");
		return;
	}

	int32 removed = additionalDepotsServerSubsystem->RemoveItem(listIdentifier, itemAmount.ItemClass, itemAmount.Amount, playerState);

	FInventoryStack stack(removed, itemAmount.ItemClass);

	int32 added = inventory->AddStackToIndex(inventoryIndex, stack, true);

	if (added < removed)
		additionalDepotsServerSubsystem->AddItem(listIdentifier, itemAmount.ItemClass, removed - added, playerState);
}
