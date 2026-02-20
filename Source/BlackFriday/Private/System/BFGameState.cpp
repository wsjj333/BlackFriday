// Fill out your copyright notice in the Description page of Project Settings.
// 게임 진행 관련: 카운트다운 / 페이즈 / 라운드

#include "System/BFGameState.h"
#include "Net/UnrealNetwork.h"

ABFGameState::ABFGameState()
{
	// 기본 팀 설정 초기화 (4팀: 북/남/동/서)
	TeamSettings.Add(FBFTeamSettings(0, EBFSpawnLocation::North));
	TeamSettings.Add(FBFTeamSettings(1, EBFSpawnLocation::South));
	TeamSettings.Add(FBFTeamSettings(2, EBFSpawnLocation::East));
	TeamSettings.Add(FBFTeamSettings(3, EBFSpawnLocation::West));
}

void ABFGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABFGameState, CountdownTime);
	DOREPLIFETIME(ABFGameState, GamePhase);
	DOREPLIFETIME(ABFGameState, CurrentRound);
	DOREPLIFETIME(ABFGameState, PlayerTeamInfos);
	DOREPLIFETIME(ABFGameState, TeamCount);
	DOREPLIFETIME(ABFGameState, TeamSettings);
	DOREPLIFETIME(ABFGameState, TeamPayments);
}

void ABFGameState::StartCountdown(int32 Seconds)
{
	if (!HasAuthority()) return;

	CountdownTime = Seconds;
	GetWorld()->GetTimerManager().SetTimer(CountdownTimerHandle, this, &ABFGameState::HandleCountdownTick, 1.0f, true);
	OnRep_CountdownTime();
}

void ABFGameState::StopCountdown()
{
	if (!HasAuthority()) return;

	GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
	CountdownTime = 0;
	OnRep_CountdownTime();
}

void ABFGameState::HandleCountdownTick()
{
	if (!HasAuthority()) return;

	CountdownTime--;

	if (CountdownTime <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
		OnCountdownFinished.Broadcast();
	}

	OnRep_CountdownTime();
}

void ABFGameState::OnRep_CountdownTime()
{
	OnCountdownChanged.Broadcast(CountdownTime);

	// 클라이언트에서만 Finished 발동 (서버는 HandleCountdownTick에서 직접 호출)
	if (CountdownTime <= 0 && !HasAuthority())
	{
		OnCountdownFinished.Broadcast();
	}
}

void ABFGameState::SetGamePhase(EBFGamePhase NewPhase)
{
	if (!HasAuthority()) return;

	GamePhase = NewPhase;
	OnRep_GamePhase();
}

void ABFGameState::SetCurrentRound(int32 NewRound)
{
	if (!HasAuthority()) return;

	CurrentRound = FMath::Clamp(NewRound, 1, MaxRounds);
	OnRep_CurrentRound();
}

void ABFGameState::OnRep_GamePhase()
{
	OnGamePhaseChanged.Broadcast(GamePhase);
}

void ABFGameState::OnRep_CurrentRound()
{
	OnRoundChanged.Broadcast(CurrentRound);
}
