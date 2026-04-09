#include "AdditionalDepotsReplicatorComponent.h"

#include "AdditionalDepotsClientSubsystem.h"
#include "AdditionalDepotsServerSubsystem.h"
#include "EngineUtils.h"
#include "NameAsStringProxyArchive.h"
#include "StructuredLog.h"
#include "ReliableMessagingPlayerComponent.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsReplicatorComponent);

#pragma optimize("", off)

FArchive& operator<<(FArchive& Ar, FReplicatedItemData& Info)
{
	Ar << Info.ListIdentifier;
	//Ar << Info.ItemClass;
	Ar << Info.Amount;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FAdditionalDepotsItemReplicationMessage& Message)
{
	Ar << Message.ItemData;
	return Ar;
}

UAdditionalDepotsReplicatorComponent::UAdditionalDepotsReplicatorComponent() {
	UE_LOG(LogAdditionalDepotsReplicatorComponent, Display, TEXT("UAdditionalDepotsReplicatorComponent::UAdditionalDepotsReplicatorComponent()"));

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UAdditionalDepotsReplicatorComponent::BeginPlay() {
	Super::BeginPlay();

	UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Display, "UAdditionalDepotsReplicatorComponent::BeginPlay()");

	const APlayerController* PlayerController = CastChecked<APlayerController>(GetOwner());

	if (!IsValid(PlayerController))
		return;

	// We are Server, and this is a remote player. Send descriptor lookup array to the client
	if (PlayerController->HasAuthority() && !PlayerController->IsLocalController())
	{
		AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());
		if (!IsValid(serverSubsystem)) {
			UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Error, "UAdditionalDepotsReplicatorComponent::OnPlayerControllerBeginPlay() Failed to find AAdditionalDepotsServerSubsystem in the world");
			return;
		}

		if (!serverSubsystem->IsInitialized())
			return;

		SendInitialReplicationData(PlayerController);
	}
	// We are local player connected to a server, register the message handler
	else if (!PlayerController->HasAuthority() && PlayerController->IsLocalController())
	{
		//register data received handler
		if (UReliableMessagingPlayerComponent* PlayerComponent = UReliableMessagingPlayerComponent::GetFromPlayer(PlayerController))
		{
			UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Log, "Registered message handler for local player");
			PlayerComponent->RegisterMessageHandler(RELIABLE_MESSAGING_CHANNEL_ID_ADDITIONAL_DEPOTS,
				UReliableMessagingPlayerComponent::FOnBulkDataReplicationPayloadReceived::CreateUObject(this, &UAdditionalDepotsReplicatorComponent::OnRawDataReceived));
		}
	}
	// if we are a local player, this BeginPlay fires way before the server subsystem is initialized, so we need to wait and send the data in TickComponent
}

void UAdditionalDepotsReplicatorComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//cant use Authority checks here because its locally spawned
	const ENetMode ActiveNetMode = GetWorld()->GetNetMode();
	if (ActiveNetMode == ENetMode::NM_Client)
	{
		SetComponentTickEnabled(false);
		return;
	}

	AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());
	if (!IsValid(serverSubsystem)) {
		UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Error, "UAdditionalDepotsReplicatorComponent::TickComponent() Failed to find AAdditionalDepotsServerSubsystem in the world");
		return;
	}

	if (!serverSubsystem->IsInitialized())
	{
		return; //waiting for initialization
	}

	for (TActorIterator<APlayerController> actorIterator(GetWorld()); actorIterator; ++actorIterator) {
		const APlayerController* PlayerController = *actorIterator;
		if (!IsValid(PlayerController))
			continue;

		SendInitialReplicationData(PlayerController);
	}

	SetComponentTickEnabled(false);
}

void UAdditionalDepotsReplicatorComponent::SendInitialReplicationData(const APlayerController* PlayerController) const
{
	if (!IsValid(PlayerController))
		return;

	AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());
	if (!IsValid(serverSubsystem) || !serverSubsystem->IsInitialized())
		return;

	FAdditionalDepotsItemReplicationMessage Message;
	Message.ItemData = TArray<FReplicatedItemData>();

	for (FName listIdentifier : serverSubsystem->GetListIdentifiers())
	{
		for (const FItemAmount& Item : serverSubsystem->GetItems(listIdentifier))
		{
			Message.ItemData.Add(FReplicatedItemData(listIdentifier, Item.ItemClass, Item.Amount));
		}
	}

	UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Display, "UAdditionalDepotsReplicatorComponent::SendInitialReplicationData() Sending initial replication message with {0} items in the lookup array to player", Message.ItemData.Num());

	SendRawMessage(PlayerController, Message.MessageId, [&](FArchive& Ar) { Ar << Message; });
}

void UAdditionalDepotsReplicatorComponent::SendRawMessage(const APlayerController* PlayerController, EAdditionalDepotsReplicatorMessageId MessageId, const TFunctionRef<void(FArchive&)>& MessageSerializer) const
{
	TArray<uint8> RawMessageData;
	FMemoryWriter RawMessageMemoryWriter(RawMessageData);
	FNameAsStringProxyArchive NameAsStringProxyArchive(RawMessageMemoryWriter);

	NameAsStringProxyArchive << MessageId;
	MessageSerializer(NameAsStringProxyArchive);

	// if we are our own target
	if (PlayerController->IsLocalController())
	{
		OnRawDataReceived(MoveTemp(RawMessageData));
	}
	else
	{
		UReliableMessagingPlayerComponent* PlayerComponent = UReliableMessagingPlayerComponent::GetFromPlayer(PlayerController);
		if (ensure(PlayerComponent))
		{
			UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Display, "UAdditionalDepotsReplicatorComponent::SendRawMessage() Sending raw message {0} to player", RawMessageData.Num());
			PlayerComponent->SendMessage(RELIABLE_MESSAGING_CHANNEL_ID_ADDITIONAL_DEPOTS, MoveTemp(RawMessageData));
		}
	}
}

void UAdditionalDepotsReplicatorComponent::OnRawDataReceived(TArray<uint8>&& InMessageData) const
{
	FMemoryReader RawMessageMemoryReader(InMessageData);
	FNameAsStringProxyArchive NameAsStringProxyArchive(RawMessageMemoryReader);

	EAdditionalDepotsReplicatorMessageId MessageId{};
	NameAsStringProxyArchive << MessageId;

	UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Display, "UAdditionalDepotsReplicatorComponent::OnRawDataReceived() Received raw message with {0} bytes", InMessageData.Num());
	if (NameAsStringProxyArchive.IsError()) return;

	switch (MessageId)
	{
	case EAdditionalDepotsReplicatorMessageId::ItemData:
	{
		FAdditionalDepotsItemReplicationMessage ItemReplicationMessage;
		NameAsStringProxyArchive << ItemReplicationMessage;

		if (NameAsStringProxyArchive.IsError())
			return;

		ReceiveItemReplicationData(ItemReplicationMessage);
		break;
	}
	case EAdditionalDepotsReplicatorMessageId::ListConfig:
	{
		break;
	}
	}
}


void UAdditionalDepotsReplicatorComponent::ReceiveItemReplicationData(const FAdditionalDepotsItemReplicationMessage& ItemReplicationMessage) const
{
	UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Display, "UAdditionalDepotsReplicatorComponent::ReceiveItemReplicationData() Received {0} item replication datas from the server", ItemReplicationMessage.ItemData.Num());

	AAdditionalDepotsClientSubsystem* clientSubsystem = UAdditionalDepotsUtils::GetSubsystemActorIncludingParentClasses<AAdditionalDepotsClientSubsystem>(GetWorld());
	if (!clientSubsystem)
	{
		UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Error, "UAdditionalDepotsReplicatorComponent::ReceiveItemReplicationData() Failed to find AAdditionalDepotsClientSubsystem in the world");
		return;
	}

	for (const FReplicatedItemData& itemData : ItemReplicationMessage.ItemData)
	{
		//FName listIdentifier = FName(*itemData.ListIdentifier);
		//clientSubsystem->AddItemData(listIdentifier, itemData.ItemClass, itemData.Amount);
	}
}

#pragma optimize("", on)
