#pragma once

#include "CoreMinimal.h"
#include "AdditionalDepotsDataTypes.h"
#include "SubsystemActorManager.h"
#include "Engine/DataAsset.h"
#include "AdditionalDepotsUtils.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdditionalDepotsUtils, Log, All);

UCLASS()
class UAdditionalDepotsUtils : public UObject
{
	GENERATED_BODY()

public:
	static TArray<TSubclassOf<UAdditionalDepotDefinition>> LoadAdditionalDepotLists();

	template<typename T>
	static T* GetSubsystemActorIncludingParentClasses(UWorld* world) {
		USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
		fgcheck(SubsystemActorManager);

		for (const TPair<TSubclassOf<AModSubsystem>, AModSubsystem*>& subsystemEntry : SubsystemActorManager->GetSubsystemActors()) {
			if (subsystemEntry.Key->IsChildOf(T::StaticClass()))
				return Cast<T>(subsystemEntry.Value);
		}

		return nullptr;
	}
};
