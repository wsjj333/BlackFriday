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

void ABFGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] ========== BeginPlay =========="));

	// ServerTravel 후 서버 플레이어(호스트)는 PostLogin이 호출되지 않음
	// 이미 존재하는 플레이어를 ConnectedPlayers와 GameState에 등록
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && !ConnectedPlayers.Contains(PC))
		{
			ConnectedPlayers.AddUnique(PC);

			if (PC->PlayerState && BFGameState)
			{
				int32 PlayerId = PC->PlayerState->GetPlayerId();

				// 이미 등록된 플레이어인지 확인 (GameInstance에서 로비 데이터 복원)
				if (!BFGameState->HasPlayerInfo(PlayerId))
				{
					BFGameState->SetPlayerTeam(PlayerId, 255);

					FString UniqueNetId = PC->PlayerState->GetUniqueId().ToString();
					BFGameState->SetPlayerUniqueNetId(PlayerId, UniqueNetId);
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] BeginPlay - Registered existing player: %s"), *PC->GetName());
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

		// GameInstance에서 팀 개수 불러와서 적용 (세션 생성 시 설정한 값)
		if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
		{
			int32 PendingTeamCount = GI->GetPendingTeamCount();
			BFGameState->SetTeamCount(PendingTeamCount);
			UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Team count set to %d from GameInstance"), PendingTeamCount);
		}
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

				// UniqueNetId 저장 (레벨 이동 후에도 플레이어 식별 가능)
				FString UniqueNetId = NewPlayer->PlayerState->GetUniqueId().ToString();
				BFGameState->SetPlayerUniqueNetId(PlayerId, UniqueNetId);
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

void ABFGameMode::CancelPlayerReady(APlayerController* Player)
{
	if (!Player)
	{
		return;
	}

	ReadyPlayers.Remove(Player);

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Player cancelled ready. Ready: %d / Connected: %d"),
		ReadyPlayers.Num(), ConnectedPlayers.Num());
}

bool ABFGameMode::IsPlayerReady(APlayerController* Player) const
{
	return Player && ReadyPlayers.Contains(Player);
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
	// 시작 조건 체크
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

APlayerController* ABFGameMode::GetPlayerControllerByRoleInTeam(uint8 TeamId, EBFPlayerRole InRole)
{
	UBFGameInstance* GI = GetGameInstance<UBFGameInstance>();
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] GetPlayerControllerByRoleInTeam - GameInstance is NULL"));
		return nullptr;
	}

	// 저장된 로비 데이터에서 해당 팀/역할의 UniqueNetId 찾기
	FString TargetUniqueNetId;
	for (const FBFPlayerTeamInfo& Info : GI->GetSavedPlayerInfos())
	{
		if (Info.TeamId == TeamId && Info.Role == InRole)
		{
			TargetUniqueNetId = Info.UniqueNetId;
			break;
		}
	}

	if (TargetUniqueNetId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] GetPlayerControllerByRoleInTeam - No player found for Team %d, Role %d"), TeamId, (uint8)InRole);
		return nullptr;
	}

	// 현재 접속한 플레이어 중에서 UniqueNetId가 일치하는 PC 찾기
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->PlayerState)
		{
			FString CurrentUniqueNetId = PC->PlayerState->GetUniqueId().ToString();
			if (CurrentUniqueNetId == TargetUniqueNetId)
			{
				UE_LOG(LogTemp, Log, TEXT("[BFGameMode] GetPlayerControllerByRoleInTeam - Found PC %s for Team %d, Role %d"),
					*PC->GetName(), TeamId, (uint8)InRole);
				return PC;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] GetPlayerControllerByRoleInTeam - PC not found for UniqueNetId: %s"), *TargetUniqueNetId);
	return nullptr;
}

TArray<APlayerController*> ABFGameMode::GetAllPlayerControllersInTeam(uint8 TeamId)
{
	TArray<APlayerController*> Result;

	UBFGameInstance* GI = GetGameInstance<UBFGameInstance>();
	if (!GI)
	{
		return Result;
	}

	// 해당 팀의 모든 UniqueNetId 수집
	TArray<FString> TeamUniqueNetIds;
	for (const FBFPlayerTeamInfo& Info : GI->GetSavedPlayerInfos())
	{
		if (Info.TeamId == TeamId)
		{
			TeamUniqueNetIds.Add(Info.UniqueNetId);
		}
	}

	// 현재 접속한 플레이어 중에서 매칭
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->PlayerState)
		{
			FString CurrentUniqueNetId = PC->PlayerState->GetUniqueId().ToString();
			if (TeamUniqueNetIds.Contains(CurrentUniqueNetId))
			{
				Result.Add(PC);
			}
		}
	}

	return Result;
}

TArray<APlayerController*> ABFGameMode::GetAllConnectedPlayerControllers() const
{
	TArray<APlayerController*> Result;
	for (const TObjectPtr<APlayerController>& PC : ConnectedPlayers)
	{
		if (PC)
		{
			Result.Add(PC.Get());
		}
	}
	return Result;
}
