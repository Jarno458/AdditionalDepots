#pragma once
#include "CoreMinimal.h"
#include "Configuration/ConfigManager.h"
#include "Engine/Engine.h"
#include "AdditionalDepotConfigStruct.generated.h"

class UConfigManager;
/* Struct generated from Mod Configuration Asset '/AdditionalDepots/AdditionalDepotConfig' */
USTRUCT(BlueprintType)
struct FAdditionalDepotConfigStruct {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    bool AbbreviateNumbers{};

    UPROPERTY(BlueprintReadWrite)
    bool CostProgressRelativeToTotal{};

    /* Retrieves active configuration value and returns object of this struct containing it */
    static FAdditionalDepotConfigStruct GetActiveConfig(UObject* WorldContext) {
        FAdditionalDepotConfigStruct ConfigStruct{};
        FConfigId ConfigId{"AdditionalDepots", ""};
        if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull)) {
            UConfigManager* ConfigManager = World->GetGameInstance()->GetSubsystem<UConfigManager>();
            ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FAdditionalDepotConfigStruct::StaticStruct(), &ConfigStruct});
        }
        return ConfigStruct;
    }
};

