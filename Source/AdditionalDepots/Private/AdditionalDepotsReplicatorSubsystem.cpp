#include "AdditionalDepotsReplicatorSubsystem.h"

#include "AdditionalDepotsClientSubsystem.h"
#include "AdditionalDepotsServerSubsystem.h"
#include "EngineUtils.h"
#include "NameAsStringProxyArchive.h"
#include "ReliableMessagingPlayerComponent.h"
#include "SubsystemActorManager.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsReplicatorSubsystem);

#pragma optimize("", off)

FArchive& operator<<(FArchive& Ar, FReplicatedItemData& Info)
{
	//Ar << Info.ListIdentifier;
	//Ar << Info.ItemClass;
	Ar << Info.Amount;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FAdditionalDepotsItemReplicationMessage& Message)
{
	Ar << Message.ItemData;
	return Ar;
}

AAdditionalDepotsReplicatorSubsystem* AAdditionalDepotsReplicatorSubsystem::Get(UWorld* world) {
	USubsystemActorManager* SubsystemActorManager = world->GetSubsystem<USubsystemActorManager>();
	fgcheck(SubsystemActorManager);

	return SubsystemActorManager->GetSubsystemActor<AAdditionalDepotsReplicatorSubsystem>();
}

AAdditionalDepotsReplicatorSubsystem* AAdditionalDepotsReplicatorSubsystem::Get(UObject* worldContext) {
	UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::Assert);

	return Get(world);
}

AAdditionalDepotsReplicatorSubsystem::AAdditionalDepotsReplicatorSubsystem() : Super() {
	UE_LOG(LogAdditionalDepotsReplicatorSubsystem, Display, TEXT("AAdditionalDepotsReplicatorSubsystem::AAdditionalDepotsReplicatorSubsystem()"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.1f;

	ReplicationPolicy = ESubsystemReplicationPolicy::SpawnLocal;
}

void AAdditionalDepotsReplicatorSubsystem::BeginPlay() {
	Super::BeginPlay();

	UE_LOG(LogAdditionalDepotsReplicatorSubsystem, Display, TEXT("AAdditionalDepotsReplicatorSubsystem::BeginPlay()"));

	//UWorld* world = GetWorld();
	//const ENetMode ActiveNetMode = world->GetNetMode();

	//cant use Authority checks here because its locally spawned
	//if (ActiveNetMode != ENetMode::NM_Client) {
	//}
}

void AAdditionalDepotsReplicatorSubsystem::Tick(float dt) {
	Super::Tick(dt);

	//cant use Authority checks here because its locally spawned
	const ENetMode ActiveNetMode = GetWorld()->GetNetMode();
	if (ActiveNetMode == ENetMode::NM_Client)
	{
		SetActorTickEnabled(false);
		return;
	}

	AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());
	if (!IsValid(serverSubsystem)) {
		UE_LOG(LogAdditionalDepotsReplicatorSubsystem, Error, TEXT("Failed to find AAdditionalDepotsServerSubsystem in the world"));
		return;
	}

	if (!serverSubsystem->IsInitialized())
	{
		return; //waiting for initialization
	}

	for (TActorIterator<APlayerController> actorIterator(GetWorld()); actorIterator; ++actorIterator) {
		const APlayerController* PlayerController = *actorIterator;
		if (!IsValid(PlayerController) || PlayerController->IsLocalController())
			continue;

		SendInitialReplicationData(PlayerController);
	}

	SetActorTickEnabled(false);
}

void AAdditionalDepotsReplicatorSubsystem::SendInitialReplicationData(const APlayerController* PlayerController) const
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

	UE_LOG(LogAdditionalDepotsReplicatorSubsystem, Log, TEXT("Sending initial replication message with %d items in the lookup array to player %s"), Message.ItemData.Num(), *GetPathName(GetOwner()));

	SendRawMessage(PlayerController, Message.MessageId, [&](FArchive& Ar) { Ar << Message; });
}

void AAdditionalDepotsReplicatorSubsystem::OnPlayerControllerBeginPlay(const APlayerController* PlayerController)
{
	if (!IsValid(PlayerController))
		return;

	// We are Server, and this is a remote player. Send descriptor lookup array to the client
	if (PlayerController->HasAuthority() && !PlayerController->IsLocalController())
	{
		AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());
		if (!IsValid(serverSubsystem)) {
			UE_LOG(LogAdditionalDepotsReplicatorSubsystem, Error, TEXT("AAdditionalDepotsReplicatorSubsystem::OnPlayerControllerBeginPlay() Failed to find AAdditionalDepotsServerSubsystem in the world"));
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
			UE_LOG(LogAdditionalDepotsReplicatorSubsystem, Log, TEXT("Registered message handler for local player"));
			PlayerComponent->RegisterMessageHandler(RELIABLE_MESSAGING_CHANNEL_ID_ADDITIONAL_DEPOTS,
				UReliableMessagingPlayerComponent::FOnBulkDataReplicationPayloadReceived::CreateUObject(this, &AAdditionalDepotsReplicatorSubsystem::OnRawDataReceived));
		}
	}
}

void AAdditionalDepotsReplicatorSubsystem::SendRawMessage(const APlayerController* PlayerController, EAdditionalDepotsReplicatorMessageId MessageId, const TFunctionRef<void(FArchive&)>& MessageSerializer) const
{
	TArray<uint8> RawMessageData;
	FMemoryWriter RawMessageMemoryWriter(RawMessageData);
	FNameAsStringProxyArchive NameAsStringProxyArchive(RawMessageMemoryWriter);

	NameAsStringProxyArchive << MessageId;
	MessageSerializer(NameAsStringProxyArchive);

	UReliableMessagingPlayerComponent* PlayerComponent = UReliableMessagingPlayerComponent::GetFromPlayer(PlayerController);
	if (ensure(PlayerComponent))
	{
		UE_LOG(LogAdditionalDepotsReplicatorSubsystem, Log, TEXT("Sending Message ID %d with %d bytes of payload to player %s"), MessageId, RawMessageData.Num(), *GetPathName(GetOwner()));
		PlayerComponent->SendMessage(RELIABLE_MESSAGING_CHANNEL_ID_ADDITIONAL_DEPOTS, MoveTemp(RawMessageData));
	}
}

void AAdditionalDepotsReplicatorSubsystem::OnRawDataReceived(TArray<uint8>&& InMessageData) const
{
	FMemoryReader RawMessageMemoryReader(InMessageData);
	FNameAsStringProxyArchive NameAsStringProxyArchive(RawMessageMemoryReader);

	EAdditionalDepotsReplicatorMessageId MessageId{};
	NameAsStringProxyArchive << MessageId;

	UE_LOG(LogAdditionalDepotsReplicatorSubsystem, Log, TEXT("Received Message ID %d with %d bytes of payload"), MessageId, InMessageData.Num());
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


void AAdditionalDepotsReplicatorSubsystem::ReceiveItemReplicationData(const FAdditionalDepotsItemReplicationMessage& ItemReplicationMessage) const
{
	UE_LOG(LogAdditionalDepotsReplicatorSubsystem, Log, TEXT("Received %d item replication datas from the server"), ItemReplicationMessage.ItemData.Num());

	AAdditionalDepotsClientSubsystem* clientSubsystem = UAdditionalDepotsUtils::GetSubsystemActorIncludingParentClasses<AAdditionalDepotsClientSubsystem>(GetWorld());
	if (!clientSubsystem)
	{
		UE_LOG(LogAdditionalDepotsReplicatorSubsystem, Error, TEXT("Failed to find AAdditionalDepotsClientSubsystem in the world"));
		return;
	}

	for (const FReplicatedItemData& itemData : ItemReplicationMessage.ItemData)
	{
		//FName listIdentifier = FName(*itemData.ListIdentifier);
		//clientSubsystem->AddItemData(listIdentifier, itemData.ItemClass, itemData.Amount);
	}
}

#pragma optimize("", on)
