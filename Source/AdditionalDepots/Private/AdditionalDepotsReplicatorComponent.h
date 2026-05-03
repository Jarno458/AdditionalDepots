#pragma once

#include "CoreMinimal.h"
#include "AdditionalDepotsDataTypes.h"
#include "FGItemDescriptor.h"
#include "FGPlayerState.h"
#include "ItemAmount.h"

#include "AdditionalDepotsReplicatorComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdditionalDepotsReplicatorComponent, Log, All);

constexpr uint8 RELIABLE_MESSAGING_CHANNEL_ID_ADDITIONAL_DEPOTS = 83; //random value to avoid collisions with other subsystems

enum class EAdditionalDepotsReplicatorMessageId : uint32
{
	ItemData = 0x01,
	ListConfig = 0x02,
};

struct FReplicatedItemData {

	// Value constructor
	FReplicatedItemData(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount)
		: ListIdentifier(listIdentifier), ItemClass(itemClass), Amount(amount)
	{
	}

	// Default constructor
	FReplicatedItemData() : FReplicatedItemData(NAME_None, nullptr, 0)
	{
	}

public:
	FName ListIdentifier;

	TSubclassOf<UFGItemDescriptor> ItemClass;

	int32 Amount;
};

struct FReplicatedDepotConfigurationData
{
	// Value constructor
	FReplicatedDepotConfigurationData(FName listIdentifier, FAdditionalDepotConfiguration config)
		: ListIdentifier(listIdentifier),
		MaxAmount(config.MaxAmount),
		MaxType(config.MaxType),
		CanDragItemsToInventory(config.CanDragItemsToInventory),
		CanBeUsedWhenBuilding(config.CanBeUsedWhenBuilding)
	{	
	}

	// Default constructor
	FReplicatedDepotConfigurationData() : FReplicatedDepotConfigurationData(NAME_None, FAdditionalDepotConfiguration())
	{
	}

	FName ListIdentifier;

	int32 MaxAmount;

	EFAAdditionalDepotsMaxType MaxType;

	bool CanDragItemsToInventory;

	bool CanBeUsedWhenBuilding;
};

struct FAdditionalDepotsItemReplicationMessage
{
	static constexpr EAdditionalDepotsReplicatorMessageId MessageId = EAdditionalDepotsReplicatorMessageId::ItemData;
	TArray<FReplicatedItemData> ItemData;

	friend FArchive& operator<<(FArchive& Ar, FAdditionalDepotsItemReplicationMessage& Message);
};

struct FAdditionalDepotsConfigReplicationMessage
{
	static constexpr EAdditionalDepotsReplicatorMessageId MessageId = EAdditionalDepotsReplicatorMessageId::ListConfig;
	TArray<FReplicatedDepotConfigurationData> ConfigData;

	friend FArchive& operator<<(FArchive& Ar, FAdditionalDepotsConfigReplicationMessage& Message);
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UAdditionalDepotsReplicatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAdditionalDepotsReplicatorComponent();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void SendInitialReplicationData(const APlayerController* PlayerController) const;

	void OnRawDataReceived(TArray<uint8>&& InMessageData) const;
	void SendRawMessage(const APlayerController* PlayerController, EAdditionalDepotsReplicatorMessageId MessageId, const TFunctionRef<void(FArchive&)>& MessageSerializer) const;

	void ReceiveItemReplicationData(const FAdditionalDepotsItemReplicationMessage& ItemReplicationMessage) const;
	void ReceiveConfigReplicationData(const FAdditionalDepotsConfigReplicationMessage& ConfigReplicationMessage) const;

	UFUNCTION() //for event binding
	void SendUpdatedItemReplicationData(FName ListIdentifier, TArray<FItemAmount> items, AFGPlayerState* playerState);
	UFUNCTION() //for event binding
	void SendUpdatedConfiguration(FName ListIdentifier, FAdditionalDepotConfiguration config);
};

