#include "AdditionalDepotsUtils.h"

#include "Logging/StructuredLog.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "UObject/SoftObjectPath.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsUtils);

#pragma optimize("", off)

TArray<TSubclassOf<UAdditionalDepotsListDetails>> UAdditionalDepotsUtils::LoadAdditionalDepotLists(){
	UE_LOGFMT(LogAdditionalDepotsUtils, Display, "AdditionalDepotsUtils::BeginPlay()");

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	const FString NativeParentClassValue = FString::Printf(TEXT("/Script/CoreUObject.Class'%s'"), *UAdditionalDepotsListDetails::StaticClass()->GetPathName());

	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.TagsAndValues.Add(TEXT("NativeParentClass"), NativeParentClassValue);

	//hardcoded alternative
	//FString className(TEXT("AdditionalDepotsListDetails"));
	//Filter.TagsAndValues.Add("PrimaryAssetType", className);

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	TArray<TSubclassOf<UAdditionalDepotsListDetails>> lists;

	for (const FAssetData& Asset : Assets)
	{
		FString GeneratedClassTag;
		if (!Asset.GetTagValue(TEXT("GeneratedClass"), GeneratedClassTag))
			continue;

		const FSoftObjectPath GeneratedClassPath = FSoftObjectPath(GeneratedClassTag);
		UClass* Generated = Cast<UClass>(GeneratedClassPath.TryLoad());
		if (!Generated)
			continue;

		if (!Generated->IsChildOf(UAdditionalDepotsListDetails::StaticClass()))
			continue;

		if (Generated->HasAnyClassFlags(CLASS_Abstract))
			continue;

		lists.Add(Generated);
	}

	return lists;
}

#pragma optimize("", on)
