#pragma once

#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Kismet/BlueprintFunctionLibrary.h"
#include "AdditionalDepotsReservedIdentifiers.generated.h"


UCLASS()
class ADDITIONALDEPOTS_API UAdditionalDepotsReservedIdentifiers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, meta = (ToolTip = "Gets the reserved Additional Depot Identifier for the Dimensional Storage"))
	static FORCEINLINE FName GetDimensionalDepotIdentifier() { return FName(TEXT("DimensionalDepot")); }

	UFUNCTION(BlueprintPure, meta = (ToolTip = "Gets the reserved Additional Depot Identifier for the local players inventory"))
	static FORCEINLINE FName GetPlayerInventoryDepotIdentifier() { return FName(TEXT("PlayerInventory")); }
};
