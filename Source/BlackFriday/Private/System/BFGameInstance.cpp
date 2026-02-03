// Fill out your copyright notice in the Description page of Project Settings.

#include "System/BFGameInstance.h"

UBFGameInstance::UBFGameInstance()
{
}

void UBFGameInstance::RecordRoundResult(int32 RoundNumber, int32 WinningTeam)
{
	FBFRoundResult Result;
	Result.RoundNumber = RoundNumber;
	Result.WinningTeam = WinningTeam;
	Result.TeamScores = TotalTeamScores;

	RoundResults.Add(Result);

	// 승리 횟수 증가
	int32& WinCount = TeamRoundWins.FindOrAdd(WinningTeam);
	WinCount++;

	UE_LOG(LogTemp, Log, TEXT("[BFGameInstance] Round %d result recorded. Winner: Team %d"),
		RoundNumber, WinningTeam);
}

void UBFGameInstance::AddTeamScore(int32 TeamId, int32 Score)
{
	int32& CurrentScore = TotalTeamScores.FindOrAdd(TeamId);
	CurrentScore += Score;
}

int32 UBFGameInstance::GetTeamTotalScore(int32 TeamId) const
{
	const int32* Score = TotalTeamScores.Find(TeamId);
	return Score ? *Score : 0;
}

int32 UBFGameInstance::GetTeamRoundWins(int32 TeamId) const
{
	const int32* Wins = TeamRoundWins.Find(TeamId);
	return Wins ? *Wins : 0;
}

void UBFGameInstance::ResetGameData()
{
	RoundResults.Empty();
	TotalTeamScores.Empty();
	TeamRoundWins.Empty();

	UE_LOG(LogTemp, Log, TEXT("[BFGameInstance] Game data reset."));
}

int32 UBFGameInstance::GetOverallWinner() const
{
	int32 WinnerTeam = 0;
	int32 MaxWins = 0;

	for (const auto& Pair : TeamRoundWins)
	{
		if (Pair.Value > MaxWins)
		{
			MaxWins = Pair.Value;
			WinnerTeam = Pair.Key;
		}
	}

	return WinnerTeam;
}
