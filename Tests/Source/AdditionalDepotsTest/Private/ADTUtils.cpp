#include "ADTUtils.h"

#include "ChatCommandLibrary.h"
#include "DefaultValueHelper.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsTestUtils);

bool UADTUtils::TryGetListIdentifierFromArgument(UCommandSender* Sender, AAdditionalDepotsServerSubsystem* serverSubsystem, const TArray<FString>& Arguments, int32 ArgumentIndex, FName& Identifier)
{
	if (!serverSubsystem)
	{
		return false;
	}

	if (Arguments.Num() <= ArgumentIndex)
	{
		return false;
	}

	Identifier = FName(*Arguments[ArgumentIndex]);

	if (!serverSubsystem->GetListIdentifiers().Contains(Identifier))
	{
		Sender->SendChatMessage(FString::Printf(TEXT("Depot with identifier \"%s\" not found"), *Arguments[ArgumentIndex]), FLinearColor::Red);
		return false;
	}

	return true;
}

AFGPlayerController* UADTUtils::GetPlayerControllerFromArgument(UCommandSender* Sender, const TArray<FString>& Arguments, int32 ArgumentIndex, UObject* WorldContext) {
	if (Arguments.Num() <= ArgumentIndex)
	{
		return Sender->GetPlayer();
	}

	TArray<AFGPlayerController*> controllers = AChatCommandSubsystem::ParsePlayerName(Sender, Arguments[ArgumentIndex], WorldContext);
	if (controllers.Num() == 0)
	{
		int32 controllerNumber;
		if (TryGetAmountFromArgument(Sender, Arguments, ArgumentIndex, controllerNumber))
		{
			UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::Assert);
			for (TPlayerControllerIterator<AFGPlayerController>::ServerAll It(World); It; ++It) {	
				controllers.Add(*It);
			}

			if (controllerNumber >= 0 && controllerNumber < controllers.Num())
			{
				return controllers[controllerNumber];
			}
			else
			{
				Sender->SendChatMessage(FString::Printf(TEXT("Player with number %d not found, total number of players: %d"), controllerNumber, controllers.Num()), FLinearColor::Red);
				return nullptr;
			}
		}

		Sender->SendChatMessage(FString::Printf(TEXT("Player \"%s\" not found"), *Arguments[ArgumentIndex]), FLinearColor::Red);
		return nullptr;
	}

	if (controllers.Num() > 1)
	{
		Sender->SendChatMessage(FString::Printf(TEXT("Multiple players found with name \"%s\""), *Arguments[ArgumentIndex]), FLinearColor::Red);
		return nullptr;
	}

	AFGPlayerController* controller = *controllers.GetData();
	return controller;
}

bool UADTUtils::TryGetItemDescriptorFromArgument(UCommandSender* Sender, const TArray<FString>& Arguments, int32 ArgumentIndex, TSubclassOf<UFGItemDescriptor>& Class)
{
	if (Arguments.Num() <= ArgumentIndex)
	{
		return false;
	}

	TArray<TSubclassOf<UFGItemDescriptor>> descriptors;
	UFGBlueprintFunctionLibrary::Cheat_GetAllDescriptors(descriptors);

	for (TSubclassOf<UFGItemDescriptor> descriptor : descriptors)
	{
		if (UFGItemDescriptor::GetItemName(descriptor).ToString().Equals(Arguments[ArgumentIndex], ESearchCase::IgnoreCase))
		{
			Class = descriptor;
			return true;
		}
	}

	Sender->SendChatMessage(FString::Printf(TEXT("Item \"%s\" not found"), *Arguments[ArgumentIndex]), FLinearColor::Red);
	return false;
}

bool UADTUtils::TryGetAmountFromArgument(UCommandSender* Sender, const TArray<FString>& Arguments, int32 ArgumentIndex, int32& amount)
{
	if (Arguments.Num() <= ArgumentIndex)
	{
		return false;
	}

	if (!FDefaultValueHelper::ParseInt(Arguments[ArgumentIndex], amount))
	{
		Sender->SendChatMessage(FString::Printf(TEXT("Invalid amount \"%s\""), *Arguments[ArgumentIndex]), FLinearColor::Red);
		return false;
	}

	return true;
}

bool UADTUtils::TryGetBooleanFromArgument(UCommandSender* Sender, const TArray<FString>& Arguments, int32 ArgumentIndex, bool& result)
{
	if (Arguments.Num() <= ArgumentIndex)
	{
		return false;
	}

	const FString Value = Arguments[ArgumentIndex].TrimStartAndEnd();
	if (Value.IsEmpty())
	{
		Sender->SendChatMessage(FString::Printf(TEXT("Invalid boolean value \"%s\""), *Arguments[ArgumentIndex]), FLinearColor::Red);
		return false;
	}

	if (Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) ||
		Value.Equals(TEXT("1"), ESearchCase::IgnoreCase) ||
		Value.Equals(TEXT("yes"), ESearchCase::IgnoreCase) ||
		Value.Equals(TEXT("y"), ESearchCase::IgnoreCase) ||
		Value.Equals(TEXT("on"), ESearchCase::IgnoreCase))
	{
		result = true;
		return true;
	}

	if (Value.Equals(TEXT("false"), ESearchCase::IgnoreCase) ||
		Value.Equals(TEXT("0"), ESearchCase::IgnoreCase) ||
		Value.Equals(TEXT("no"), ESearchCase::IgnoreCase) ||
		Value.Equals(TEXT("n"), ESearchCase::IgnoreCase) ||
		Value.Equals(TEXT("off"), ESearchCase::IgnoreCase))
	{
		result = false;
		return true;
	}

	Sender->SendChatMessage(FString::Printf(TEXT("Invalid boolean value \"%s\" (use true/false, 1/0, yes/no, on/off)"), *Arguments[ArgumentIndex]), FLinearColor::Red);
	return false;
}

bool UADTUtils::TryGetMaxTypeFromArgument(UCommandSender* Sender, const TArray<FString>& Arguments, int32 ArgumentIndex, EFAAdditionalDepotsMaxType& result)
{
	if (Arguments.Num() <= ArgumentIndex)
	{
		return false;
	}

	const UEnum* Enum = StaticEnum<EFAAdditionalDepotsMaxType>();
	const int64 FoundValue = Enum->GetValueByNameString(*Arguments[ArgumentIndex].TrimStartAndEnd().ToLower());
	if (FoundValue == INDEX_NONE)
	{
		Sender->SendChatMessage(FString::Printf(TEXT("Invalid max type \"%s\" (use Stacks|Total|TotalHidden|None)"), *Arguments[ArgumentIndex]), 	FLinearColor::Red);
		return false;
	}

	result = static_cast<EFAAdditionalDepotsMaxType>(FoundValue);
	return true;
}
