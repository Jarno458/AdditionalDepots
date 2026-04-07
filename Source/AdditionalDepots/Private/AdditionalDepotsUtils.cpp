#include "AdditionalDepotsUtils.h"

#include "Logging/StructuredLog.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsUtils);

#pragma optimize("", off)

TArray<TSubclassOf<UAdditionalDepotsListDetails>> UAdditionalDepotsUtils::LoadAdditionalDepotLists(){
	UE_LOGFMT(LogAdditionalDepotsUtils, Display, "AdditionalDepotsUtils::BeginPlay()");

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	TArray<TSubclassOf<UAdditionalDepotsListDetails>> lists;

	for (const FAssetData& Asset : Assets)
	{
		const UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
		if (!Blueprint)
			continue;

		UClass* Generated = Blueprint->GeneratedClass;
		if (!Generated)
			continue;

		if (!Generated->IsChildOf(UAdditionalDepotsListDetails::StaticClass()))
			continue;

		if (Generated->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
			continue;

		lists.Add(Generated);
	}

	return lists;
}

#pragma optimize("", on)
