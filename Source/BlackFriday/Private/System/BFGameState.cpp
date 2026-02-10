// Fill out your copyright notice in the Description page of Project Settings.

#include "System/BFGameState.h"
#include "Net/UnrealNetwork.h"

ABFGameState::ABFGameState()
{
	// GameState는 기본적으로 bReplicates = true

	// 기본 팀 설정 초기화 (4팀: 북/남/동/서)
	TeamSettings.Add(FBFTeamSettings(0, EBFSpawnLocation::North));  // 팀 1 - 북쪽
	TeamSettings.Add(FBFTeamSettings(1, EBFSpawnLocation::South));  // 팀 2 - 남쪽
	TeamSettings.Add(FBFTeamSettings(2, EBFSpawnLocation::East));   // 팀 3 - 동쪽
	TeamSettings.Add(FBFTeamSettings(3, EBFSpawnLocation::West));   // 팀 4 - 서쪽
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
}

void ABFGameState::StartCountdown(int32 Seconds)
{
	if (!HasAuthority())
	{
		return;
	}

	CountdownTime = Seconds;
	SetGamePhase(EBFGamePhase::Countdown);

	// 1초마다 틱
	GetWorld()->GetTimerManager().SetTimer(
		CountdownTimerHandle,
		this,
		&ABFGameState::HandleCountdownTick,
		1.0f,
		true  // 반복
	);

	// 서버도 OnRep 호출 (서버는 자동으로 OnRep 안 불림)
	OnRep_CountdownTime();
}

void ABFGameState::StopCountdown()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
	CountdownTime = 0;
	OnRep_CountdownTime();
}

void ABFGameState::HandleCountdownTick()
{
	if (!HasAuthority())
	{
		return;
	}

	CountdownTime--;

	if (CountdownTime <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
		CountdownTime = 0;

		// 델리게이트 발동
		OnCountdownFinished.Broadcast();

		// 플레이 상태로 전환
		SetGamePhase(EBFGamePhase::Playing);
	}

	// 서버 로컬 이벤트 발동 (복제는 자동)
	OnRep_CountdownTime();
}

void ABFGameState::SetGamePhase(EBFGamePhase NewPhase)
{
	if (!HasAuthority())
	{
		return;
	}

	GamePhase = NewPhase;
	OnRep_GamePhase();
}

void ABFGameState::SetCurrentRound(int32 NewRound)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentRound = FMath::Clamp(NewRound, 1, MaxRounds);
	OnRep_CurrentRound();
}

void ABFGameState::OnRep_CountdownTime()
{
	OnCountdownChanged.Broadcast(CountdownTime);

	if (CountdownTime <= 0)
	{
		OnCountdownFinished.Broadcast();
	}
}

void ABFGameState::OnRep_GamePhase()
{
	OnGamePhaseChanged.Broadcast(GamePhase);
}

void ABFGameState::OnRep_CurrentRound()
{
	OnRoundChanged.Broadcast(CurrentRound);
}

void ABFGameState::SetPlayerTeam(int32 PlayerId, uint8 TeamId)
{
	if (!HasAuthority())
	{
		return;
	}

	// 255는 미선택이므로 인원 체크 안 함
	if (TeamId != 255)
	{
		// 해당 플레이어가 이미 이 팀에 있는지 확인 (자기 자신은 제외하고 카운트)
		int32 CurrentCount = 0;
		for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
		{
			if (Info.TeamId == TeamId && Info.PlayerId != PlayerId)
			{
				CurrentCount++;
			}
		}

		if (CurrentCount >= MaxPlayersPerTeam)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFGameState] Team %d is full (%d/%d)"), TeamId, CurrentCount, MaxPlayersPerTeam);
			return;
		}
	}

	// 자동 역할 부여 (Gunner 우선, 이미 있으면 Pusher)
	EBFPlayerRole AutoRole = EBFPlayerRole::None;
	if (TeamId != 255)
	{
		if (IsRoleAvailableInTeam(TeamId, EBFPlayerRole::Gunner))
		{
			AutoRole = EBFPlayerRole::Gunner;
		}
		else if (IsRoleAvailableInTeam(TeamId, EBFPlayerRole::Pusher))
		{
			AutoRole = EBFPlayerRole::Pusher;
		}
	}

	// 기존 정보 찾기
	for (FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId)
		{
			Info.TeamId = TeamId;
			Info.Role = AutoRole;
			OnRep_PlayerTeamInfos();
			OnPlayerTeamChanged.Broadcast(PlayerId, TeamId);
			OnPlayerRoleChanged.Broadcast(PlayerId, AutoRole);
			UE_LOG(LogTemp, Warning, TEXT("[BFGameState] SetPlayerTeam - Updated Player %d to Team %d, Role %d"), PlayerId, TeamId, (uint8)AutoRole);
			return;
		}
	}

	// 새로 추가
	PlayerTeamInfos.Add(FBFPlayerTeamInfo(PlayerId, TEXT(""), TeamId, AutoRole));
	OnRep_PlayerTeamInfos();
	OnPlayerTeamChanged.Broadcast(PlayerId, TeamId);
	OnPlayerRoleChanged.Broadcast(PlayerId, AutoRole);
	UE_LOG(LogTemp, Warning, TEXT("[BFGameState] SetPlayerTeam - Added Player %d to Team %d, Role %d"), PlayerId, TeamId, (uint8)AutoRole);
}

void ABFGameState::RemovePlayerTeamInfo(int32 PlayerId)
{
	if (!HasAuthority())
	{
		return;
	}

	PlayerTeamInfos.RemoveAll([PlayerId](const FBFPlayerTeamInfo& Info)
	{
		return Info.PlayerId == PlayerId;
	});
	OnRep_PlayerTeamInfos();
}

uint8 ABFGameState::GetPlayerTeam(int32 PlayerId) const
{
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId)
		{
			return Info.TeamId;
		}
	}
	return 255;  // 미선택
}

TArray<int32> ABFGameState::GetPlayersInTeam(uint8 TeamId) const
{
	TArray<int32> Result;
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.TeamId == TeamId)
		{
			Result.Add(Info.PlayerId);
		}
	}
	return Result;
}

int32 ABFGameState::GetTeamPlayerCount(uint8 TeamId) const
{
	int32 Count = 0;
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.TeamId == TeamId)
		{
			Count++;
		}
	}
	return Count;
}

bool ABFGameState::HasPlayerSelectedTeam(int32 PlayerId) const
{
	return GetPlayerTeam(PlayerId) != 255;
}

void ABFGameState::SetPlayerName(int32 PlayerId, const FString& NewName)
{
	if (!HasAuthority())
	{
		return;
	}

	// 기존 정보 찾기
	for (FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId)
		{
			Info.PlayerName = NewName;
			OnRep_PlayerTeamInfos();
			return;
		}
	}

	// 없으면 새로 추가
	PlayerTeamInfos.Add(FBFPlayerTeamInfo(PlayerId, NewName));
	OnRep_PlayerTeamInfos();
}

FString ABFGameState::GetPlayerName(int32 PlayerId) const
{
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId)
		{
			return Info.PlayerName;
		}
	}
	return TEXT("");
}

void ABFGameState::SetPlayerUniqueNetId(int32 PlayerId, const FString& UniqueNetId)
{
	if (!HasAuthority())
	{
		return;
	}

	for (FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId)
		{
			Info.UniqueNetId = UniqueNetId;
			OnRep_PlayerTeamInfos();
			UE_LOG(LogTemp, Log, TEXT("[BFGameState] Player %d UniqueNetId set to: %s"), PlayerId, *UniqueNetId);
			return;
		}
	}

	// 없으면 새로 추가
	FBFPlayerTeamInfo NewInfo;
	NewInfo.PlayerId = PlayerId;
	NewInfo.UniqueNetId = UniqueNetId;
	PlayerTeamInfos.Add(NewInfo);
	OnRep_PlayerTeamInfos();
	UE_LOG(LogTemp, Log, TEXT("[BFGameState] Player %d added with UniqueNetId: %s"), PlayerId, *UniqueNetId);
}

void ABFGameState::OnRep_PlayerTeamInfos()
{
	// 클라이언트에서 팀 정보 복제 수신 시 UI 갱신을 위해 Broadcast
	// (-1, 255)는 "전체 갱신" 신호 - UI에서 FullRefresh 처리
	OnPlayerTeamChanged.Broadcast(-1, 255);
}

void ABFGameState::SetTeamCount(int32 NewTeamCount)
{
	if (!HasAuthority())
	{
		return;
	}

	// 게임 진행 중에는 변경 불가
	if (GamePhase != EBFGamePhase::Waiting)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameState] Cannot change team count after game started"));
		return;
	}

	// 유효 범위 체크 (1~8팀)
	TeamCount = FMath::Clamp(NewTeamCount, 1, 8);

	// 현재 선택된 팀이 새 팀 개수를 초과하면 초기화
	for (FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.TeamId >= TeamCount)
		{
			Info.TeamId = 255;  // 미선택으로 리셋
		}
	}

	OnRep_TeamCount();
	OnRep_PlayerTeamInfos();

	UE_LOG(LogTemp, Log, TEXT("[BFGameState] Team count changed to %d"), TeamCount);
}

void ABFGameState::OnRep_TeamCount()
{
	OnTeamCountChanged.Broadcast(TeamCount);
}

bool ABFGameState::SetPlayerRole(int32 PlayerId, EBFPlayerRole NewRole)
{
	if (!HasAuthority())
	{
		return false;
	}

	// 플레이어 정보 찾기
	FBFPlayerTeamInfo* PlayerInfo = nullptr;
	for (FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId)
		{
			PlayerInfo = &Info;
			break;
		}
	}

	if (!PlayerInfo)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameState] Player %d not found in team infos"), PlayerId);
		return false;
	}

	// 팀 선택 안 했으면 역할 선택 불가
	if (PlayerInfo->TeamId == 255)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameState] Player %d must select team first"), PlayerId);
		return false;
	}

	// 같은 팀에서 해당 역할이 이미 선택됐는지 체크
	if (NewRole != EBFPlayerRole::None && !IsRoleAvailableInTeam(PlayerInfo->TeamId, NewRole))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameState] Role %d already taken in Team %d"),
			(uint8)NewRole, PlayerInfo->TeamId);
		return false;
	}

	PlayerInfo->Role = NewRole;
	OnRep_PlayerTeamInfos();
	OnPlayerRoleChanged.Broadcast(PlayerId, NewRole);

	UE_LOG(LogTemp, Log, TEXT("[BFGameState] Player %d role changed to %d"), PlayerId, (uint8)NewRole);
	return true;
}

EBFPlayerRole ABFGameState::GetPlayerRole(int32 PlayerId) const
{
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId)
		{
			return Info.Role;
		}
	}
	return EBFPlayerRole::None;
}

bool ABFGameState::HasPlayerSelectedRole(int32 PlayerId) const
{
	return GetPlayerRole(PlayerId) != EBFPlayerRole::None;
}

bool ABFGameState::IsRoleAvailableInTeam(uint8 TeamId, EBFPlayerRole InRole) const
{
	if (InRole == EBFPlayerRole::None)
	{
		return true;
	}

	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.TeamId == TeamId && Info.Role == InRole)
		{
			return false;  // 이미 누군가 선택함
		}
	}
	return true;
}

int32 ABFGameState::GetPlayerWithRoleInTeam(uint8 TeamId, EBFPlayerRole InRole) const
{
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.TeamId == TeamId && Info.Role == InRole)
		{
			return Info.PlayerId;
		}
	}
	return -1;  // 없음
}

bool ABFGameState::IsTeamFull(uint8 TeamId) const
{
	return GetTeamPlayerCount(TeamId) >= MaxPlayersPerTeam;
}

EBFSpawnLocation ABFGameState::GetTeamSpawnLocation(uint8 TeamId) const
{
	for (const FBFTeamSettings& Settings : TeamSettings)
	{
		if (Settings.TeamId == TeamId)
		{
			return Settings.SpawnLocation;
		}
	}
	return EBFSpawnLocation::North;  // 기본값
}

bool ABFGameState::HasPlayerInfo(int32 PlayerId) const
{
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId)
		{
			return true;
		}
	}
	return false;
}

//TODO : 500 줄 넘는거 기능별로 나눠서 정리 ㄱㄱ