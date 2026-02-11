// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "BFGameState.h"
#include "BFGameInstance.generated.h"

// 라운드 결과 구조체
USTRUCT(BlueprintType)
struct FBFRoundResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BF|Game")
	int32 RoundNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BF|Game")
	int32 WinningTeam = 0;

	// 추가 데이터 (팀별 점수 등)
	UPROPERTY(BlueprintReadOnly, Category = "BF|Game")
	TMap<int32, int32> TeamScores;
};

UCLASS()
class BLACKFRIDAY_API UBFGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UBFGameInstance();

	// ===== 라운드 결과 관리 =====

	UFUNCTION(BlueprintCallable, Category = "BF|Game")
	void RecordRoundResult(int32 RoundNumber, int32 WinningTeam);

	UFUNCTION(BlueprintCallable, Category = "BF|Game")
	void AddTeamScore(int32 TeamId, int32 Score);

	UFUNCTION(BlueprintPure, Category = "BF|Game")
	int32 GetTeamTotalScore(int32 TeamId) const;

	UFUNCTION(BlueprintPure, Category = "BF|Game")
	int32 GetTeamRoundWins(int32 TeamId) const;

	UFUNCTION(BlueprintPure, Category = "BF|Game")
	TArray<FBFRoundResult> GetAllRoundResults() const { return RoundResults; }

	UFUNCTION(BlueprintCallable, Category = "BF|Game")
	void ResetGameData();

	// ===== 로컬 플레이어 정보 (세션 접속 전 임시 저장) =====

	UPROPERTY(BlueprintReadWrite, Category = "BF|Player")
	FString LocalPlayerName;

	UFUNCTION(BlueprintCallable, Category = "BF|Player")
	void SetLocalPlayerName(const FString& NewName) { LocalPlayerName = NewName; }

	UFUNCTION(BlueprintPure, Category = "BF|Player")
	FString GetLocalPlayerName() const { return LocalPlayerName; }

	// ===== 최종 승자 결정 =====

	UFUNCTION(BlueprintPure, Category = "BF|Game")
	int32 GetOverallWinner() const;

	// ===== 세션 생성 시 설정 (로비 진입 전) =====

	// 세션 생성 시 팀 개수 설정 (로비에서 불러와서 사용)
	UPROPERTY(BlueprintReadWrite, Category = "BF|Session")
	int32 PendingTeamCount = 4;

	UFUNCTION(BlueprintCallable, Category = "BF|Session")
	void SetPendingTeamCount(int32 InTeamCount) { PendingTeamCount = FMath::Clamp(InTeamCount, 1, 8); }

	UFUNCTION(BlueprintPure, Category = "BF|Session")
	int32 GetPendingTeamCount() const { return PendingTeamCount; }

	// ===== 로비 데이터 저장 (레벨 이동 시 유지) =====

	UFUNCTION(BlueprintCallable, Category = "BF|Lobby")
	void SaveLobbyData(const TArray<FBFPlayerTeamInfo>& PlayerInfos, int32 TeamCount);

	UFUNCTION(BlueprintPure, Category = "BF|Lobby")
	TArray<FBFPlayerTeamInfo> GetSavedPlayerInfos() const { return SavedPlayerInfos; }

	UFUNCTION(BlueprintPure, Category = "BF|Lobby")
	int32 GetSavedTeamCount() const { return SavedTeamCount; }

	UFUNCTION(BlueprintPure, Category = "BF|Lobby")
	FBFPlayerTeamInfo GetSavedPlayerInfo(int32 PlayerId) const;

	// UniqueNetId로 저장된 플레이어 정보 찾기 (레벨 이동 후 사용)
	UFUNCTION(BlueprintPure, Category = "BF|Lobby")
	FBFPlayerTeamInfo GetSavedPlayerInfoByUniqueId(const FString& UniqueNetId) const;

	// PlayerController에서 UniqueNetId 문자열 가져오기 (Blueprint용 헬퍼)
	UFUNCTION(BlueprintPure, Category = "BF|Lobby")
	static FString GetUniqueNetIdFromPlayer(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "BF|Lobby")
	void ClearLobbyData();

protected:
	// 라운드별 결과 저장
	UPROPERTY()
	TArray<FBFRoundResult> RoundResults;

	// 팀별 누적 점수
	UPROPERTY()
	TMap<int32, int32> TotalTeamScores;

	// 팀별 라운드 승리 횟수
	UPROPERTY()
	TMap<int32, int32> TeamRoundWins;

	// ===== 로비 데이터 (레벨 이동 시 유지) =====

	UPROPERTY()
	TArray<FBFPlayerTeamInfo> SavedPlayerInfos;

	UPROPERTY()
	int32 SavedTeamCount = 4;
};
