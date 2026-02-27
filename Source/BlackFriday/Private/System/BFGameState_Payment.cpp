// Fill out your copyright notice in the Description page of Project Settings.
// 결제 기록 관리

#include "System/BFGameState.h"

void ABFGameState::AddTeamPayment(uint8 TeamId, float Amount)
{
	if (!HasAuthority()) return;

	for (FBFTeamPaymentRecord& Record : TeamPayments)
	{
		if (Record.TeamId == TeamId)
		{
			Record.RoundPayment = Amount;
			Record.TotalPayment += Amount;
			OnRep_TeamPayments();
			UE_LOG(LogTemp, Log, TEXT("[BFGameState] Team %d +%.0f원 (누적: %.0f원)"), TeamId, Amount, Record.TotalPayment);
			return;
		}
	}

	FBFTeamPaymentRecord NewRecord;
	NewRecord.TeamId = TeamId;
	NewRecord.RoundPayment = Amount;
	NewRecord.TotalPayment = Amount;
	TeamPayments.Add(NewRecord);
	OnRep_TeamPayments();
	UE_LOG(LogTemp, Log, TEXT("[BFGameState] Team %d 첫 결제: %.0f원"), TeamId, Amount);
}

float ABFGameState::GetTeamTotalPayment(uint8 TeamId) const
{
	for (const FBFTeamPaymentRecord& Record : TeamPayments)
	{
		if (Record.TeamId == TeamId) return Record.TotalPayment;
	}
	return 0.0f;
}

uint8 ABFGameState::GetRoundWinningTeam() const
{
	uint8 WinnerTeamId = 255;
	float MaxPayment = -1.0f;

	for (const FBFTeamPaymentRecord& Record : TeamPayments)
	{
		if (Record.RoundPayment > MaxPayment)
		{
			MaxPayment = Record.RoundPayment;
			WinnerTeamId = Record.TeamId;
		}
	}
	return WinnerTeamId;
}

void ABFGameState::ResetRoundPayments()
{
	for (FBFTeamPaymentRecord& Record : TeamPayments)
	{
		Record.RoundPayment = 0.0f;
	}
}

uint8 ABFGameState::GetWinningTeam() const
{
	uint8 WinnerTeamId = 255;
	float MaxPayment = -1.0f;

	for (const FBFTeamPaymentRecord& Record : TeamPayments)
	{
		if (Record.TotalPayment > MaxPayment)
		{
			MaxPayment = Record.TotalPayment;
			WinnerTeamId = Record.TeamId;
		}
	}
	return WinnerTeamId;
}

void ABFGameState::OnRep_TeamPayments()
{
	OnTeamPaymentsChanged.Broadcast();
}
