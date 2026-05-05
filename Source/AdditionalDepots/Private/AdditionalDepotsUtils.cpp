#include "AdditionalDepotsUtils.h"

#include "Logging/StructuredLog.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsUtils);

TArray<TSubclassOf<UAdditionalDepotDefinition>> UAdditionalDepotsUtils::LoadAdditionalDepotLists() {
	UE_LOGFMT(LogAdditionalDepotsUtils, Display, "AdditionalDepotsUtils::BeginPlay()");

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	const FString NativeParentClassValue = FString::Printf(TEXT("/Script/CoreUObject.Class'%s'"), *UAdditionalDepotDefinition::StaticClass()->GetPathName());
	//const FString className(TEXT("AdditionalDepotsListDetails"));

	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.TagsAndValues.Add(TEXT("NativeParentClass"), NativeParentClassValue);
	//Filter.TagsAndValues.Add("PrimaryAssetType", className); //hardcoded alternative

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	TArray<TSubclassOf<UAdditionalDepotDefinition>> lists;

	for (const FAssetData& Asset : Assets)
	{
		FString GeneratedClassTag;
		if (!Asset.GetTagValue(TEXT("GeneratedClass"), GeneratedClassTag))
			continue;

		const FSoftObjectPath GeneratedClassPath = FSoftObjectPath(GeneratedClassTag);
		UClass* Generated = Cast<UClass>(GeneratedClassPath.TryLoad());
		if (!Generated)
			continue;

		if (!Generated->IsChildOf(UAdditionalDepotDefinition::StaticClass()))
			continue;

		if (Generated->HasAnyClassFlags(CLASS_Abstract))
			continue;

		lists.Add(Generated);
	}

	return lists;
}
