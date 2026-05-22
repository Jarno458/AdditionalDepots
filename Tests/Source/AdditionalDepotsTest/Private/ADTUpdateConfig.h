#pragma once

#include "CoreMinimal.h"

#include "Command/ChatCommandInstance.h"

#include "ADTUpdateConfig.generated.h"

UCLASS()
class ADTUpdateConfig: public AChatCommandInstance
{
	GENERATED_BODY()

public:
	ADTUpdateConfig();

	EExecutionStatus ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) override;
};
