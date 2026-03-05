// Fill out your copyright notice in the Description page of Project Settings.

#include "System/BFGameMode.h"
#include "System/BFGameState.h"
#include "System/BFGameInstance.h"
#include "System/BFCheckoutManager.h"
#include "System/BFPlayerController.h"
#include "GameFramework/PlayerState.h"

void ABFGameMode::NotifyPlayerReady(APlayerController* Player)
{
	if (!Player) return;

	ReadyPlayers.Add(Player);

	if (BFGameState) BFGameState->SetReadyPlayerCount(ReadyPlayers.Num());

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Player ready. Ready: %d / Connected: %d"),
		ReadyPlayers.Num(), ConnectedPlayers.Num());

	if (AreAllPlayersReady())
	{
		OnAllPlayersReady.Broadcast();

		if (bAutoStartWhenReady && bWaitingForPlayers)
		{
			StartGameCountdown();
		}
	}
}

void ABFGameMode::CancelPlayerReady(APlayerController* Player)
{
	if (!Player) return;

	ReadyPlayers.Remove(Player);

	if (BFGameState) BFGameState->SetReadyPlayerCount(ReadyPlayers.Num());

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Player cancelled ready. Ready: %d / Connected: %d"),
		ReadyPlayers.Num(), ConnectedPlayers.Num());
}

bool ABFGameMode::IsPlayerReady(APlayerController* Player) const
{
	return Player && ReadyPlayers.Contains(Player);
}

bool ABFGameMode::AreAllPlayersReady() const
{
	// 1. 인원수 체크
	if (ConnectedPlayers.Num() < MinPlayersToStart)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] 시작 불가: 인원 부족. 현재: %d / 최소: %d"),
			ConnectedPlayers.Num(), MinPlayersToStart);
		return false;
	}

	// 2. Ready 체크
	for (const TObjectPtr<APlayerController>& PC : ConnectedPlayers)
	{
		if (!ReadyPlayers.Contains(PC))
		{
			FString PCName = PC ? PC->GetName() : TEXT("Unknown");
			UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] 시작 불가: 플레이어(%s)가 준비(Ready) 상태가 아님."), *PCName);
			return false;
		}
	}

	// 3. 팀 선택 체크
	if (!HaveAllPlayersSelectedTeam())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] 시작 불가: 팀을 선택하지 않은 플레이어가 있음!"));

		if (BFGameState)
		{
			for (const auto& PC : ConnectedPlayers)
			{
				if (PC && PC->PlayerState)
				{
					int32 PID = PC->PlayerState->GetPlayerId();
					if (BFGameState->GetPlayerTeam(PID) == 255)
					{
						UE_LOG(LogTemp, Warning, TEXT(" -> [범인] Player %d (%s) : 팀 없음 (255)"), PID, *PC->GetName());
					}
				}
			}
		}
		return false;
	}

	// 4. 역할 선택 체크
	if (!HaveAllPlayersSelectedRole())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] 시작 불가: 직업(Role)을 선택하지 않은 플레이어가 있음!"));

		if (BFGameState)
		{
			for (const auto& PC : ConnectedPlayers)
			{
				if (PC && PC->PlayerState)
				{
					int32 PID = PC->PlayerState->GetPlayerId();
					if (BFGameState->GetPlayerRole(PID) == EBFPlayerRole::None)
					{
						UE_LOG(LogTemp, Warning, TEXT(" -> [범인] Player %d (%s) : 직업 없음 (None)"), PID, *PC->GetName());
					}
				}
			}
		}
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] 모든 조건 만족! 게임 카운트다운 진입 가능."));
	return true;
}

void ABFGameMode::CheckAndStartGame()
{
	if (!bIsLobby) return;

	if (AreAllPlayersReady())
	{
		StartGameCountdown();
	}
}

void ABFGameMode::HostStartGame()
{
	if (!AreAllPlayersReady())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] Cannot start game - not all players ready!"));
		UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] Connected: %d, MinRequired: %d"), ConnectedPlayers.Num(), MinPlayersToStart);
		return;
	}

	// GameInstance에 로비 데이터 저장 (레벨 이동 후에도 유지)
	if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
	{
		if (BFGameState)
		{
			GI->SaveLobbyData(BFGameState->GetAllPlayerInfos(), BFGameState->GetTotalTeamCount());
			UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Lobby data saved to GameInstance"));
		}
	}

	// 모든 클라이언트에 로딩 화면 표시 요청
	UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] HostStartGame - ConnectedPlayers count: %d"), ConnectedPlayers.Num());
	for (APlayerController* PC : ConnectedPlayers)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameMode]   -> PC: %s, Class: %s, IsLocal: %s"),
			*PC->GetName(), *PC->GetClass()->GetName(),
			PC->IsLocalController() ? TEXT("true") : TEXT("false"));

		if (ABFPlayerController* BFPC = Cast<ABFPlayerController>(PC))
		{
			BFPC->ClientShowLoadingScreen();
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[BFGameMode]   -> Cast to ABFPlayerController FAILED!"));
		}
	}

	// RPC가 클라이언트에 도달할 시간 확보 후 레벨 이동
	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Host starting game - traveling to Mart level in 0.5s"));
	FTimerHandle TravelTimer;
	GetWorld()->GetTimerManager().SetTimer(TravelTimer, [this]()
	{
		GetWorld()->ServerTravel(TEXT("/Game/Colab/JJS/Map/Map?listen"));
	}, 0.5f, false);
}

void ABFGameMode::StartGameCountdown()
{
	if (!BFGameState) return;

	bWaitingForPlayers = false;
	BFGameState->SetGamePhase(EBFGamePhase::Countdown);
	BFGameState->StartCountdown(StartCountdownSeconds);

	// 모든 클라이언트에 로딩 화면 제거 요청
	for (APlayerController* PC : ConnectedPlayers)
	{
		if (ABFPlayerController* BFPC = Cast<ABFPlayerController>(PC))
		{
			BFPC->ClientHideLoadingScreen();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Game countdown started: %d seconds"), StartCountdownSeconds);
}

void ABFGameMode::HandleCountdownFinished()
{
	if (!BFGameState) return;

	EBFGamePhase CurrentPhase = BFGameState->GetGamePhase();

	if (CurrentPhase == EBFGamePhase::Countdown)
	{
		// 게임 시작 카운트다운 완료 → 라운드 시작
		UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Start countdown finished! Starting round."));
		OnGameStarted.Broadcast();
		StartRound();
	}
	else if (CurrentPhase == EBFGamePhase::Playing)
	{
		// 라운드 플레이 타임 완료 → 라운드 종료
		UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Round time over!"));
		EndRound(-1); // -1 = 타임오버 (승자 없음)
	}
}

void ABFGameMode::StartRound()
{
	if (!BFGameState) return;

	BFGameState->SetGamePhase(EBFGamePhase::Playing);
	BFGameState->StartCountdown(RoundPlayTimeSeconds);

	OnRoundStarted.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Round %d started! Play time: %d seconds"),
		BFGameState->GetCurrentRound(), RoundPlayTimeSeconds);

	if (CheckoutManager)
	{
		CheckoutManager->LogActiveCounters(BFGameState->GetCurrentRound());
	}
}

void ABFGameMode::RegisterCheckoutManager(ABFCheckoutManager* Manager)
{
	CheckoutManager = Manager;
	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] CheckoutManager registered"));
}

void ABFGameMode::EndRound(int32 WinningTeam)
{
	if (!BFGameState) return;

	// 1. 페이즈 변경
	BFGameState->SetGamePhase(EBFGamePhase::RoundEnd);

	// 2. 결제 처리 먼저 (GameState에 팀별 금액 누적)
	if (CheckoutManager)
	{
		CheckoutManager->ProcessCheckoutsAndDeactivate(BFGameState->GetCurrentRound());
	}

	// 3. 이번 라운드 결제 금액 기준 우승팀 결정 (타임오버 시 -1로 들어옴)
	int32 FinalWinner = WinningTeam;
	if (WinningTeam == -1)
	{
		uint8 WinnerFromPayment = BFGameState->GetRoundWinningTeam();
		if (WinnerFromPayment != 255)
		{
			FinalWinner = WinnerFromPayment;
		}
	}

	// 4. 결과 기록
	if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
	{
		GI->RecordRoundResult(BFGameState->GetCurrentRound(), FinalWinner);
	}

	// 5. 5초 뒤 다음 단계 진행
	GetWorld()->GetTimerManager().SetTimer(
		RoundTransitionTimerHandle,
		this,
		&ABFGameMode::AdvanceToNextRound,
		5.0f,
		false
	);

	bool bIsLastRound = BFGameState->GetCurrentRound() >= BFGameState->GetMaxRounds();
	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Round %d 끝! %s"),
		BFGameState->GetCurrentRound(),
		bIsLastRound ? TEXT("(마지막 라운드) 5초 뒤 게임 종료...") : TEXT("5초 뒤 다음 라운드 진행..."));
}

void ABFGameMode::AdvanceToNextRound()
{
	if (!BFGameState) return;

	int32 NextRound = BFGameState->GetCurrentRound() + 1;

	if (NextRound > BFGameState->GetMaxRounds())
	{
		BFGameState->SetGamePhase(EBFGamePhase::GameEnd);

		// 최종 결과 로그
		if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
		{
			int32 Winner = GI->GetPaymentWinner();
			UE_LOG(LogTemp, Log, TEXT("[BFGameMode] ===== 게임 종료 결과 ====="));

			for (const FBFRoundResult& Result : GI->GetAllRoundResults())
			{
				UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Round %d 우승: Team %d"),
					Result.RoundNumber, Result.WinningTeam);
			}

			for (int32 i = 0; i < 8; i++)
			{
				float Total = GI->GetTotalPayment(i);
				if (Total > 0.0f)
				{
					UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Team %d 누적 결제: %.0f원"), i, Total);
				}
			}

			UE_LOG(LogTemp, Log, TEXT("[BFGameMode] 최종 우승: Team %d"), Winner);
			UE_LOG(LogTemp, Log, TEXT("[BFGameMode] ========================="));
		}
	}
	else
	{
		// GameInstance에 다음 라운드 번호 저장 후 레벨 재시작
		if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
		{
			GI->SetNextRound(NextRound);
		}

		UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Restarting level for Round %d..."), NextRound);
		GetWorld()->ServerTravel(GetWorld()->GetName() + TEXT("?listen"), false);
	}
}
