#pragma once

#include "CoreMinimal.h"
#include "FGCentralStorageSubsystem.h"
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
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	AFGCentralStorageSubsystem* centralStorageSubsystem;

	TMap<FName, FMappedItemAmount> depotContents;

public:
	UFUNCTION(BlueprintPure, DisplayName = "Get Depot Identifier for Dimensional Storage")
	static FORCEINLINE FName GetDimensionalDepotIdentifier()
	{
		return "DimensionalDepot";
	}

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Depot List", ToolTip = "Adds a new depot list definition to the subsystem, should be called on each client and server."))
	void AddList(FName listIdentifier);

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

	UPROPERTY(BlueprintAssignable, Category = "Additional Depots|Events")
	FADOnDepotItemsRemoved OnItemsRemoved;
private:

};
