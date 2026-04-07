#pragma once

#include "CoreMinimal.h"
#include "AdditionalDepotsDataTypes.h"
#include "Engine/DataAsset.h"
#include "AdditionalDepotsUtils.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdditionalDepotsUtils, Log, All);

UCLASS()
class UAdditionalDepotsUtils : public UObject
{
	GENERATED_BODY()

public:
	static TArray<TSubclassOf<UAdditionalDepotsListDetails>> LoadAdditionalDepotLists();
};
