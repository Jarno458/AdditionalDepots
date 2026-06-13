#include "ADTSetContent.h"

#include "AdditionalDepotsServerSubsystem.h"
#include "ADTUtils.h"
#include "Command/ChatCommandLibrary.h"

ADTSetContent::ADTSetContent() {
	CommandName = TEXT("ad-setcontent");
	Usage = FText::FromString(TEXT("/ad-setcontent <depot-identifier> <content preset index (0-3)> [<player name>] - Replaces all content with predefined content set, 0 for no content (alternative /adcontentnp for no player specified)"));
	Aliases.Add(TEXT("adcontent"));
	Aliases.Add(TEXT("adcontentnp"));
	bOnlyUsableByPlayer = true;
	MinNumberOfArguments = 2;
}

EExecutionStatus ADTSetContent::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());

	FName Identifier;
	if (!UADTUtils::TryGetListIdentifierFromArgument(Sender, serverSubsystem, Arguments, 0, Identifier))
	{
		return EExecutionStatus::BAD_ARGUMENTS;
	}

	int32 PresetIndex;
	if (!UADTUtils::TryGetAmountFromArgument(Sender, Arguments, 1, PresetIndex))
	{
		return EExecutionStatus::BAD_ARGUMENTS;
	}

	TArray<FItemAmount> items;
	if (ContentPresets.IsValidIndex(PresetIndex))
	{
		for (TPair<TSubclassOf<UFGItemDescriptor>, int>& itemAmounts : ContentPresets[PresetIndex].ItemAmounts)
		{
			items.Add(FItemAmount(itemAmounts.Key, itemAmounts.Value));
		}
	}

	if (Label.EndsWith("np")) {
		serverSubsystem->SetDepotContent(Identifier, items);
	} else	{
		AFGPlayerController* controller = UADTUtils::GetPlayerControllerFromArgument(Sender, Arguments, 3, this);
		if (!controller)
		{
			return EExecutionStatus::BAD_ARGUMENTS;
		}

		serverSubsystem->SetDepotContent(Identifier, items, controller->GetPlayerState<AFGPlayerState>());
	}

	return EExecutionStatus::COMPLETED;
}


