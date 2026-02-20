// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Data/BFGameTypes.h"
#include "BFGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCountdownChanged, int32, RemainingSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGamePhaseChanged, EBFGamePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundChanged, int32, NewRound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCountdownFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerTeamChanged, int32, PlayerId, uint8, NewTeamId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamCountChanged, int32, NewTeamCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerRoleChanged, int32, PlayerId, EBFPlayerRole, NewRole);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTeamPaymentsChanged);

UCLASS()
class BLACKFRIDAY_API ABFGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ABFGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ===== 게임 진행 (서버 전용) =====

	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void StartCountdown(int32 Seconds);

	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void StopCountdown();

	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void SetGamePhase(EBFGamePhase NewPhase);

	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void SetCurrentRound(int32 NewRound);

	// ===== 게임 진행 Getter =====

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	int32 GetCountdownTime() const { return CountdownTime; }

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	EBFGamePhase GetGamePhase() const { return GamePhase; }

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	int32 GetCurrentRound() const { return CurrentRound; }

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	int32 GetMaxRounds() const { return MaxRounds; }

	// ===== 팀/플레이어 관리 (서버 전용) =====

	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void SetPlayerTeam(int32 PlayerId, uint8 TeamId);

	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void RemovePlayerTeamInfo(int32 PlayerId);

	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void SetTeamCount(int32 NewTeamCount);

	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	bool SetPlayerRole(int32 PlayerId, EBFPlayerRole NewRole);

	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void SetPlayerName(int32 PlayerId, const FString& NewName);

	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void SetPlayerUniqueNetId(int32 PlayerId, const FString& UniqueNetId);

	// ===== 팀/플레이어 Getter =====

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	uint8 GetPlayerTeam(int32 PlayerId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	TArray<int32> GetPlayersInTeam(uint8 TeamId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	int32 GetTeamPlayerCount(uint8 TeamId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	bool HasPlayerSelectedTeam(int32 PlayerId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	int32 GetTotalTeamCount() const { return TeamCount; }

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	int32 GetMaxPlayersPerTeam() const { return MaxPlayersPerTeam; }

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	bool IsTeamFull(uint8 TeamId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	EBFSpawnLocation GetTeamSpawnLocation(uint8 TeamId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	TArray<FBFTeamSettings> GetAllTeamSettings() const { return TeamSettings; }

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	FString GetPlayerName(int32 PlayerId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	TArray<FBFPlayerTeamInfo> GetAllPlayerInfos() const { return PlayerTeamInfos; }

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	bool HasPlayerInfo(int32 PlayerId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	EBFPlayerRole GetPlayerRole(int32 PlayerId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	bool HasPlayerSelectedRole(int32 PlayerId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	bool IsRoleAvailableInTeam(uint8 TeamId, EBFPlayerRole InRole) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	int32 GetPlayerWithRoleInTeam(uint8 TeamId, EBFPlayerRole InRole) const;

	// ===== 결제 관리 (서버 전용) =====

	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void AddTeamPayment(uint8 TeamId, float Amount);

	// ===== 결제 Getter =====

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	float GetTeamTotalPayment(uint8 TeamId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	TArray<FBFTeamPaymentRecord> GetAllTeamPayments() const { return TeamPayments; }

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	uint8 GetWinningTeam() const;

	// ===== 델리게이트 =====

	UPROPERTY(BlueprintAssignable, Category = "BF|GameState")
	FOnCountdownChanged OnCountdownChanged;

	UPROPERTY(BlueprintAssignable, Category = "BF|GameState")
	FOnGamePhaseChanged OnGamePhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "BF|GameState")
	FOnRoundChanged OnRoundChanged;

	UPROPERTY(BlueprintAssignable, Category = "BF|GameState")
	FOnCountdownFinished OnCountdownFinished;

	UPROPERTY(BlueprintAssignable, Category = "BF|GameState")
	FOnPlayerTeamChanged OnPlayerTeamChanged;

	UPROPERTY(BlueprintAssignable, Category = "BF|GameState")
	FOnTeamCountChanged OnTeamCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "BF|GameState")
	FOnPlayerRoleChanged OnPlayerRoleChanged;

	UPROPERTY(BlueprintAssignable, Category = "BF|GameState")
	FOnTeamPaymentsChanged OnTeamPaymentsChanged;

protected:
	// ===== Replicated 속성 =====

	UPROPERTY(ReplicatedUsing = OnRep_CountdownTime, BlueprintReadOnly, Category = "BF|GameState")
	int32 CountdownTime = 0;

	UPROPERTY(ReplicatedUsing = OnRep_GamePhase, BlueprintReadOnly, Category = "BF|GameState")
	EBFGamePhase GamePhase = EBFGamePhase::Waiting;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentRound, BlueprintReadOnly, Category = "BF|GameState")
	int32 CurrentRound = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BF|GameState")
	int32 MaxRounds = 3;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerTeamInfos, BlueprintReadOnly, Category = "BF|GameState")
	TArray<FBFPlayerTeamInfo> PlayerTeamInfos;

	UPROPERTY(ReplicatedUsing = OnRep_TeamCount, BlueprintReadOnly, Category = "BF|GameState")
	int32 TeamCount = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BF|GameState")
	int32 MaxPlayersPerTeam = 2;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "BF|GameState")
	TArray<FBFTeamSettings> TeamSettings;

	UPROPERTY(ReplicatedUsing = OnRep_TeamPayments, BlueprintReadOnly, Category = "BF|GameState")
	TArray<FBFTeamPaymentRecord> TeamPayments;

	// ===== OnRep 함수 =====

	UFUNCTION() void OnRep_CountdownTime();
	UFUNCTION() void OnRep_GamePhase();
	UFUNCTION() void OnRep_CurrentRound();
	UFUNCTION() void OnRep_PlayerTeamInfos();
	UFUNCTION() void OnRep_TeamCount();
	UFUNCTION() void OnRep_TeamPayments();

private:
	FTimerHandle CountdownTimerHandle;
	void HandleCountdownTick();
};
