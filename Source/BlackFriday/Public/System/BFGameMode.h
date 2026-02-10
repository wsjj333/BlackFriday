// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BFGameState.h"
#include "BFGameMode.generated.h"

class ABFGameState;

// 델리게이트 - 게임 이벤트 콜백
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoundStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundEnded, int32, WinningTeam);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllPlayersReady);

UCLASS()
class BLACKFRIDAY_API ABFGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABFGameMode();

	// ===== 오버라이드 =====
	virtual void BeginPlay() override;
	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	// ===== 게임 흐름 제어 (서버에서만 호출) =====

	// 게임 시작 조건 체크 및 자동 시작
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	void CheckAndStartGame();

	// 호스트가 게임 시작 (마트 레벨로 이동)
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	void HostStartGame();

	// 카운트다운 시작 (수동 호출용)
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	void StartGameCountdown();

	// 라운드 시작
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	void StartRound();

	// 라운드 종료
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	void EndRound(int32 WinningTeam);

	// 다음 라운드로 진행
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	void AdvanceToNextRound();

	// 플레이어 레디 알림 (클라이언트에서 RPC로 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	void NotifyPlayerReady(APlayerController* Player);

	// 플레이어 레디 취소 (클라이언트에서 RPC로 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	void CancelPlayerReady(APlayerController* Player);

	// 플레이어가 레디 상태인지 확인
	UFUNCTION(BlueprintPure, Category = "BF|GameMode")
	bool IsPlayerReady(APlayerController* Player) const;

	// ===== 팀 관련 =====

	// 플레이어 팀 변경 요청 (클라이언트에서 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	void RequestChangeTeam(APlayerController* Player, uint8 NewTeamId);

	// 모든 플레이어가 팀을 선택했는지 확인
	UFUNCTION(BlueprintPure, Category = "BF|GameMode")
	bool HaveAllPlayersSelectedTeam() const;

	// 팀 개수 설정 (호스트만, 게임 시작 전에만)
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	void SetTeamCount(int32 NewTeamCount);

	// ===== 역할 관련 =====

	// 플레이어 역할 변경 요청 (클라이언트에서 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	bool RequestChangeRole(APlayerController* Player, EBFPlayerRole NewRole);

	// 모든 플레이어가 역할을 선택했는지 확인
	UFUNCTION(BlueprintPure, Category = "BF|GameMode")
	bool HaveAllPlayersSelectedRole() const;

	// ===== 이름 관련 =====

	// 플레이어 이름 설정 요청 (클라이언트에서 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	void RequestSetPlayerName(APlayerController* Player, const FString& NewName);

	// ===== 설정 =====

	// 게임 시작에 필요한 최소 플레이어 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BF|GameMode")
	int32 MinPlayersToStart = 2;

	// 게임 시작 카운트다운 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BF|GameMode")
	int32 StartCountdownSeconds = 5;

	// 라운드 시작 카운트다운 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BF|GameMode")
	int32 RoundCountdownSeconds = 3;

	// 자동 시작 활성화 (false = 호스트가 수동으로 시작)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BF|GameMode")
	bool bAutoStartWhenReady = false;

	// ===== 델리게이트 =====

	UPROPERTY(BlueprintAssignable, Category = "BF|GameMode")
	FOnGameStarted OnGameStarted;

	UPROPERTY(BlueprintAssignable, Category = "BF|GameMode")
	FOnRoundStarted OnRoundStarted;

	UPROPERTY(BlueprintAssignable, Category = "BF|GameMode")
	FOnRoundEnded OnRoundEnded;

	UPROPERTY(BlueprintAssignable, Category = "BF|GameMode")
	FOnAllPlayersReady OnAllPlayersReady;

	// ===== Getter =====

	UFUNCTION(BlueprintPure, Category = "BF|GameMode")
	int32 GetConnectedPlayerCount() const { return ConnectedPlayers.Num(); }

	UFUNCTION(BlueprintPure, Category = "BF|GameMode")
	int32 GetReadyPlayerCount() const { return ReadyPlayers.Num(); }

	UFUNCTION(BlueprintPure, Category = "BF|GameMode")
	bool IsWaitingForPlayers() const { return bWaitingForPlayers; }

	// ===== 마트 맵 스폰용 헬퍼 함수 =====

	// 팀의 특정 역할 플레이어 컨트롤러 가져오기 (UniqueNetId로 매칭)
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	APlayerController* GetPlayerControllerByRoleInTeam(uint8 TeamId, EBFPlayerRole InRole);

	// 팀의 모든 플레이어 컨트롤러 가져오기 (UniqueNetId로 매칭)
	UFUNCTION(BlueprintCallable, Category = "BF|GameMode")
	TArray<APlayerController*> GetAllPlayerControllersInTeam(uint8 TeamId);

	// 현재 접속한 모든 플레이어 컨트롤러 가져오기
	UFUNCTION(BlueprintPure, Category = "BF|GameMode")
	TArray<APlayerController*> GetAllConnectedPlayerControllers() const;

protected:
	// GameState 캐시
	UPROPERTY()
	TObjectPtr<ABFGameState> BFGameState;

	// 접속한 플레이어 목록
	UPROPERTY()
	TArray<TObjectPtr<APlayerController>> ConnectedPlayers;

	// 로딩 완료한 플레이어 목록
	UPROPERTY()
	TSet<TObjectPtr<APlayerController>> ReadyPlayers;

	// 모든 플레이어 준비 완료 여부
	bool AreAllPlayersReady() const;

	// 카운트다운 완료 콜백 (GameState에서 호출됨)
	UFUNCTION()
	void HandleCountdownFinished();

	// 게임 시작 대기 체크
	bool bWaitingForPlayers = true;
};
