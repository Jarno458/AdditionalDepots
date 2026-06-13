#include "ADTAddItems.h"

#include "AdditionalDepotsServerSubsystem.h"
#include "ADTUtils.h"
#include "Command/ChatCommandLibrary.h"

ADTAddItems::ADTAddItems() {
	CommandName = TEXT("ad-addmany");
	Usage = FText::FromString(TEXT("/ad-addmany <depot-identifier> <amount> <\"item-name\" .. n> [<player name>] - Adds an item to target depot (alternative /adaddmanynp for no player specified, or /adaddmannpyspecific for a specific player)"));
	Aliases.Add(TEXT("adaddmany"));
	Aliases.Add(TEXT("adaddmanynp"));
	Aliases.Add(TEXT("adaddmannpyspecific"));
	bOnlyUsableByPlayer = true;
	MinNumberOfArguments = 3;
}

EExecutionStatus ADTAddItems::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());

	FName Identifier;
	if (!UADTUtils::TryGetListIdentifierFromArgument(Sender, serverSubsystem, Arguments, 0, Identifier))
	{
		return EExecutionStatus::BAD_ARGUMENTS;
	}

	int32 amount;
	if (!UADTUtils::TryGetAmountFromArgument(Sender, Arguments, 1, amount))
	{
		return EExecutionStatus::BAD_ARGUMENTS;
	}

	TArray<FItemAmount> itemsToAdd;

	int32 lastItemArgument = Arguments.Num() - 1;
	if (Label.EndsWith("specific"))
		lastItemArgument -= 1;

	for (int i = 2; i <= lastItemArgument; i++){ //TODO max according to Label
		TSubclassOf<UFGItemDescriptor> itemDescriptor;
		if (!UADTUtils::TryGetItemDescriptorFromArgument(Sender, Arguments, i, itemDescriptor))
		{
			return EExecutionStatus::BAD_ARGUMENTS;
		}

		itemsToAdd.Add(FItemAmount(itemDescriptor, amount));
	}

	if (Label.EndsWith("np")){
		serverSubsystem->AddItems(Identifier, itemsToAdd);
	}
	else {
		AFGPlayerController* controller = nullptr;
		if (Label == TEXT("adaddmanyspecific")) {
			controller = UADTUtils::GetPlayerControllerFromArgument(Sender, Arguments, Arguments.Num() - 1, this);
			if (!controller)
			{
				return EExecutionStatus::BAD_ARGUMENTS;
			}
		}

		serverSubsystem->AddItems(Identifier, itemsToAdd, controller ? controller->GetPlayerState<AFGPlayerState>() : nullptr);
	}

	return EExecutionStatus::COMPLETED;
}


