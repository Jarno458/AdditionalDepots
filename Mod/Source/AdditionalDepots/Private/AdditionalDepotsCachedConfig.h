#pragma once

#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Kismet/BlueprintFunctionLibrary.h"
#include "AdditionalDepotsCachedConfig.generated.h"

UCLASS()
class UAdditionalDepotsCachedConfig : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

private:
	static bool AbbreviateNumbers;
	static bool CostProgressRelativeToTotal;

public:
	UFUNCTION(BlueprintPure, meta = (ToolTip = "Gets whether numbers should be abbreviated"))
	static FORCEINLINE bool GetAbbreviateNumbers() { return AbbreviateNumbers; }

	UFUNCTION(BlueprintPure, meta = (ToolTip = "Gets whether the cost progress bar should display its sources relative to total rather then used"))
	static FORCEINLINE bool GetProgressBarRelativeToTotal() { return CostProgressRelativeToTotal; }

	static void UpdateFromConfig(UObject* worldContext);
};
