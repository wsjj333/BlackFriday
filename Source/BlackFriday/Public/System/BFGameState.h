// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BFGameState.generated.h"

// 게임 페이즈 열거형
UENUM(BlueprintType)
enum class EBFGamePhase : uint8
{
	Waiting      UMETA(DisplayName = "Waiting"),
	Countdown    UMETA(DisplayName = "Countdown"),
	Playing      UMETA(DisplayName = "Playing"),
	RoundEnd     UMETA(DisplayName = "RoundEnd"),
	GameEnd      UMETA(DisplayName = "GameEnd")
};

// 플레이어 역할 열거형
UENUM(BlueprintType)
enum class EBFPlayerRole : uint8
{
	None         UMETA(DisplayName = "None"),      // 미선택
	Pusher       UMETA(DisplayName = "Pusher"),    // 카트 미는 역할 (운전수)
	Gunner       UMETA(DisplayName = "Gunner")     // 총 쏘는 역할 (사수)
};

// 시작 위치 열거형
UENUM(BlueprintType)
enum class EBFSpawnLocation : uint8
{
	North        UMETA(DisplayName = "North"),     // 북쪽
	South        UMETA(DisplayName = "South"),     // 남쪽
	East         UMETA(DisplayName = "East"),      // 동쪽
	West         UMETA(DisplayName = "West")       // 서쪽
};

// 팀 설정 구조체
USTRUCT(BlueprintType)
struct FBFTeamSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	uint8 TeamId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	EBFSpawnLocation SpawnLocation = EBFSpawnLocation::North;

	FBFTeamSettings() {}
	FBFTeamSettings(uint8 InTeamId, EBFSpawnLocation InSpawnLocation)
		: TeamId(InTeamId), SpawnLocation(InSpawnLocation) {}
};

// 플레이어 팀 정보 구조체
USTRUCT(BlueprintType)
struct FBFPlayerTeamInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	int32 PlayerId = -1;

	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	uint8 TeamId = 255;  // 255 = 미선택

	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	EBFPlayerRole Role = EBFPlayerRole::None;

	// 레벨 이동 후에도 플레이어 식별용 (PlayerId는 레벨마다 바뀜)
	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	FString UniqueNetId;

	FBFPlayerTeamInfo() {}
	FBFPlayerTeamInfo(int32 InPlayerId, const FString& InPlayerName = TEXT(""), uint8 InTeamId = 255, EBFPlayerRole InRole = EBFPlayerRole::None, const FString& InUniqueNetId = TEXT(""))
		: PlayerId(InPlayerId), PlayerName(InPlayerName), TeamId(InTeamId), Role(InRole), UniqueNetId(InUniqueNetId) {}
};

// 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCountdownChanged, int32, RemainingSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGamePhaseChanged, EBFGamePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundChanged, int32, NewRound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCountdownFinished);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerTeamChanged, int32, PlayerId, uint8, NewTeamId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamCountChanged, int32, NewTeamCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerRoleChanged, int32, PlayerId, EBFPlayerRole, NewRole);

UCLASS()
class BLACKFRIDAY_API ABFGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ABFGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ===== 서버 전용 함수 =====

	// 카운트다운 시작 (서버에서만 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void StartCountdown(int32 Seconds);

	// 카운트다운 정지 (서버에서만 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void StopCountdown();

	// 게임 페이즈 변경 (서버에서만 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void SetGamePhase(EBFGamePhase NewPhase);

	// 라운드 변경 (서버에서만 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void SetCurrentRound(int32 NewRound);

	// 플레이어 팀 설정 (서버에서만 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void SetPlayerTeam(int32 PlayerId, uint8 TeamId);

	// 플레이어 팀 정보 제거 (서버에서만 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void RemovePlayerTeamInfo(int32 PlayerId);

	// 팀 개수 설정 (서버에서만 호출, 게임 시작 전에만)
	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void SetTeamCount(int32 NewTeamCount);

	// 플레이어 역할 설정 (서버에서만 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	bool SetPlayerRole(int32 PlayerId, EBFPlayerRole NewRole);

	// 플레이어 이름 설정 (서버에서만 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void SetPlayerName(int32 PlayerId, const FString& NewName);

	// 플레이어 UniqueNetId 설정 (서버에서만 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameState")
	void SetPlayerUniqueNetId(int32 PlayerId, const FString& UniqueNetId);

	// ===== Getter 함수 (모든 클라이언트) =====

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	int32 GetCountdownTime() const { return CountdownTime; }

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	EBFGamePhase GetGamePhase() const { return GamePhase; }

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	int32 GetCurrentRound() const { return CurrentRound; }

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	int32 GetMaxRounds() const { return MaxRounds; }

	// ===== 팀 관련 Getter =====

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

	// ===== 시작 위치 관련 Getter =====

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	EBFSpawnLocation GetTeamSpawnLocation(uint8 TeamId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	TArray<FBFTeamSettings> GetAllTeamSettings() const { return TeamSettings; }

	// ===== 이름 관련 Getter =====

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	FString GetPlayerName(int32 PlayerId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	TArray<FBFPlayerTeamInfo> GetAllPlayerInfos() const { return PlayerTeamInfos; }

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	bool HasPlayerInfo(int32 PlayerId) const;

	// ===== 역할 관련 Getter =====

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	EBFPlayerRole GetPlayerRole(int32 PlayerId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	bool HasPlayerSelectedRole(int32 PlayerId) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	bool IsRoleAvailableInTeam(uint8 TeamId, EBFPlayerRole InRole) const;

	UFUNCTION(BlueprintPure, Category = "BF|GameState")
	int32 GetPlayerWithRoleInTeam(uint8 TeamId, EBFPlayerRole InRole) const;

	// ===== 델리게이트 (Blueprint에서 바인딩 가능) =====

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

	// ===== 팀 관련 Replicated 속성 =====

	UPROPERTY(ReplicatedUsing = OnRep_PlayerTeamInfos, BlueprintReadOnly, Category = "BF|GameState")
	TArray<FBFPlayerTeamInfo> PlayerTeamInfos;

	UPROPERTY(ReplicatedUsing = OnRep_TeamCount, BlueprintReadOnly, Category = "BF|GameState")
	int32 TeamCount = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BF|GameState")
	int32 MaxPlayersPerTeam = 2;

	// 팀별 설정 (시작 위치 등)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "BF|GameState")
	TArray<FBFTeamSettings> TeamSettings;

	// ===== OnRep 함수 =====

	UFUNCTION()
	void OnRep_CountdownTime();

	UFUNCTION()
	void OnRep_GamePhase();

	UFUNCTION()
	void OnRep_CurrentRound();

	UFUNCTION()
	void OnRep_PlayerTeamInfos();

	UFUNCTION()
	void OnRep_TeamCount();

private:
	// 타이머 핸들
	FTimerHandle CountdownTimerHandle;

	// 타이머 콜백 (서버에서만 실행)
	void HandleCountdownTick();
};
