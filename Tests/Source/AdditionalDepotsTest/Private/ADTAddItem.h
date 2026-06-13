#pragma once

#include "CoreMinimal.h"
#include "Resources/FGItemDescriptor.h"
#include "FGPlayerController.h"

#include "Command/ChatCommandInstance.h"

#include "ADTAddItem.generated.h"

USTRUCT()
struct FIntervalItem
{
	GENERATED_BODY()

	FDateTime NextTimestamp;

	int intervalInSeconds;

	FName listIdentifier;

	TSubclassOf<UFGItemDescriptor> Item;

	int32 Amount;

	int32 IntervalCount;

	UPROPERTY()
	AFGPlayerState* PlayerState;
};

UCLASS()
class ADTAddItem: public AChatCommandInstance
{
	GENERATED_BODY()

private:
	 TArray<FIntervalItem> IntervalItems;

public:
	ADTAddItem();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;

	void Tick(float DeltaSeconds) override;
};
