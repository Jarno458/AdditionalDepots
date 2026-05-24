#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"

#include "ADTSetItem.generated.h"

UCLASS()
class ADTSetItem: public AChatCommandInstance
{
	GENERATED_BODY()

public:
	ADTSetItem();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
