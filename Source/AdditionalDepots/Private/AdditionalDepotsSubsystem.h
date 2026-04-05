#pragma once

#include "CoreMinimal.h"
#include "FGCentralStorageSubsystem.h"
#include "ItemAmount.h"
#include "Subsystem/ModSubsystem.h"
#include "ListView.h"
#include "AdditionalDepotsSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdditionalDepotsSubsystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FADOnDepotItemsRemoved,
	FName, ListIdentifier,
	TSubclassOf<UFGItemDescriptor>, ItemClass,
	int, Amount
);

UENUM(BlueprintType)
enum class EFAAdditionalDepotsMaxType : uint8 {
	Stacks UMETA(DisplayName = "Number of Stacks"),
	Total UMETA(DisplayName = "Total amout of items"),
	TotalHidden UMETA(DisplayName = "Total amout of items (only used for progress bar)"),
	None UMETA(DisplayName = "None (no progress bar)"),
};

USTRUCT(BlueprintType)
struct FAAdditionalDepotsItemDetails
{
	GENERATED_BODY()

	FAAdditionalDepotsItemDetails() :
		Amount(0), MaxAmount(0), MaxType(EFAAdditionalDepotsMaxType::None), Color(FLinearColor::White)
	{
	}


	FAAdditionalDepotsItemDetails(int32 inAmount, int32 inMaxAmount, EFAAdditionalDepotsMaxType inMaxType, FLinearColor inColor) :
		Amount(inAmount),
		MaxAmount(inMaxAmount),
		MaxType(inMaxType),
		Color(inColor)
	{
	}

	UPROPERTY(BlueprintReadWrite)
	int32 Amount;

	UPROPERTY(BlueprintReadWrite)
	int32 MaxAmount;

	UPROPERTY(BlueprintReadWrite)
	EFAAdditionalDepotsMaxType MaxType;

	UPROPERTY(BlueprintReadWrite)
	FLinearColor Color;
};

USTRUCT(BlueprintType)
struct FAAdditionalDepotsListDetails
{
	GENERATED_BODY()

	FAAdditionalDepotsListDetails() :
		Identifier(NAME_None),
		Name(""),
		MaxAmount(0),
		MaxType(EFAAdditionalDepotsMaxType::None),
		Color(FLinearColor::White),
		PersistInSaveGame(false),
		CanDragItemsToInventory(false),
		Icon(nullptr)
	{
	}

	FAAdditionalDepotsListDetails(FName identifier, FString name, int32 inMaxAmount, EFAAdditionalDepotsMaxType inMaxType, FLinearColor inColor) :
		Identifier(identifier),
		MaxAmount(inMaxAmount),
		MaxType(inMaxType),
		Color(inColor),
		PersistInSaveGame(true),
		CanDragItemsToInventory(true),
		Icon(nullptr)
	{
	}

	UPROPERTY(BlueprintReadWrite)
	FName Identifier;

	UPROPERTY(BlueprintReadWrite)
	FString Name;

	UPROPERTY(BlueprintReadWrite)
	int32 MaxAmount;

	UPROPERTY(BlueprintReadWrite)
	EFAAdditionalDepotsMaxType MaxType;

	UPROPERTY(BlueprintReadWrite)
	FLinearColor Color;

	UPROPERTY(BlueprintReadWrite)
	bool PersistInSaveGame;

	UPROPERTY(BlueprintReadWrite)
	bool CanDragItemsToInventory;

	UPROPERTY(BlueprintReadWrite)
	UTexture2D* Icon;
};

UCLASS()
class AAdditionalDepotsSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AAdditionalDepotsSubsystem();

	static AAdditionalDepotsSubsystem* Get(class UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Additional Depots Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AAdditionalDepotsSubsystem* Get(UObject* worldContext);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	AFGCentralStorageSubsystem* centralStorageSubsystem;

	TMap<FName, FAAdditionalDepotsListDetails> depotLists;
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
	void AddList(const FAAdditionalDepotsListDetails& details);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Depot Content", ToolTip = "Sets the content of a depot list (overriding existing content), should only be called on server."))
	void SetDepotContent(FName listIdentifier, TArray<FItemAmount> items);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Item To Depot", ToolTip = "Adds an amount of items to a specific depot list, returns the total items added"))

	int32 AddItem(FName listIdentifier, FItemAmount item);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Remove Item To Depot", ToolTip = "Removes an amount of items to a specific depot list, returns the total items removed"))

	int32 RemoveItem(FName listIdentifier, FItemAmount item);

	UFUNCTION(BlueprintCallable, DisplayName = "Get All Depot Lists Identifiers")
	TArray<FName> GetListIdentifiers();

	UFUNCTION(BlueprintCallable, DisplayName = "Get All items in Depot List")
	TArray<FItemAmount> GetItems(FName listIdentifier);

	UFUNCTION(BlueprintPure, DisplayName = "Get Details For Depot List")
	FAAdditionalDepotsListDetails GetListDetails(FName listIdentifier);

	UFUNCTION(BlueprintPure, DisplayName = "Get Details For Depot Item")
	FAAdditionalDepotsItemDetails GetItemDetails(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass);

	UFUNCTION(BlueprintCallable)
	void OnCentralStorageConstruct(UListView* ListView);

	UPROPERTY(BlueprintAssignable, Category = "Additional Depots|Events")
	FADOnDepotItemsRemoved OnItemsRemoved;
private:

};
