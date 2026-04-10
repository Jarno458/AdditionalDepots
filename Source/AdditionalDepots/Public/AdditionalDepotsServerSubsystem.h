#pragma once

#include "CoreMinimal.h"
#include "AdditionalDepotsDataTypes.h"
#include "FGSaveInterface.h"
#include "ItemAmount.h"
#include "Subsystem/ModSubsystem.h"
#include "AdditionalDepotsServerSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdditionalDepotsServerSubsystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FADOnDepotItemsAddedOrRemoved,
	FName, ListIdentifier,
	TSubclassOf<UFGItemDescriptor>, ItemClass,
	int, Amount
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FADOnDepotItemAmountUpdated,
	FName, ListIdentifier,
	TArray<FItemAmount>, Items
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FADOnDepotConfigurationUpdated,
	FName, ListIdentifier,
	FAdditionalDepotConfiguration, Configuration
);

USTRUCT()
struct FAAdditionalDepotsSaveableDepotContents
{
	GENERATED_BODY()

public:
	UPROPERTY(SaveGame)
	FName ListIdentifier;

	UPROPERTY(SaveGame)
	FMappedItemAmount Contents;
};


UCLASS()
class ADDITIONALDEPOTS_API AAdditionalDepotsServerSubsystem : public AModSubsystem, public IFGSaveInterface
{
	GENERATED_BODY()

	// Begin IFGSaveInterface
	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;
	virtual void GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects) override {};
	virtual bool NeedTransform_Implementation() override { return false; };
	virtual bool ShouldSave_Implementation() const override { return true; };
	// End IFSaveInterface

public:
	AAdditionalDepotsServerSubsystem();

	static AAdditionalDepotsServerSubsystem* Get(UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Get Additional Depots Serverside Subsystem", Meta = (DefaultToSelf = "worldContext"))
	static AAdditionalDepotsServerSubsystem* Get(UObject* worldContext);

protected:
	virtual void PostActorCreated() override;
	virtual void BeginPlay() override;

private:
	TMap<FName, FAdditionalDepotConfiguration> depotConfigurations;
	TMap<FName, FMappedItemAmount> depotContents;
	TMap<FName, bool> persistInSave;

	FCriticalSection depotLock;

	bool initialized;

	TArray<FAAdditionalDepotsSaveableDepotContents> saveableDepotContents;

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

	UFUNCTION(BlueprintCallable, DisplayName = "Get All configuration for Depot List")
	FAdditionalDepotConfiguration GetConfiguration(FName listIdentifier);

	UFUNCTION(BlueprintCallable, DisplayName = "Update Can Drag Items To Inventory")
	void UpdateCanDragToInventory(FName listIdentifier, bool canDrag);

	UFUNCTION(BlueprintCallable, DisplayName = "Update Can Be Used For Building And Crafting")
	void UpdateCanBeUsedForBuildingAndCrafting(FName listIdentifier, bool canBeUsed);

	UFUNCTION(BlueprintCallable, DisplayName = "Update Max Amount")
	void UpdateMaxAmount(FName listIdentifier, EFAAdditionalDepotsMaxType maxType, int max);

	UPROPERTY(BlueprintAssignable, Category = "Additional Depots|Events")
	FADOnDepotItemsAddedOrRemoved OnItemRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Additional Depots|Events")
	FADOnDepotItemsAddedOrRemoved OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Additional Depots|Events")
	FADOnDepotItemAmountUpdated OnItemAmountUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Additional Depots|Events")
	FADOnDepotConfigurationUpdated OnConfigurationUpdated;

private:
	void AddList(TSubclassOf<UAdditionalDepotDefinition> details);

	int32 AddItemInternal(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount, bool broadcast = true);
	int32 RemoveItemInternal(FName listIdentifier, TSubclassOf<UFGItemDescriptor> itemClass, int32 amount, bool broadcast = true);


	void BroadCastNewItemAmounts(FName listIdentifier, TArray<TSubclassOf<UFGItemDescriptor>> itemClasses);
	void BroadCastNewConfiguration(FName listIdentifier);
};
