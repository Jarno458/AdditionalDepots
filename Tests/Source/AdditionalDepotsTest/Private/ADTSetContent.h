#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"
#include "Resources/FGItemDescriptor.h"

#include "ADTSetContent.generated.h"

USTRUCT(BlueprintType)
struct FItemAmountMap
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Depot Contents")
	TMap<TSubclassOf<UFGItemDescriptor >, int32> ItemAmounts;
};


UCLASS()
class ADTSetContent: public AChatCommandInstance
{
	GENERATED_BODY()

public: 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Depot Contents")
	TArray<FItemAmountMap> ContentPresets;

public:
	ADTSetContent();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
