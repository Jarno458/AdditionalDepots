#include "ADTSetItem.h"

#include "AdditionalDepotsServerSubsystem.h"
#include "ADTUtils.h"
#include "ChatCommandLibrary.h"

ADTSetItem::ADTSetItem() {
	CommandName = TEXT("ad-set");
	Usage = FText::FromString(TEXT("/ad-set <depot-identifier> <amount> <\"item-name\"> [<player name>] - Adds an item to target depot (alternative /adsetnp for no player specified)"));
	Aliases.Add(TEXT("adset"));
	Aliases.Add(TEXT("adaddnp"));
	bOnlyUsableByPlayer = true;
	MinNumberOfArguments = 3;
}

EExecutionStatus ADTSetItem::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());

	FName Identifier;
	if (!UADTUtils::TryGetListIdentifierFromArgument(Sender, serverSubsystem, Arguments, 0, Identifier))
		return EExecutionStatus::BAD_ARGUMENTS;

	int32 amount;
	if (!UADTUtils::TryGetAmountFromArgument(Sender, Arguments, 1, amount))
		return EExecutionStatus::BAD_ARGUMENTS;

	TSubclassOf<UFGItemDescriptor> itemDescriptor;
	if (!UADTUtils::TryGetItemDescriptorFromArgument(Sender, Arguments, 2, itemDescriptor))
		return EExecutionStatus::BAD_ARGUMENTS;

	if (Label.EndsWith("np")) {
		serverSubsystem->SetItem(Identifier, itemDescriptor, amount);
	}
	else
	{
		AFGPlayerController* controller = UADTUtils::GetPlayerControllerFromArgument(Sender, Arguments, 3, this);
		if (!controller)
			return EExecutionStatus::BAD_ARGUMENTS;

		serverSubsystem->SetItem(Identifier, itemDescriptor, amount, controller->GetPlayerState<AFGPlayerState>());
	}

	Sender->SendChatMessage(FString::Printf(TEXT("Set %s to %d successfully."), *UFGItemDescriptor::GetItemName(itemDescriptor).ToString(), amount), FLinearColor::Green);

	return EExecutionStatus::COMPLETED;
}
