#pragma once

#include "CoreMinimal.h"
#include "AdditionalDepotsDataTypes.h"
#include "FGCentralStorageSubsystem.h"
#include "ItemAmount.h"
#include "Engine/DataAsset.h"
#include "Subsystem/ModSubsystem.h"
#include "AdditionalDepotsClientSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdditionalDepotsClientSubsystem, Log, All);

USTRUCT(BlueprintType)
struct FAdditionalDepotsListDetailsData
{
	GENERATED_BODY()

public:
	FAdditionalDepotsListDetailsData() : 
		Name(""),
		MaxAmount(0),
		MaxType(EFAAdditionalDepotsMaxType::None),
		Color(FLinearColor::White),
		CanDragItemsToInventory(false),
		Icon(nullptr)
	{
	}

	FAdditionalDepotsListDetailsData(TSubclassOf<UAdditionalDepotsListDetails> details)
	{
		if (details)
		{
			if (const UAdditionalDepotsListDetails* cdo = details.GetDefaultObject())
			{
				Name = cdo->Name;
				MaxAmount = cdo->MaxAmount;
				MaxType = cdo->MaxType;
				Color = cdo->Color;
				CanDragItemsToInventory = cdo->CanDragItemsToInventory;
				Icon = cdo->Icon;
			}
		}
	}

public:
	UPROPERTY(BlueprintReadOnly, Category = "AdditionalDepots")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "AdditionalDepots")
	int32 MaxAmount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "AdditionalDepots")
	EFAAdditionalDepotsMaxType MaxType = EFAAdditionalDepotsMaxType::None;

	UPROPERTY(BlueprintReadOnly, Category = "AdditionalDepots")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "AdditionalDepots")
	bool CanDragItemsToInventory = false;

	UPROPERTY(BlueprintReadOnly, Category = "AdditionalDepots")
	UTexture2D* Icon = nullptr;
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
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	AFGCentralStorageSubsystem* centralStorageSubsystem;

	TMap<FName, FAdditionalDepotsListDetailsData> depotLists;
	TMap<FName, FMappedItemAmount> depotContents;

	FName activeList;

public:
	UFUNCTION(BlueprintPure, DisplayName = "Get Depot Identifier for Dimensional Storage")
	static FORCEINLINE FName GetDimensionalDepotIdentifier()
	{
		return "DimensionalDepot";
	}

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get Active Depot List", ToolTip = "Get Active Depot List (DimensionalDepot for the Dimensional Depot)"))
	FORCEINLINE FName GetActiveList() const { return activeList; }

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Select current Depot List", ToolTip = "Select current Depot List, use DimensionalDepot for the Dimensional Depot"))
	void SetActiveList(FName listIdentifier);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Depot List", ToolTip = "Adds a new depot list definition to the subsystem, should be called on each client and server."))
	void AddList(TSubclassOf<UAdditionalDepotsListDetails> details);

	UFUNCTION(BlueprintCallable, DisplayName = "Get All Depot Lists Identifiers")
	TArray<FName> GetListIdentifiers();

	UFUNCTION(BlueprintCallable, DisplayName = "Get All items in Depot List")
	TArray<FItemAmount> GetItems(FName listIdentifier);

	UFUNCTION(BlueprintPure, DisplayName = "Get Depot List Details")
	FAdditionalDepotsListDetailsData GetListDetails(FName listIdentifier) const;

	UFUNCTION(BlueprintPure, DisplayName = "Get Depot Item Details")
	FAdditionalDepotsItemDetails GetItemDetails(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass) const;

private:

};
