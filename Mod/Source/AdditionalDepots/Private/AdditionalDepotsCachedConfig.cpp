#include "AdditionalDepotsCachedConfig.h"

#include "AdditionalDepotConfigStruct.h"

bool UAdditionalDepotsCachedConfig::AbbreviateNumbers = true;
bool UAdditionalDepotsCachedConfig::CostProgressRelativeToTotal = false;

void UAdditionalDepotsCachedConfig::UpdateFromConfig(UObject* worldContext)
{
	FAdditionalDepotConfigStruct Config = FAdditionalDepotConfigStruct::GetActiveConfig(worldContext);

	AbbreviateNumbers = Config.AbbreviateNumbers;
	CostProgressRelativeToTotal = Config.CostProgressRelativeToTotal;
}
