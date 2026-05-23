#include "ADTPrintPlayerState.h"

#include "ChatCommandLibrary.h"
#include "CommandSender.h"
#include "FGPlayerController.h"
#include "FGPlayerState.h"

ADTPrintPlayerState::ADTPrintPlayerState() {
	CommandName = TEXT("ad-ps");
	Usage = FText::FromString(TEXT("/ad-ps print player states of current connected clients"));
	Aliases.Add(TEXT("adps"));
	MinNumberOfArguments = 0;
}

EExecutionStatus ADTPrintPlayerState::ExecuteCommand_Implementation(UCommandSender* Sender, const TArray<FString>& Arguments, const FString& Label) {
	UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::Assert);

	for (TPlayerControllerIterator<AFGPlayerController>::ServerAll It(World); It; ++It) {
		AFGPlayerController* controller = *It;
		AFGPlayerState* state = controller->GetPlayerState<AFGPlayerState>();
		FString name = state->GetPlayerName();
		FString platform = state->GetPlayingPlatformName();

		Sender->SendChatMessage(FString::Printf(TEXT("Player: %s, platform: %s (ControllerAddr: 0x%p, StateAddr: 0x%p)"), *name, *platform, controller, state), FLinearColor::Yellow);
	}

	return EExecutionStatus::COMPLETED;
}