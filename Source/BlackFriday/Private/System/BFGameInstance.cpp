// Fill out your copyright notice in the Description page of Project Settings.

#include "System/BFGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

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
	TeamTotalPayments.Empty();
	RemainingCounters = 4;
	NextRound = 1;

	UE_LOG(LogTemp, Log, TEXT("[BFGameInstance] Game data reset."));
}

void UBFGameInstance::AddTotalPayment(int32 TeamId, float Amount)
{
	float& Total = TeamTotalPayments.FindOrAdd(TeamId);
	Total += Amount;
	UE_LOG(LogTemp, Log, TEXT("[BFGameInstance] Team %d 누적 결제: %.0f원"), TeamId, Total);
}

float UBFGameInstance::GetTotalPayment(int32 TeamId) const
{
	const float* Total = TeamTotalPayments.Find(TeamId);
	return Total ? *Total : 0.0f;
}

int32 UBFGameInstance::GetPaymentWinner() const
{
	int32 Winner = -1;
	float MaxPayment = -1.0f;
	for (const auto& Pair : TeamTotalPayments)
	{
		if (Pair.Value > MaxPayment)
		{
			MaxPayment = Pair.Value;
			Winner = Pair.Key;
		}
	}
	return Winner;
}

EBFRoundTheme UBFGameInstance::GetRoundTheme(int32 RoundNumber) const
{
	// RoundNumber는 1부터 시작
	int32 Index = RoundNumber - 1;
	if (RoundThemes.IsValidIndex(Index))
	{
		return RoundThemes[Index];
	}
	return EBFRoundTheme::Food;
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

void UBFGameInstance::SaveLobbyData(const TArray<FBFPlayerTeamInfo>& PlayerInfos, int32 TeamCount)
{
	SavedPlayerInfos = PlayerInfos;
	SavedTeamCount = TeamCount;

	UE_LOG(LogTemp, Log, TEXT("[BFGameInstance] SaveLobbyData - Saved %d players, %d teams"),
		SavedPlayerInfos.Num(), SavedTeamCount);

	for (const FBFPlayerTeamInfo& Info : SavedPlayerInfos)
	{
		UE_LOG(LogTemp, Log, TEXT("[BFGameInstance] - Player %d: Name=%s, Team=%d, Role=%d, UniqueNetId=%s"),
			Info.PlayerId, *Info.PlayerName, Info.TeamId, (uint8)Info.Role, *Info.UniqueNetId);
	}
}

FBFPlayerTeamInfo UBFGameInstance::GetSavedPlayerInfo(int32 PlayerId) const
{
	for (const FBFPlayerTeamInfo& Info : SavedPlayerInfos)
	{
		if (Info.PlayerId == PlayerId)
		{
			return Info;
		}
	}
	return FBFPlayerTeamInfo();
}

FBFPlayerTeamInfo UBFGameInstance::GetSavedPlayerInfoByUniqueId(const FString& UniqueNetId) const
{
	for (const FBFPlayerTeamInfo& Info : SavedPlayerInfos)
	{
		if (Info.UniqueNetId == UniqueNetId)
		{
			return Info;
		}
	}
	return FBFPlayerTeamInfo();
}

FString UBFGameInstance::GetUniqueNetIdFromPlayer(APlayerController* PlayerController)
{
	if (PlayerController && PlayerController->PlayerState)
	{
		return PlayerController->PlayerState->GetUniqueId().ToString();
	}
	return TEXT("");
}

void UBFGameInstance::ClearLobbyData()
{
	SavedPlayerInfos.Empty();
	SavedTeamCount = 4;

	UE_LOG(LogTemp, Log, TEXT("[BFGameInstance] Lobby data cleared."));
}
