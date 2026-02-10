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

	UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] ========== BeginPlay (bIsLobby=%s) =========="),
		bIsLobby ? TEXT("true") : TEXT("false"));

	// ServerTravel 후 서버 플레이어(호스트)는 PostLogin이 호출되지 않음
	// 이미 존재하는 플레이어를 ConnectedPlayers에 등록
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && !ConnectedPlayers.Contains(PC))
		{
			ConnectedPlayers.AddUnique(PC);

			if (PC->PlayerState && BFGameState)
			{
				int32 PlayerId = PC->PlayerState->GetPlayerId();

				if (bIsLobby)
				{
					// 로비: 팀 미선택 상태로 초기 등록
					if (!BFGameState->HasPlayerInfo(PlayerId))
					{
						BFGameState->SetPlayerTeam(PlayerId, 255);

						FString UniqueNetId = PC->PlayerState->GetUniqueId().ToString();
						BFGameState->SetPlayerUniqueNetId(PlayerId, UniqueNetId);
					}
				}
				else
				{
					// 마트(인게임): GameInstance에서 로비 데이터 복구
					if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
					{
						FString CurrentUniqueNetId = PC->PlayerState->GetUniqueId().ToString();

						for (const FBFPlayerTeamInfo& Info : GI->GetSavedPlayerInfos())
						{
							if (Info.UniqueNetId == CurrentUniqueNetId)
							{
								BFGameState->SetPlayerTeam(PlayerId, Info.TeamId);
								BFGameState->SetPlayerRole(PlayerId, Info.Role);
								BFGameState->SetPlayerName(PlayerId, Info.PlayerName);
								BFGameState->SetPlayerUniqueNetId(PlayerId, CurrentUniqueNetId);

								UE_LOG(LogTemp, Log, TEXT("[BFGameMode] BeginPlay(Mart) - Restored host player %d: Team=%d, Role=%d"),
									PlayerId, Info.TeamId, (uint8)Info.Role);
								break;
							}
						}
					}
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

		if (NewPlayer->PlayerState)
		{
			int32 PlayerId = NewPlayer->PlayerState->GetPlayerId();

			if (bIsLobby)
			{
				// 로비: Ready 등록 + UniqueNetId 저장
				ReadyPlayers.Add(NewPlayer);

				if (BFGameState)
				{
					FString UniqueNetId = NewPlayer->PlayerState->GetUniqueId().ToString();
					BFGameState->SetPlayerUniqueNetId(PlayerId, UniqueNetId);
				}
			}
			else
			{
				// 마트(인게임): GameInstance에서 로비 데이터 복구
				if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
				{
					FString CurrentUniqueNetId = NewPlayer->PlayerState->GetUniqueId().ToString();

					for (const FBFPlayerTeamInfo& Info : GI->GetSavedPlayerInfos())
					{
						if (Info.UniqueNetId == CurrentUniqueNetId)
						{
							if (BFGameState)
							{
								BFGameState->SetPlayerTeam(PlayerId, Info.TeamId);
								BFGameState->SetPlayerRole(PlayerId, Info.Role);
								BFGameState->SetPlayerName(PlayerId, Info.PlayerName);
								BFGameState->SetPlayerUniqueNetId(PlayerId, CurrentUniqueNetId);
							}

							UE_LOG(LogTemp, Log, TEXT("[BFGameMode] PostLogin(Mart) - Restored player %d: Team=%d, Role=%d, Name=%s"),
								PlayerId, Info.TeamId, (uint8)Info.Role, *Info.PlayerName);
							break;
						}
					}
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Player logged in. Total: %d (bIsLobby=%s)"),
			ConnectedPlayers.Num(), bIsLobby ? TEXT("true") : TEXT("false"));

		if (bIsLobby)
		{
			// 로비: 자동 시작 체크
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

    // 3. 팀 선택 체크 (★ 여기서 막힐 확률 99%)
    if (!HaveAllPlayersSelectedTeam())
    {
        UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] 시작 불가: 팀을 선택하지 않은 플레이어가 있음!"));
        
        // 범인 찾기
        if (BFGameState)
        {
            for (const auto& PC : ConnectedPlayers)
            {
                if (PC && PC->PlayerState)
                {
                    int32 PID = PC->PlayerState->GetPlayerId();
                    // 255 = 팀 미선택
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
        
        // 범인 찾기
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

    // 통과!
    UE_LOG(LogTemp, Log, TEXT("[BFGameMode] 모든 조건 만족! 게임 카운트다운 진입 가능."));
    return true;
}

void ABFGameMode::CheckAndStartGame()
{
	if (!bIsLobby)
	{
		return;
	}

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
	BFGameState->SetGamePhase(EBFGamePhase::Countdown);
	BFGameState->StartCountdown(StartCountdownSeconds);

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Game countdown started: %d seconds"), StartCountdownSeconds);
}

void ABFGameMode::HandleCountdownFinished()
{
	if (!BFGameState)
	{
		return;
	}

	EBFGamePhase CurrentPhase = BFGameState->GetGamePhase();

	if (CurrentPhase == EBFGamePhase::Countdown)
	{
		// 게임 시작 카운트다운(20초) 완료 → 라운드 시작
		UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Start countdown finished! Starting round."));
		OnGameStarted.Broadcast();
		StartRound();
	}
	else if (CurrentPhase == EBFGamePhase::Playing)
	{
		// 라운드 플레이 타임(30초/10분) 완료 → 라운드 종료
		UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Round time over!"));
		EndRound(-1); // -1 = 타임오버 (승자 없음)
	}
}

void ABFGameMode::StartRound()
{
	if (!BFGameState)
	{
		return;
	}

	// 라운드 플레이 타이머 시작
	BFGameState->SetGamePhase(EBFGamePhase::Playing);
	BFGameState->StartCountdown(RoundPlayTimeSeconds);

	OnRoundStarted.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Round %d started! Play time: %d seconds"),
		BFGameState->GetCurrentRound(), RoundPlayTimeSeconds);
}

void ABFGameMode::EndRound(int32 WinningTeam)
{
	if (!BFGameState) return;

	// 1. 페이즈 변경 및 결과 기록
	BFGameState->SetGamePhase(EBFGamePhase::RoundEnd);
	if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
	{
		GI->RecordRoundResult(BFGameState->GetCurrentRound(), WinningTeam);
	}

	// 2. 기존 타이머가 있다면 초기화 후 5초 뒤 다음 라운드 진행
	GetWorld()->GetTimerManager().SetTimer(
		RoundTransitionTimerHandle, 
		this, 
		&ABFGameMode::AdvanceToNextRound, 
		5.0f, 
		false
	);

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Round %d 끝! 5초 뒤 다음 단계 진행..."), BFGameState->GetCurrentRound());
}

void ABFGameMode::AdvanceToNextRound()
{
	if (!BFGameState) return;

	int32 NextRound = BFGameState->GetCurrentRound() + 1;

	// 마지막 라운드 체크
	if (NextRound > BFGameState->GetMaxRounds())
	{
		BFGameState->SetGamePhase(EBFGamePhase::GameEnd);
	}
	else
	{
		// 중요: 다음 라운드 카운트다운 시작 전 시간을 먼저 세팅해서 0초 노출 방지
		BFGameState->StopCountdown(); // 기존 타이머 중지
		BFGameState->SetCurrentRound(NextRound);
        
		// 페이즈 변경 및 카운트다운 시작
		BFGameState->SetGamePhase(EBFGamePhase::Countdown);
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


//TODO : 500 줄 넘는거 기능별로 나눠서 정리 ㄱㄱ