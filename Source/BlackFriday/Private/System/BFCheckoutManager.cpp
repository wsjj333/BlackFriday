// Fill out your copyright notice in the Description page of Project Settings.

#include "System/BFCheckoutManager.h"
#include "System/BFCheckoutCounter.h"
#include "System/BFGameState.h"
#include "System/BFGameMode.h"

ABFCheckoutManager::ABFCheckoutManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false; // 서버 전용 관리 액터
}

void ABFCheckoutManager::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) return;

	// GameMode에 자신을 등록
	if (ABFGameMode* GM = GetWorld()->GetAuthGameMode<ABFGameMode>())
	{
		GM->RegisterCheckoutManager(this);
		UE_LOG(LogTemp, Log, TEXT("[BFCheckoutManager] Registered to GameMode (Counters: %d)"), Counters.Num());
	}
}

void ABFCheckoutManager::ProcessCheckoutsAndDeactivate(int32 CurrentRound)
{
	if (!HasAuthority()) return;

	ABFGameState* GS = GetWorld()->GetGameState<ABFGameState>();

	// 1. 활성 카운터 전체 결제 처리
	for (ABFCheckoutCounter* Counter : Counters)
	{
		if (!IsValid(Counter) || !Counter->IsCounterActive()) continue;

		uint8 TeamId = Counter->GetOccupyingTeamId();
		if (TeamId == 255) continue;

		float Amount = Counter->ProcessCheckout();

		if (GS && Amount > 0.0f)
		{
			GS->AddTeamPayment(TeamId, Amount);
		}

		// 라운드 전환을 위해 점유 초기화
		Counter->ResetOccupation();
	}

	// 2. 마지막 라운드가 아니면 카운터 1개 비활성화
	//    1라운드 종료: 4→3개, 2라운드 종료: 3→2개
	if (GS && CurrentRound < GS->GetMaxRounds())
	{
		for (int32 i = Counters.Num() - 1; i >= 0; --i)
		{
			if (IsValid(Counters[i]) && Counters[i]->IsCounterActive())
			{
				Counters[i]->Deactivate();
				UE_LOG(LogTemp, Log, TEXT("[BFCheckoutManager] Counter[%d] deactivated after Round %d (Active: %d)"),
					i, CurrentRound, GetActiveCounterCount());
				break;
			}
		}
	}

	OnAllCheckoutsProcessed.Broadcast(CurrentRound);
}

int32 ABFCheckoutManager::GetActiveCounterCount() const
{
	int32 Count = 0;
	for (const ABFCheckoutCounter* Counter : Counters)
	{
		if (IsValid(Counter) && Counter->IsCounterActive())
		{
			Count++;
		}
	}
	return Count;
}
