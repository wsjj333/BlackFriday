// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
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
};
