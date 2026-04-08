#pragma once

#include "CoreMinimal.h"
#include "FGRemoteCallObject.h"
#include "ItemAmount.h"

#include "AdditionalDepotRCO.generated.h"

class FLifetimeProperty;
DECLARE_LOG_CATEGORY_EXTERN(LogAdditionalDepotRCO, Log, All);

UCLASS()
class UAdditionalDepotRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	bool RcoDummy;

	UFUNCTION(BlueprintCallable, Reliable, Server)
	void ServerRemoveItem(FName listIdentifier, FItemAmount itemAmount);

	UFUNCTION(BlueprintCallable, Reliable, Server)
	void ServerAddItem(FName listIdentifier, FItemAmount itemAmount);
};
