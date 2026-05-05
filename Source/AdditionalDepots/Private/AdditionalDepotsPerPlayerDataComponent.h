#pragma once

#include "CoreMinimal.h"
#include "AdditionalDepotsDataTypes.h"
#include "FGPlayerState.h"
#include "FGPlayerStateComponentInterface.h"
#include "FGSaveInterface.h"
#include "AdditionalDepotsPerPlayerDataComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdditionalDepotsPerPlayerDataComponent, Log, All);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UAdditionalDepotsPerPlayerDataComponent : public UActorComponent, public IFGSaveInterface, public IFGPlayerStateComponentInterface
{
	GENERATED_BODY()

	// Begin IFGSaveInterface
	virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override {};
	virtual void GatherDependencies_Implementation(TArray<UObject*>& out_dependentObjects) override {};
	virtual bool NeedTransform_Implementation() override { return false; };
	virtual bool ShouldSave_Implementation() const override { return true; };

	// End IFSaveInterface
	virtual void CopyComponentProperties_Implementation(UActorComponent* intoComponent) override;

public:
	UAdditionalDepotsPerPlayerDataComponent();

	UFUNCTION(BlueprintPure, DisplayName = "Get Additional Depots Per Player Data")
	static UAdditionalDepotsPerPlayerDataComponent* Get(AFGPlayerState* playerState);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;

private:
	UPROPERTY(Replicated, SaveGame)
	TArray<FAdditionalDepotListPriority> DepotListPriorities;

public:
	UPROPERTY(SaveGame)
	TMap<FName, FMappedItemAmount> depotContents;

public:
	UFUNCTION(BlueprintCallable, Category = "AdditionalDepots")
	void SetListPriorities(TArray<FAdditionalDepotListPriority> Array);

	UFUNCTION(BlueprintCallable, Category = "AdditionalDepots")
	TArray<FAdditionalDepotListPriority>& GetListPriorities();
};

