// Fill out your copyright notice in the Description page of Project Settings.

#include "System/BFGameMode.h"
#include "System/BFGameState.h"
#include "System/BFGameInstance.h"
#include "System/BFPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

ABFGameMode::ABFGameMode()
{
	// GameState 클래스 지정
	GameStateClass = ABFGameState::StaticClass();

	// PlayerController 클래스 지정
	PlayerControllerClass = ABFPlayerController::StaticClass();

	// 기본 설정
	bStartPlayersAsSpectators = false;
}

void ABFGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] ========== BeginPlay (bIsLobby=%s) =========="),
		bIsLobby ? TEXT("true") : TEXT("false"));

	// ServerTravel 후 서버 플레이어(호스트)는 PostLogin이 호출되지 않으므로 직접 등록
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && !ConnectedPlayers.Contains(PC))
		{
			ConnectedPlayers.AddUnique(PC);
			RestorePlayerFromGameInstance(PC);
		}
	}
}

void ABFGameMode::RestorePlayerFromGameInstance(APlayerController* PC)
{
	if (!PC || !PC->PlayerState || !BFGameState) return;

	int32 PlayerId = PC->PlayerState->GetPlayerId();

	if (bIsLobby)
	{
		if (!BFGameState->HasPlayerInfo(PlayerId))
		{
			BFGameState->SetPlayerTeam(PlayerId, 255);
			BFGameState->SetPlayerUniqueNetId(PlayerId, PC->PlayerState->GetUniqueId().ToString());
		}
	}
	else
	{
		UBFGameInstance* GI = GetGameInstance<UBFGameInstance>();
		if (!GI) return;

		FString UniqueNetId = PC->PlayerState->GetUniqueId().ToString();
		for (const FBFPlayerTeamInfo& Info : GI->GetSavedPlayerInfos())
		{
			if (Info.UniqueNetId == UniqueNetId)
			{
				BFGameState->SetPlayerTeam(PlayerId, Info.TeamId);
				BFGameState->SetPlayerRole(PlayerId, Info.Role);
				BFGameState->SetPlayerName(PlayerId, Info.PlayerName);
				BFGameState->SetPlayerUniqueNetId(PlayerId, UniqueNetId);
				UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Restored player %d: Team=%d, Role=%d, Name=%s"),
					PlayerId, Info.TeamId, (uint8)Info.Role, *Info.PlayerName);
				break;
			}
		}
	}
}

void ABFGameMode::InitGameState()
{
	Super::InitGameState();

	BFGameState = GetGameState<ABFGameState>();

	if (BFGameState)
	{
		// 카운트다운 완료 델리게이트 바인딩
		BFGameState->OnCountdownFinished.AddDynamic(this, &ABFGameMode::HandleCountdownFinished);

		// GameInstance에서 팀 개수 + 라운드 번호 불러와서 적용
		if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
		{
			int32 PendingTeamCount = GI->GetPendingTeamCount();
			BFGameState->SetTeamCount(PendingTeamCount);
			UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Team count set to %d from GameInstance"), PendingTeamCount);

			// 레벨 재시작 시 이전 라운드 번호 복원
			int32 SavedRound = GI->GetNextRound();
			if (SavedRound > 1)
			{
				BFGameState->SetCurrentRound(SavedRound);
				UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Round restored to %d from GameInstance"), SavedRound);
			}
		}
	}
}

void ABFGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer) return;

	ConnectedPlayers.AddUnique(NewPlayer);

	if (bIsLobby)
		ReadyPlayers.Add(NewPlayer);

	RestorePlayerFromGameInstance(NewPlayer);

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Player logged in. Total: %d (bIsLobby=%s)"),
		ConnectedPlayers.Num(), bIsLobby ? TEXT("true") : TEXT("false"));

	if (bIsLobby)
	{
		CheckAndStartGame();
	}
	else
	{
		// 마트: 로비에서 접속했던 인원이 모두 들어오면 카운트다운 시작
		if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
		{
			int32 ExpectedPlayers = GI->GetSavedPlayerInfos().Num();
			UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Mart - Connected: %d / Expected: %d"),
				ConnectedPlayers.Num(), ExpectedPlayers);

			if (ConnectedPlayers.Num() >= ExpectedPlayers && ExpectedPlayers > 0)
			{
				UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Mart - All players connected! Starting countdown."));
				StartGameCountdown();
			}
		}
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
