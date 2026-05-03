#include "AdditionalDepotsReplicatorComponent.h"

#include "AdditionalDepotsClientSubsystem.h"
#include "AdditionalDepotsServerSubsystem.h"
#include "AdditionalDepotsUtils.h"
#include "EngineUtils.h"
#include "ObjectAndNameAsStringProxyArchive.h"
#include "StructuredLog.h"
#include "ReliableMessagingPlayerComponent.h"

DEFINE_LOG_CATEGORY(LogAdditionalDepotsReplicatorComponent);

#pragma optimize("", off)

FArchive& operator<<(FArchive& Ar, FReplicatedItemData& Info)
{
	Ar << Info.ListIdentifier;
	Ar << Info.ItemClass;
	Ar << Info.Amount;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FAdditionalDepotsItemReplicationMessage& Message)
{
	Ar << Message.ItemData;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FReplicatedDepotConfigurationData& Info)
{
	Ar << Info.ListIdentifier;
	Ar << Info.MaxAmount;
	Ar << Info.MaxType;
	Ar << Info.CanDragItemsToInventory;
	Ar << Info.CanBeUsedWhenBuilding;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FAdditionalDepotsConfigReplicationMessage& Message)
{
	Ar << Message.ConfigData;
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

	const ENetMode ActiveNetMode = GetWorld()->GetNetMode();
	if (ActiveNetMode != ENetMode::NM_Client)
	{
		AAdditionalDepotsServerSubsystem* serverSubsystem = AAdditionalDepotsServerSubsystem::Get(GetWorld());
		if (!IsValid(serverSubsystem)) {
			UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Error, "UAdditionalDepotsReplicatorComponent::OnPlayerControllerBeginPlay() Failed to find AAdditionalDepotsServerSubsystem in the world");
			return;
		}

		serverSubsystem->OnItemAmountUpdated.AddDynamic(this, &UAdditionalDepotsReplicatorComponent::SendUpdatedItemReplicationData);
		serverSubsystem->OnConfigurationUpdated.AddDynamic(this, &UAdditionalDepotsReplicatorComponent::SendUpdatedConfiguration);
	}

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
}

void UAdditionalDepotsReplicatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APlayerController* tickingPlayerController = CastChecked<APlayerController>(GetOwner());
	if (!IsValid(tickingPlayerController))
		return;

	if (!tickingPlayerController->HasAuthority())
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

	AFGPlayerState* playerState = PlayerController->GetPlayerState<AFGPlayerState>();
	if (!IsValid(playerState))
		return;

	FAdditionalDepotsItemReplicationMessage Message;
	Message.ItemData = TArray<FReplicatedItemData>();

	FAdditionalDepotsConfigReplicationMessage ConfigMessage;
	ConfigMessage.ConfigData = TArray<FReplicatedDepotConfigurationData>();

	for (FName listIdentifier : serverSubsystem->GetListIdentifiers())
	{
		for (const FItemAmount& Item : serverSubsystem->GetItems(listIdentifier, playerState))
		{
			Message.ItemData.Add(FReplicatedItemData(listIdentifier, Item.ItemClass, Item.Amount));
		}

		FAdditionalDepotConfiguration Config = serverSubsystem->GetConfiguration(listIdentifier);

		ConfigMessage.ConfigData.Add(FReplicatedDepotConfigurationData(listIdentifier, Config));
	}

	UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Display, "UAdditionalDepotsReplicatorComponent::SendInitialReplicationData() Sending initial replication message with {0} items in the lookup array to player {1}", Message.ItemData.Num(), playerState->GetPlayerName());

	SendRawMessage(PlayerController, Message.MessageId, [&](FArchive& Ar) { Ar << Message; });
	SendRawMessage(PlayerController, ConfigMessage.MessageId, [&](FArchive& Ar) { Ar << ConfigMessage; });
}

void UAdditionalDepotsReplicatorComponent::SendRawMessage(const APlayerController* PlayerController, EAdditionalDepotsReplicatorMessageId MessageId, const TFunctionRef<void(FArchive&)>& MessageSerializer) const
{
	TArray<uint8> RawMessageData;
	FMemoryWriter RawMessageMemoryWriter(RawMessageData);
	FObjectAndNameAsStringProxyArchive ProxyArchive(RawMessageMemoryWriter, true);

	ProxyArchive << MessageId;
	MessageSerializer(ProxyArchive);

	// if we are our own target
	if (PlayerController->IsLocalController())
	{
		UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Display, "UAdditionalDepotsReplicatorComponent::SendRawMessage() Sending raw message {0} to local player", RawMessageData.Num());
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
	FObjectAndNameAsStringProxyArchive ProxyArchive(RawMessageMemoryReader, true);

	EAdditionalDepotsReplicatorMessageId MessageId{};
	ProxyArchive << MessageId;

	UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Display, "UAdditionalDepotsReplicatorComponent::OnRawDataReceived() Received raw message with {0} bytes", InMessageData.Num());
	if (ProxyArchive.IsError()) return;

	switch (MessageId)
	{
		case EAdditionalDepotsReplicatorMessageId::ItemData:
		{
			FAdditionalDepotsItemReplicationMessage ItemReplicationMessage;
			ProxyArchive << ItemReplicationMessage;

			if (ProxyArchive.IsError())
				return;

			ReceiveItemReplicationData(ItemReplicationMessage);
			break;
		}
		case EAdditionalDepotsReplicatorMessageId::ListConfig:
		{

			FAdditionalDepotsConfigReplicationMessage ConfigReplicationMessage;
			ProxyArchive << ConfigReplicationMessage;

			if (ProxyArchive.IsError())
				return;

			ReceiveConfigReplicationData(ConfigReplicationMessage);
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
		clientSubsystem->AddItemData(itemData.ListIdentifier, itemData.ItemClass, itemData.Amount);
	}
}

void UAdditionalDepotsReplicatorComponent::ReceiveConfigReplicationData(const FAdditionalDepotsConfigReplicationMessage& ConfigReplicationMessage) const
{
	UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Display, "UAdditionalDepotsReplicatorComponent::ReceiveConfigReplicationData() Received {0} configuration replication datas from the server", ConfigReplicationMessage.ConfigData.Num());

	AAdditionalDepotsClientSubsystem* clientSubsystem = UAdditionalDepotsUtils::GetSubsystemActorIncludingParentClasses<AAdditionalDepotsClientSubsystem>(GetWorld());
	if (!clientSubsystem)
	{
		UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Error, "UAdditionalDepotsReplicatorComponent::ReceiveConfigReplicationData() Failed to find AAdditionalDepotsClientSubsystem in the world");
		return;
	}

	for (const FReplicatedDepotConfigurationData& configData : ConfigReplicationMessage.ConfigData)
	{
		FAdditionalDepotConfiguration config;

		config.MaxAmount = configData.MaxAmount;
		config.MaxType = configData.MaxType;
		config.CanDragItemsToInventory = configData.CanDragItemsToInventory;
		config.CanBeUsedWhenBuilding = configData.CanBeUsedWhenBuilding;

		clientSubsystem->UpdateConfiguration(configData.ListIdentifier, config);
	}
}

void UAdditionalDepotsReplicatorComponent::SendUpdatedItemReplicationData(FName ListIdentifier, TArray<FItemAmount> items, AFGPlayerState* playerState)
{
	FAdditionalDepotsItemReplicationMessage Message;
	Message.ItemData = TArray<FReplicatedItemData>();

	for (const FItemAmount& Item : items)
	{
		Message.ItemData.Add(FReplicatedItemData(ListIdentifier, Item.ItemClass, Item.Amount));
	}

	if (IsValid(playerState))
	{
		const APlayerController* PlayerController = playerState->GetPlayerController();
		if (IsValid(PlayerController))
		{
			UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Display, "UAdditionalDepotsReplicatorComponent::SendUpdatedItemReplicationData() Sending updated replication message with {0} items in the lookup array to player {1}", Message.ItemData.Num(), playerState->GetPlayerName());
			SendRawMessage(PlayerController, Message.MessageId, [&](FArchive& Ar) { Ar << Message; });
		}
	} 
	else
	{
		//TODO could optimize using relevancy, that would be fancy
		for (TActorIterator<APlayerController> actorIterator(GetWorld()); actorIterator; ++actorIterator) {
			const APlayerController* PlayerController = *actorIterator;
			if (!IsValid(PlayerController))
				continue;

			UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Display, "UAdditionalDepotsReplicatorComponent::SendUpdatedItemReplicationData() Sending updated replication message with {0} items in the lookup array to player {1}", Message.ItemData.Num(), PlayerController->GetPlayerState<APlayerState>()->GetPlayerName());

			SendRawMessage(PlayerController, Message.MessageId, [&](FArchive& Ar) { Ar << Message; });
		}
	}
}

void UAdditionalDepotsReplicatorComponent::SendUpdatedConfiguration(FName ListIdentifier, FAdditionalDepotConfiguration config)
{
	FAdditionalDepotsConfigReplicationMessage Message;
	Message.ConfigData = TArray<FReplicatedDepotConfigurationData>();
	Message.ConfigData.Add(FReplicatedDepotConfigurationData(ListIdentifier, config));

	for (TActorIterator<APlayerController> actorIterator(GetWorld()); actorIterator; ++actorIterator) {
		const APlayerController* PlayerController = *actorIterator;
		if (!IsValid(PlayerController))
			continue;

		UE_LOGFMT(LogAdditionalDepotsReplicatorComponent, Display, "UAdditionalDepotsReplicatorComponent::SendUpdatedConfiguration() Sending updated configuration message with {0} configurations in the lookup array to player {1}", Message.ConfigData.Num(), PlayerController->GetPlayerState<APlayerState>()->GetPlayerName());

		SendRawMessage(PlayerController, Message.MessageId, [&](FArchive& Ar) { Ar << Message; });
	}
}

#pragma optimize("", on)
