#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"

#include "ADTPrintPlayerState.generated.h"

UCLASS()
class ADTPrintPlayerState: public AChatCommandInstance
{
	GENERATED_BODY()

public:
	ADTPrintPlayerState();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
