#pragma once

#include "CoreMinimal.h"
#include "AdditionalDepotsServerSubsystem.h"
#include "Command/CommandSender.h"
#include "FGPlayerController.h"

#include "ADTUtils.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdditionalDepotsTestUtils, Log, All);

UCLASS()
class UADTUtils : public UObject
{
	GENERATED_BODY()

public:
	static bool TryGetListIdentifierFromArgument(UCommandSender* Sender, AAdditionalDepotsServerSubsystem* serverSubsystem, const TArray<FString>& Arguments, int32 ArgumentIndex, FName& Identifier);

	static AFGPlayerController* GetPlayerControllerFromArgument(UCommandSender* Sender, const TArray<FString>& Arguments, int32 ArgumentIndex, UObject* WorldContext);

	static bool TryGetItemDescriptorFromArgument(UCommandSender* Sender, const TArray<FString>& Arguments, int32 ArgumentIndex, TSubclassOf<UFGItemDescriptor>& Class);

	static bool TryGetAmountFromArgument(UCommandSender* Sender, const TArray<FString>& Arguments, int32 ArgumentIndex, int32& amount);

	 static bool TryGetBooleanFromArgument(UCommandSender* Sender, const TArray<FString>& Arguments, int32 ArgumentIndex, bool& result);

	static bool TryGetMaxTypeFromArgument(UCommandSender* Sender, const TArray<FString>& Arguments, int32 ArgumentIndex, EFAAdditionalDepotsMaxType& result);
};

