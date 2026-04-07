#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"

#include "AdditionalDepotsReplicatorSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdditionalDepotsReplicatorSubsystem, Log, All);

constexpr uint8 RELIABLE_MESSAGING_CHANNEL_ID_ADDITIONAL_DEPOTS = 83; //random value to avoid collisions with other subsystems



enum class EAdditionalDepotsReplicatorMessageId : uint32
{
	InitialReplication = 0x01,
	UpdateItem = 0x02,
};

/*
struct FPlayerInfoSubsystemInitialReplicationMessage
{
	static constexpr EPlayerInfoSubsystemMessageId MessageId = EPlayerInfoSubsystemMessageId::InitialReplication;
	TArray<FReplicatedFApPlayerInfo> PlayerInfos;

	friend FArchive& operator<<(FArchive& Ar, FPlayerInfoSubsystemInitialReplicationMessage& Message);
};

struct FPlayerInfoSubsystemUpdateReplicationMessage
{
	static constexpr EPlayerInfoSubsystemMessageId MessageId = EPlayerInfoSubsystemMessageId::PartialUpdate;
	TArray<FReplicatedFApPlayerInfo> PlayerInfos;

	friend FArchive& operator<<(FArchive& Ar, FPlayerInfoSubsystemInitialReplicationMessage& Message);
};
*/

UCLASS()
class AAdditionalDepotsReplicatorSubsystem : public AModSubsystem
{
	GENERATED_BODY()

public:
	AAdditionalDepotsReplicatorSubsystem();

	virtual void BeginPlay() override;

	virtual void Tick(float dt) override;

	static AAdditionalDepotsReplicatorSubsystem* Get(UWorld* world);
	UFUNCTION(BlueprintPure, Category = "Schematic", DisplayName = "Internal Additional Depots Replication system", Meta = (DefaultToSelf = "worldContext"))
	static AAdditionalDepotsReplicatorSubsystem* Get(UObject* worldContext);

private:
	//UPROPERTY()
	//AApConnectionInfoSubsystem* connectionInfoSubsystem;

	//bool isInitialized;

	//TMap<FApPlayer, FString> PlayerGamesMap;
	//TMap<FApPlayer, FString> PlayerNamesMap;

	//bool hasMultipleTeams = false;

public:
	/*UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsInitialized() const { return isInitialized; };

	UFUNCTION(BlueprintPure)
	FString GetPlayerName(FApPlayer player) const;

	UFUNCTION(BlueprintPure)
	FString GetPlayerGame(FApPlayer player) const;

	UFUNCTION(BlueprintPure)
	int GetPlayerCount() const;

	UFUNCTION(BlueprintPure)
	TSet<int> GetTeams() const;

	UFUNCTION(BlueprintPure)
	TArray<FApPlayer> GetAllPlayers() const;

	 UFUNCTION(BlueprintPure)
	 TArray<FString> GetAllGames() const;

	 UFUNCTION(BlueprintPure)
	 TArray<FApPlayer> GetAllPlayersPlayingGame(FString game) const;
	 */

private:
	/*void InitializeData(TArray<FReplicatedFApPlayerInfo> playerInfos);

	void SendInitialReplicationDataForAllClients();
	void SendInitialReplicationData(const APlayerController* PlayerController);

	void UpdateReplicationDataForAllClients(const TArray<FReplicatedFApPlayerInfo>& playerInfosToUpdate) const;
	void SendUpdatedReplicationData(APlayerController* PlayerController, const TArray<FReplicatedFApPlayerInfo> playerInfos) const;
	*/

	//
	// called from Player Controller mixin
	//
public:
	UFUNCTION(BlueprintCallable)
	void OnPlayerControllerBeginPlay(const APlayerController* PlayerController);

protected:
	/** Handles a reliable message */
	void OnRawDataReceived(TArray<uint8>&& InMessageData);
	/** Sends a a reliable message */
	void SendRawMessage(const APlayerController* PlayerController, EAdditionalDepotsReplicatorMessageId MessageId, const TFunctionRef<void(FArchive&)>& MessageSerializer) const;

	/** Handles Initial Replication Data for Player Info Subsystem */
	//void ReceiveInitialReplicationData(const FPlayerInfoSubsystemInitialReplicationMessage& Message);
	//void ReceiveInitialReplicationData(const FPlayerInfoSubsystemUpdateReplicationMessage& Message);
	//
	//
	//
};
