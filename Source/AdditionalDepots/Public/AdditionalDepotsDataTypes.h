#pragma once

#include "Engine/DataAsset.h"

#include "AdditionalDepotsDataTypes.generated.h"

UENUM(BlueprintType)
enum class EFAAdditionalDepotsMaxType : uint8 {
	Stacks UMETA(DisplayName = "Number of Stacks"),
	Total UMETA(DisplayName = "Total amout of items"),
	TotalHidden UMETA(DisplayName = "Total amout of items (only used for progress bar)"),
	None UMETA(DisplayName = "None (no progress bar)"),
};

UCLASS(Abstract, BlueprintType)
class ADDITIONALDEPOTS_API UAdditionalDepotDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Details")
	FName Identifier = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Details")
	FString Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Max Capacity")
	int32 MaxAmount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Max Capacity")
	EFAAdditionalDepotsMaxType MaxType = EFAAdditionalDepotsMaxType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Server Details")
	bool PersistInSaveGame = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Behaviour")
	bool CanDragItemsToInventory = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	UTexture2D* Icon = nullptr;
};