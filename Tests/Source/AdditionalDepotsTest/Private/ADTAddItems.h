#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"

#include "ADTAddItems.generated.h"

UCLASS()
class ADTAddItems: public AChatCommandInstance
{
	GENERATED_BODY()

public:
	ADTAddItems();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
