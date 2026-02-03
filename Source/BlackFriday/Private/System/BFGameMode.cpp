// Fill out your copyright notice in the Description page of Project Settings.

#include "System/BFGameMode.h"
#include "System/BFGameState.h"
#include "System/BFGameInstance.h"
#include "System/BFPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

ABFGameMode::ABFGameMode()
{
	// GameState 클래스 지정
	GameStateClass = ABFGameState::StaticClass();

	// PlayerController 클래스 지정
	PlayerControllerClass = ABFPlayerController::StaticClass();

	// 기본 설정
	bStartPlayersAsSpectators = false;
}

void ABFGameMode::InitGameState()
{
	Super::InitGameState();

	BFGameState = GetGameState<ABFGameState>();

	if (BFGameState)
	{
		// 카운트다운 완료 델리게이트 바인딩
		BFGameState->OnCountdownFinished.AddDynamic(this, &ABFGameMode::HandleCountdownFinished);
	}
}

void ABFGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer)
	{
		ConnectedPlayers.AddUnique(NewPlayer);

		// GameInstance에서 로컬 플레이어 이름 가져와서 등록
		if (NewPlayer->PlayerState)
		{
			int32 PlayerId = NewPlayer->PlayerState->GetPlayerId();

			// 클라이언트의 GameInstance에서 이름을 가져오려면 RPC가 필요
			// 여기서는 일단 PlayerState 정보 등록만 해두고, 이름은 클라이언트가 RPC로 전달
			if (BFGameState)
			{
				BFGameState->SetPlayerTeam(PlayerId, 255); // 미선택 상태로 등록
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Player logged in. Total: %d"), ConnectedPlayers.Num());

		// 자동 시작 체크는 NotifyPlayerReady에서 수행
	}
}

void ABFGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (APlayerController* PC = Cast<APlayerController>(Exiting))
	{
		ConnectedPlayers.Remove(PC);
		ReadyPlayers.Remove(PC);

		// 팀 정보도 제거
		if (BFGameState && PC->PlayerState)
		{
			BFGameState->RemovePlayerTeamInfo(PC->PlayerState->GetPlayerId());
		}

		UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Player logged out. Remaining: %d"), ConnectedPlayers.Num());
	}
}

void ABFGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	// 여기서 Pawn 스폰 등 추가 처리 가능
}

void ABFGameMode::NotifyPlayerReady(APlayerController* Player)
{
	if (!Player)
	{
		return;
	}

	ReadyPlayers.Add(Player);

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

bool ABFGameMode::AreAllPlayersReady() const
{
	// 최소 인원 체크
	if (ConnectedPlayers.Num() < MinPlayersToStart)
	{
		return false;
	}

	// 모든 접속자가 Ready 상태인지 체크
	for (const TObjectPtr<APlayerController>& PC : ConnectedPlayers)
	{
		if (!ReadyPlayers.Contains(PC))
		{
			return false;
		}
	}

	// 모든 플레이어가 팀 선택했는지 체크
	if (!HaveAllPlayersSelectedTeam())
	{
		return false;
	}

	// 모든 플레이어가 역할 선택했는지 체크
	if (!HaveAllPlayersSelectedRole())
	{
		return false;
	}

	return true;
}

void ABFGameMode::CheckAndStartGame()
{
	if (AreAllPlayersReady())
	{
		StartGameCountdown();
	}
}

void ABFGameMode::HostStartGame()
{
	// 서버에서만 실행 (GameMode는 서버에만 존재하므로 추가 체크 불필요)

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Host starting game - traveling to Mart level"));

	// 마트 레벨로 ServerTravel
	GetWorld()->ServerTravel(TEXT("/Game/Colab/JJS/Map/Map?listen"));
}

void ABFGameMode::StartGameCountdown()
{
	if (!BFGameState)
	{
		return;
	}

	bWaitingForPlayers = false;
	BFGameState->StartCountdown(StartCountdownSeconds);

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Game countdown started: %d seconds"), StartCountdownSeconds);
}

void ABFGameMode::HandleCountdownFinished()
{
	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Countdown finished!"));

	// 게임 시작 이벤트 발동 (문 열림 등)
	OnGameStarted.Broadcast();

	// 라운드 시작
	StartRound();
}

void ABFGameMode::StartRound()
{
	if (!BFGameState)
	{
		return;
	}

	OnRoundStarted.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Round %d started!"), BFGameState->GetCurrentRound());
}

void ABFGameMode::EndRound(int32 WinningTeam)
{
	if (!BFGameState)
	{
		return;
	}

	BFGameState->SetGamePhase(EBFGamePhase::RoundEnd);
	OnRoundEnded.Broadcast(WinningTeam);

	// GameInstance에 결과 저장
	if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
	{
		GI->RecordRoundResult(BFGameState->GetCurrentRound(), WinningTeam);
	}

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Round %d ended. Winner: Team %d"),
		BFGameState->GetCurrentRound(), WinningTeam);
}

void ABFGameMode::AdvanceToNextRound()
{
	if (!BFGameState)
	{
		return;
	}

	int32 NextRound = BFGameState->GetCurrentRound() + 1;

	if (NextRound > BFGameState->GetMaxRounds())
	{
		// 모든 라운드 완료 - 게임 종료
		BFGameState->SetGamePhase(EBFGamePhase::GameEnd);
		UE_LOG(LogTemp, Log, TEXT("[BFGameMode] All rounds complete. Game ended."));
	}
	else
	{
		// 다음 라운드
		BFGameState->SetCurrentRound(NextRound);
		BFGameState->StartCountdown(RoundCountdownSeconds);
	}
}

void ABFGameMode::RequestChangeTeam(APlayerController* Player, uint8 NewTeamId)
{
	if (!Player || !BFGameState)
	{
		return;
	}

	if (!Player->PlayerState)
	{
		return;
	}

	int32 PlayerId = Player->PlayerState->GetPlayerId();

	// 팀 ID 유효성 체크
	if (NewTeamId >= BFGameState->GetTotalTeamCount())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] Invalid team ID: %d"), NewTeamId);
		return;
	}

	BFGameState->SetPlayerTeam(PlayerId, NewTeamId);

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Player %d changed to Team %d"), PlayerId, NewTeamId);

	// 자동 시작 조건 재체크
	if (bAutoStartWhenReady && bWaitingForPlayers && AreAllPlayersReady())
	{
		StartGameCountdown();
	}
}

bool ABFGameMode::HaveAllPlayersSelectedTeam() const
{
	if (!BFGameState)
	{
		return false;
	}

	for (const TObjectPtr<APlayerController>& PC : ConnectedPlayers)
	{
		if (PC && PC->PlayerState)
		{
			if (!BFGameState->HasPlayerSelectedTeam(PC->PlayerState->GetPlayerId()))
			{
				return false;
			}
		}
	}
	return true;
}

void ABFGameMode::SetTeamCount(int32 NewTeamCount)
{
	if (!BFGameState)
	{
		return;
	}

	BFGameState->SetTeamCount(NewTeamCount);
}

bool ABFGameMode::RequestChangeRole(APlayerController* Player, EBFPlayerRole NewRole)
{
	if (!Player || !BFGameState)
	{
		return false;
	}

	if (!Player->PlayerState)
	{
		return false;
	}

	int32 PlayerId = Player->PlayerState->GetPlayerId();

	bool bSuccess = BFGameState->SetPlayerRole(PlayerId, NewRole);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Player %d changed role to %d"), PlayerId, (uint8)NewRole);

		// 자동 시작 조건 재체크
		if (bAutoStartWhenReady && bWaitingForPlayers && AreAllPlayersReady())
		{
			StartGameCountdown();
		}
	}

	return bSuccess;
}

bool ABFGameMode::HaveAllPlayersSelectedRole() const
{
	if (!BFGameState)
	{
		return false;
	}

	for (const TObjectPtr<APlayerController>& PC : ConnectedPlayers)
	{
		if (PC && PC->PlayerState)
		{
			if (!BFGameState->HasPlayerSelectedRole(PC->PlayerState->GetPlayerId()))
			{
				return false;
			}
		}
	}
	return true;
}

void ABFGameMode::RequestSetPlayerName(APlayerController* Player, const FString& NewName)
{
	if (!Player || !BFGameState)
	{
		return;
	}

	if (!Player->PlayerState)
	{
		return;
	}

	int32 PlayerId = Player->PlayerState->GetPlayerId();
	BFGameState->SetPlayerName(PlayerId, NewName);

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Player %d name set to: %s"), PlayerId, *NewName);
}
