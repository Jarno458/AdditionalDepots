#include "ADTUpdateConfig.h"

#include "AdditionalDepotsServerSubsystem.h"
#include "ADTUtils.h"
#include "Command/ChatCommandLibrary.h"

ADTUpdateConfig::ADTUpdateConfig() {
	CommandName = TEXT("ad-config");
	Usage = FText::FromString(TEXT("/ad-config <depot-identifier> <CanDragItemsToInventory> <CanBeUsedWhenBuilding> - Sets usage of depot"));
	Aliases.Add(TEXT("adconfig"));
	MinNumberOfArguments = 3;
}

EExecutionStatus ADTUpdateConfig::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());

	FName Identifier;
	if (!UADTUtils::TryGetListIdentifierFromArgument(Sender, serverSubsystem, Arguments, 0, Identifier))
	{
		return EExecutionStatus::BAD_ARGUMENTS;
	}

	bool canDragItemsToInventory;
	if (!UADTUtils::TryGetBooleanFromArgument(Sender, Arguments, 1, canDragItemsToInventory))
	{
		return EExecutionStatus::BAD_ARGUMENTS;
	}

	bool canBeUsedWhenBuilding;
	if (!UADTUtils::TryGetBooleanFromArgument(Sender, Arguments, 2, canBeUsedWhenBuilding))
	{
		return EExecutionStatus::BAD_ARGUMENTS;
	}

	serverSubsystem->UpdateCanDragToInventory(Identifier, canDragItemsToInventory);
	serverSubsystem->UpdateCanBeUsedForBuildingAndCrafting(Identifier, canBeUsedWhenBuilding);

	return EExecutionStatus::COMPLETED;
}


