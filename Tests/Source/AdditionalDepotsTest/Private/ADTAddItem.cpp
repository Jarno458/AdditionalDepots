#include "ADTAddItem.h"

#include "AdditionalDepotsServerSubsystem.h"
#include "ADTUtils.h"
#include "ChatCommandLibrary.h"

ADTAddItem::ADTAddItem() {
	CommandName = TEXT("ad-add");
	Usage = FText::FromString(TEXT("/ad-add <depot-identifier> <amount> <\"item-name\"> [interval] [<player name>] - Adds an item to target depot (alternative /adaddnp for no player specified, or /adaddauto for automatic mode adding)"));
	Aliases.Add(TEXT("adadd"));
	Aliases.Add(TEXT("adaddnp"));
	Aliases.Add(TEXT("adaddauto"));
	Aliases.Add(TEXT("adaddautonp"));
	bOnlyUsableByPlayer = true;
	MinNumberOfArguments = 3;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

EExecutionStatus ADTAddItem::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
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

	TSubclassOf<UFGItemDescriptor> itemDescriptor;
	if (!UADTUtils::TryGetItemDescriptorFromArgument(Sender, Arguments, 2, itemDescriptor))
	{
		return EExecutionStatus::BAD_ARGUMENTS;
	}


	if (Label.Contains("auto")) {
		int32 intervalInSeconds;
		if (!UADTUtils::TryGetAmountFromArgument(Sender, Arguments, 3, intervalInSeconds))
		{
			return EExecutionStatus::BAD_ARGUMENTS;
		}

		SetActorTickEnabled(true);

		FIntervalItem item;
		item.listIdentifier = Identifier;
		item.Item = itemDescriptor;
		item.Amount = amount;
		item.IntervalCount = 100;
		item.intervalInSeconds = intervalInSeconds;
		item.NextTimestamp = FDateTime::Now() + FTimespan::FromSeconds(intervalInSeconds);

		if (Label.EndsWith("np")) {
			item.PlayerState = nullptr;
		} else
		{
			AFGPlayerController* controller = UADTUtils::GetPlayerControllerFromArgument(Sender, Arguments, 4, this);
			if (!controller)
			{
				return EExecutionStatus::BAD_ARGUMENTS;
			}

			item.PlayerState = controller->GetPlayerState<AFGPlayerState>();
		}

		IntervalItems.Add(item);

		Sender->SendChatMessage(FString::Printf(TEXT("Adding %d %s every %d seconds for a total of 100 times."), amount, *UFGItemDescriptor::GetItemName(itemDescriptor).ToString(), intervalInSeconds), FLinearColor::Green);
	}
	else {
		if (Label.EndsWith("np")) {
			serverSubsystem->AddItem(Identifier, itemDescriptor, amount);
		}
		else {
			AFGPlayerController* controller = UADTUtils::GetPlayerControllerFromArgument(Sender, Arguments, 3, this);
			if (!controller)
			{
				return EExecutionStatus::BAD_ARGUMENTS;
			}

			serverSubsystem->AddItem(Identifier, itemDescriptor, amount, controller->GetPlayerState<AFGPlayerState>());

			Sender->SendChatMessage(FString::Printf(TEXT("Added %d %s successfully."), amount, *UFGItemDescriptor::GetItemName(itemDescriptor).ToString()), FLinearColor::Green);
		}
	}

	return EExecutionStatus::COMPLETED;
}

void ADTAddItem::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FDateTime now = FDateTime::Now();

	AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());

	for (FIntervalItem& intervalItem : IntervalItems)
	{
		if (intervalItem.IntervalCount > 0 && now >= intervalItem.NextTimestamp)
		{
			intervalItem.IntervalCount--;

			serverSubsystem->AddItem(intervalItem.listIdentifier, intervalItem.Item, intervalItem.Amount);

			intervalItem.NextTimestamp = now + FTimespan::FromSeconds(intervalItem.intervalInSeconds);

		}
	}
}


