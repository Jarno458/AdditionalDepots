#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"

#include "ADTUpdateMax.generated.h"

UCLASS()
class ADTUpdateMax: public AChatCommandInstance
{
	GENERATED_BODY()

public:
	ADTUpdateMax();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
