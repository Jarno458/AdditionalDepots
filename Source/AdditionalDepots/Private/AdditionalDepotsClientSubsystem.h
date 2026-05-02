#pragma once

#include "CoreMinimal.h"
#include "AdditionalDepotsDataTypes.h"
#include "FGCentralStorageSubsystem.h"
#include "ItemAmount.h"
#include "FGPlayerState.h"
#include "Subsystem/ModSubsystem.h"

#include "AdditionalDepotsClientSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdditionalDepotsClientSubsystem, Log, All);

USTRUCT(BlueprintType)
struct FAdditionalDepotsListDetailsData
{
	GENERATED_BODY()

public:
	FAdditionalDepotsListDetailsData() : 
		Name(FText::GetEmpty()),
		MaxAmount(0),
		MaxType(EFAAdditionalDepotsMaxType::None),
		Color(FLinearColor::White),
		CanDragItemsToInventory(false),
		Icon(nullptr),
		CanBeUsedWhenBuilding(false)
	{
	}

	FAdditionalDepotsListDetailsData(TSubclassOf<UAdditionalDepotDefinition> details)
	{
		if (details)
		{
			if (const UAdditionalDepotDefinition* cdo = details.GetDefaultObject())
			{
				Name = cdo->Name;
				MaxAmount = cdo->MaxAmount;
				MaxType = cdo->MaxType;
				Color = cdo->Color;
				CanDragItemsToInventory = cdo->CanDragItemsToInventory;
				Icon = cdo->Icon;
				CanBeUsedWhenBuilding = cdo->CanBeUsedWhenBuilding;
			}
		}
	}

public:
	UPROPERTY(BlueprintReadWrite, Category = "AdditionalDepots")
	FText Name;

	UPROPERTY(BlueprintReadWrite, Category = "AdditionalDepots")
	int32 MaxAmount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AdditionalDepots")
	EFAAdditionalDepotsMaxType MaxType = EFAAdditionalDepotsMaxType::None;

	UPROPERTY(BlueprintReadWrite, Category = "AdditionalDepots")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(BlueprintReadWrite, Category = "AdditionalDepots")
	bool CanDragItemsToInventory = false;

	UPROPERTY(BlueprintReadWrite, Category = "AdditionalDepots")
	UTexture2D* Icon = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "AdditionalDepots")
	bool CanBeUsedWhenBuilding = false;
};

USTRUCT(BlueprintType)
struct FAdditionalDepotsColorAmount
{
	GENERATED_BODY()

public:
	FAdditionalDepotsColorAmount() :
		Amount(0),
		Color(FLinearColor::White)
	{
	}

	FAdditionalDepotsColorAmount(int32 amount, FLinearColor color) :
		Amount(amount),
		Color(color)
	{
	}

	UPROPERTY(BlueprintReadOnly, Category = "AdditionalDepots")
	int32 Amount;

	UPROPERTY(BlueprintReadOnly, Category = "AdditionalDepots")
	FLinearColor Color;
};

USTRUCT(BlueprintType)
struct FAdditionalDepotsItemDetails
{
	GENERATED_BODY()

public:
	FAdditionalDepotsItemDetails() :
		Amount(0),
		MaxAmount(0),
		MaxType(EFAAdditionalDepotsMaxType::None),
		Color(FLinearColor::White)
	{
	}

	FAdditionalDepotsItemDetails(TSubclassOf<UFGItemDescriptor> itemClass, int32 amount, int32 maxAmount, EFAAdditionalDepotsMaxType maxType, FLinearColor color) :
		Amount(amount),
		MaxAmount(maxAmount),
		MaxType(maxType),
		Color(color)
	{
	}

	UPROPERTY(BlueprintReadOnly, Category = "AdditionalDepots")
	int32 Amount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "AdditionalDepots")
	int32 MaxAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "AdditionalDepots")
	EFAAdditionalDepotsMaxType MaxType = EFAAdditionalDepotsMaxType::None;

	UPROPERTY(BlueprintReadOnly, Category = "AdditionalDepots")
	FLinearColor Color = FLinearColor::White;
};

UCLASS()
class AAdditionalDepotsClientSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AAdditionalDepotsClientSubsystem();

	static AAdditionalDepotsClientSubsystem* Get(UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Additional Depots Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AAdditionalDepotsClientSubsystem* Get(UObject* worldContext);

protected:
	virtual void PostActorCreated() override;
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	AFGCentralStorageSubsystem* centralStorageSubsystem;

	UPROPERTY() //UPROPERTY() to not loose reference to UTexture2D* Icon;
	TMap<FName, FAdditionalDepotsListDetailsData> depotLists;
	TMap<FName, FMappedItemAmount> depotContents;

	FName activeList;

public:
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Active Depot List", ToolTip = "Get Active Depot List (DimensionalDepot for the Dimensional Depot)"))
	FORCEINLINE FName GetActiveList() const { return activeList; }

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Select current Depot List", ToolTip = "Select current Depot List, use DimensionalDepot for the Dimensional Depot"))
	void SetActiveList(FName listIdentifier);

	UFUNCTION(BlueprintCallable, DisplayName = "Get All Depot Lists Identifiers")
	TArray<FName> GetListIdentifiers();

	UFUNCTION(BlueprintCallable, DisplayName = "Get All items in Depot List")
	TArray<FItemAmount> GetItems(FName listIdentifier);

	UFUNCTION(BlueprintPure)
	FAdditionalDepotsListDetailsData GetListDetails(FName listIdentifier) const;

	UFUNCTION(BlueprintPure)
	FAdditionalDepotsItemDetails GetItemDetails(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass) const;

	UFUNCTION(BlueprintCallable)
	int32 GetTotalAmountStoredAmountForItem(TSubclassOf<UFGItemDescriptor> itemClass);

	UFUNCTION(BlueprintCallable)
	TArray<FAdditionalDepotsColorAmount> GetOrderedRelativeStorages(APlayerState* state, int cost, TSubclassOf<UFGItemDescriptor> itemClass, int32& OutTotalAmount);

public: //internal

	void AddItemData(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount);

	void UpdateConfiguration(FName listIdentifier, const FAdditionalDepotConfiguration& config);

private:
	void AddList(TSubclassOf<UAdditionalDepotDefinition> details);
};
