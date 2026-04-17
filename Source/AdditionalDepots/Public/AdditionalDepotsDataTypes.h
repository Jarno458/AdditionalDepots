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

USTRUCT(BlueprintType)
struct ADDITIONALDEPOTS_API FAdditionalDepotConfiguration
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Configuration")
	int32 MaxAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Configuration")
	EFAAdditionalDepotsMaxType MaxType = EFAAdditionalDepotsMaxType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Configuration")
	bool CanDragItemsToInventory = true;

	UPROPERTY(BlueprintReadOnly, Category = "Configuration")
	bool CanBeUsedWhenBuilding = true;
};

UCLASS(Abstract, BlueprintType)
class ADDITIONALDEPOTS_API UAdditionalDepotDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identifier")
	FName Identifier = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	FText Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
	int32 MaxAmount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
	EFAAdditionalDepotsMaxType MaxType = EFAAdditionalDepotsMaxType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
	bool CanDragItemsToInventory = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration")
	bool CanBeUsedWhenBuilding = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Savegame")
	bool PersistInSaveGame = true;
};

USTRUCT(BlueprintType)
struct ADDITIONALDEPOTS_API FAdditionalDepotListPriority
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, SaveGame)
	FName Identifier = NAME_None;

	UPROPERTY(BlueprintReadOnly, SaveGame)
	bool CanBeUsedWhenBuilding = true;
};