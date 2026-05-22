#include "AdditionalDepotsUtils.h"

#include "Logging/StructuredLog.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "FGPlayerController.h"
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

AFGPlayerState* UAdditionalDepotsUtils::TryGetPlayerStateFromInventory(const UFGInventoryComponent* inventory)
{
	if (!IsValid(inventory))
		return nullptr;

	AFGPlayerState* directState = inventory->GetOwningPlayerState();
	if (IsValid(directState))
		return directState;

	APawn* ownerPawn = Cast<APawn>(inventory->GetOwner());
	if (!IsValid(ownerPawn))
		return TryGetPlayerStateBasedOnController(inventory);

	AFGPlayerState* pawnOwnedState = ownerPawn->GetPlayerState<AFGPlayerState>();
	if (IsValid(pawnOwnedState))
		return pawnOwnedState;

	return TryGetPlayerStateBasedOnController(inventory);
}

AFGPlayerState* UAdditionalDepotsUtils::TryGetPlayerStateBasedOnController(const UFGInventoryComponent* inventory)
{
	for (TPlayerControllerIterator<AFGPlayerController>::ServerAll playerController(inventory->GetWorld()); playerController; ++playerController) {
		if (!IsValid(*playerController))
			continue;

		AFGCharacterPlayer* character = Cast<AFGCharacterPlayer>(playerController->GetCharacter());
		if (!IsValid(character))
			continue;

		if (character->GetInventory() == inventory)
			return playerController->GetPlayerState<AFGPlayerState>();
	}

	return nullptr;
}
