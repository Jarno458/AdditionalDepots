#pragma once

#include "CoreMinimal.h"
#include "AdditionalDepotsDataTypes.h"
#include "ItemAmount.h"
#include "Subsystem/ModSubsystem.h"
#include "AdditionalDepotsServerSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdditionalDepotsServerSubsystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FADOnDepotItemsRemoved,
	FName, ListIdentifier,
	TSubclassOf<UFGItemDescriptor>, ItemClass,
	int, Amount
);

USTRUCT()
struct FAdditionalDepotsDepotConfig
{
	GENERATED_BODY()

public:
	FAdditionalDepotsDepotConfig() :
		MaxAmount(0),
		MaxType(EFAAdditionalDepotsMaxType::None),
		CanDragItemsToInventory(false),
		PersistInSaveGame(false)
	{
	}

	FAdditionalDepotsDepotConfig(TSubclassOf<UAdditionalDepotDefinition> details)
	{
		if (details)
		{
			if (const UAdditionalDepotDefinition* cdo = details.GetDefaultObject())
			{
				MaxAmount = cdo->MaxAmount;
				MaxType = cdo->MaxType;
				CanDragItemsToInventory = cdo->CanDragItemsToInventory;
				PersistInSaveGame = cdo->PersistInSaveGame;
			}
		}
	}

	UPROPERTY()
	int32 MaxAmount;

	UPROPERTY()
	EFAAdditionalDepotsMaxType MaxType;

	UPROPERTY()
	bool CanDragItemsToInventory;

	UPROPERTY()
	bool PersistInSaveGame;
};

UCLASS()
class ADDITIONALDEPOTS_API AAdditionalDepotsServerSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AAdditionalDepotsServerSubsystem();

	static AAdditionalDepotsServerSubsystem* Get(UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Additional Depots Serverside Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AAdditionalDepotsServerSubsystem* Get(UObject* worldContext);

protected:
	virtual void PostActorCreated() override;
	virtual void BeginPlay() override;

private:
	TMap<FName, FAdditionalDepotsDepotConfig> depotConfigurations;
	TMap<FName, FMappedItemAmount> depotContents;

	bool initialized;

public:
	FORCEINLINE bool IsInitialized() const { return initialized; }

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set Depot Content", ToolTip = "Sets the content of a depot list (overriding existing content), should only be called on server."))
	void SetDepotContent(FName listIdentifier, TArray<FItemAmount> items);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Item To Depot", ToolTip = "Adds an amount of items to a specific depot list, returns the total items added"))
	int32 AddItem(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Items To Depot", ToolTip = "Adds an amount of items to a specific depot list, returns the total items added"))
	TArray<FItemAmount> AddItems(FName listIdentifier, TArray<FItemAmount> items);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Remove Item To Depot", ToolTip = "Removes an amount of items form a specific depot list, returns the total items removed"))
	int32 RemoveItem(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Remove Items To Depot", ToolTip = "Removes an amount of items from a specific depot list, returns the total items removed"))
	TArray<FItemAmount> RemoveItems(FName listIdentifier, TArray<FItemAmount> items);

	UFUNCTION(BlueprintCallable, DisplayName = "Get All Depot Lists Identifiers")
	TArray<FName> GetListIdentifiers();

	UFUNCTION(BlueprintCallable, DisplayName = "Get All items in Depot List")
	TArray<FItemAmount> GetItems(FName listIdentifier);

	UPROPERTY(BlueprintAssignable, Category = "Additional Depots|Events")
	FADOnDepotItemsRemoved OnItemRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Additional Depots|Events")
	FADOnDepotItemsRemoved OnItemAdded;

private:
	void AddList(TSubclassOf<UAdditionalDepotDefinition> details);
};
