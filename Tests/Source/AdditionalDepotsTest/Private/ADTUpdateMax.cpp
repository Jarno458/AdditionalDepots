#include "ADTUpdateMax.h"

#include "AdditionalDepotsServerSubsystem.h"
#include "ADTUtils.h"
#include "ChatCommandLibrary.h"

ADTUpdateMax::ADTUpdateMax() {
	CommandName = TEXT("ad-max");
	Usage = FText::FromString(TEXT("/ad-max <depot-identifier> <Max Type> <Max Amount> - Sets max allowed items for a depot"));
	Aliases.Add(TEXT("admax"));
	MinNumberOfArguments = 3;
}

EExecutionStatus ADTUpdateMax::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());

	FName Identifier;
	if (!UADTUtils::TryGetListIdentifierFromArgument(Sender, serverSubsystem, Arguments, 0, Identifier))
	{
		return EExecutionStatus::BAD_ARGUMENTS;
	}

	EFAAdditionalDepotsMaxType MaxType;
	if (!UADTUtils::TryGetMaxTypeFromArgument(Sender, Arguments, 1, MaxType))
	{
		return EExecutionStatus::BAD_ARGUMENTS;
	}

	int32 MaxAmount;
	if (!UADTUtils::TryGetAmountFromArgument(Sender, Arguments, 2, MaxAmount))
	{
		return EExecutionStatus::BAD_ARGUMENTS;
	}

	serverSubsystem->UpdateMaxAmount(Identifier, MaxType, MaxAmount);

	return EExecutionStatus::COMPLETED;
}


