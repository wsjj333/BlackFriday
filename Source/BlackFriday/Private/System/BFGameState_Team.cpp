// Fill out your copyright notice in the Description page of Project Settings.
// 팀 / 플레이어 / 역할 관리

#include "System/BFGameState.h"
#include "Net/UnrealNetwork.h"

void ABFGameState::SetPlayerTeam(int32 PlayerId, uint8 TeamId)
{
	if (!HasAuthority()) return;

	if (TeamId != 255)
	{
		// 자기 자신 제외하고 팀 인원 체크
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

	// 자동 역할 부여 (Gunner 우선)
	EBFPlayerRole AutoRole = EBFPlayerRole::None;
	if (TeamId != 255)
	{
		if (IsRoleAvailableInTeam(TeamId, EBFPlayerRole::Gunner))
			AutoRole = EBFPlayerRole::Gunner;
		else if (IsRoleAvailableInTeam(TeamId, EBFPlayerRole::Pusher))
			AutoRole = EBFPlayerRole::Pusher;
	}

	for (FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId)
		{
			Info.TeamId = TeamId;
			Info.Role = AutoRole;
			OnRep_PlayerTeamInfos();
			OnPlayerTeamChanged.Broadcast(PlayerId, TeamId);
			OnPlayerRoleChanged.Broadcast(PlayerId, AutoRole);
			UE_LOG(LogTemp, Warning, TEXT("[BFGameState] Player %d → Team %d, Role %d"), PlayerId, TeamId, (uint8)AutoRole);
			return;
		}
	}

	PlayerTeamInfos.Add(FBFPlayerTeamInfo(PlayerId, TEXT(""), TeamId, AutoRole));
	OnRep_PlayerTeamInfos();
	OnPlayerTeamChanged.Broadcast(PlayerId, TeamId);
	OnPlayerRoleChanged.Broadcast(PlayerId, AutoRole);
	UE_LOG(LogTemp, Warning, TEXT("[BFGameState] Player %d added → Team %d, Role %d"), PlayerId, TeamId, (uint8)AutoRole);
}

void ABFGameState::RemovePlayerTeamInfo(int32 PlayerId)
{
	if (!HasAuthority()) return;

	PlayerTeamInfos.RemoveAll([PlayerId](const FBFPlayerTeamInfo& Info)
	{
		return Info.PlayerId == PlayerId;
	});
	OnRep_PlayerTeamInfos();
}

void ABFGameState::SetTeamCount(int32 NewTeamCount)
{
	if (!HasAuthority()) return;

	if (GamePhase != EBFGamePhase::Waiting)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameState] Cannot change team count after game started"));
		return;
	}

	TeamCount = FMath::Clamp(NewTeamCount, 1, 8);

	for (FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.TeamId >= TeamCount)
			Info.TeamId = 255;
	}

	OnRep_TeamCount();
	OnRep_PlayerTeamInfos();
	UE_LOG(LogTemp, Log, TEXT("[BFGameState] Team count → %d"), TeamCount);
}

void ABFGameState::SetPlayerName(int32 PlayerId, const FString& NewName)
{
	if (!HasAuthority()) return;

	for (FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId)
		{
			Info.PlayerName = NewName;
			OnRep_PlayerTeamInfos();
			return;
		}
	}

	PlayerTeamInfos.Add(FBFPlayerTeamInfo(PlayerId, NewName));
	OnRep_PlayerTeamInfos();
}

void ABFGameState::SetPlayerUniqueNetId(int32 PlayerId, const FString& UniqueNetId)
{
	if (!HasAuthority()) return;

	for (FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId)
		{
			Info.UniqueNetId = UniqueNetId;
			OnRep_PlayerTeamInfos();
			UE_LOG(LogTemp, Log, TEXT("[BFGameState] Player %d UniqueNetId: %s"), PlayerId, *UniqueNetId);
			return;
		}
	}

	FBFPlayerTeamInfo NewInfo;
	NewInfo.PlayerId = PlayerId;
	NewInfo.UniqueNetId = UniqueNetId;
	PlayerTeamInfos.Add(NewInfo);
	OnRep_PlayerTeamInfos();
	UE_LOG(LogTemp, Log, TEXT("[BFGameState] Player %d added with UniqueNetId: %s"), PlayerId, *UniqueNetId);
}

void ABFGameState::OnRep_PlayerTeamInfos()
{
	// (-1, 255) = 전체 갱신 신호
	OnPlayerTeamChanged.Broadcast(-1, 255);
}

void ABFGameState::OnRep_TeamCount()
{
	OnTeamCountChanged.Broadcast(TeamCount);
}

uint8 ABFGameState::GetPlayerTeam(int32 PlayerId) const
{
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId) return Info.TeamId;
	}
	return 255;
}

TArray<int32> ABFGameState::GetPlayersInTeam(uint8 TeamId) const
{
	TArray<int32> Result;
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.TeamId == TeamId) Result.Add(Info.PlayerId);
	}
	return Result;
}

int32 ABFGameState::GetTeamPlayerCount(uint8 TeamId) const
{
	int32 Count = 0;
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.TeamId == TeamId) Count++;
	}
	return Count;
}

bool ABFGameState::HasPlayerSelectedTeam(int32 PlayerId) const
{
	return GetPlayerTeam(PlayerId) != 255;
}

bool ABFGameState::IsTeamFull(uint8 TeamId) const
{
	return GetTeamPlayerCount(TeamId) >= MaxPlayersPerTeam;
}

EBFSpawnLocation ABFGameState::GetTeamSpawnLocation(uint8 TeamId) const
{
	for (const FBFTeamSettings& Settings : TeamSettings)
	{
		if (Settings.TeamId == TeamId) return Settings.SpawnLocation;
	}
	return EBFSpawnLocation::North;
}

FString ABFGameState::GetPlayerName(int32 PlayerId) const
{
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId) return Info.PlayerName;
	}
	return TEXT("");
}

bool ABFGameState::HasPlayerInfo(int32 PlayerId) const
{
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId) return true;
	}
	return false;
}

bool ABFGameState::SetPlayerRole(int32 PlayerId, EBFPlayerRole NewRole)
{
	if (!HasAuthority()) return false;

	FBFPlayerTeamInfo* PlayerInfo = nullptr;
	for (FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId) { PlayerInfo = &Info; break; }
	}

	if (!PlayerInfo)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameState] Player %d not found"), PlayerId);
		return false;
	}

	if (PlayerInfo->TeamId == 255)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameState] Player %d must select team first"), PlayerId);
		return false;
	}

	if (NewRole != EBFPlayerRole::None && !IsRoleAvailableInTeam(PlayerInfo->TeamId, NewRole))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameState] Role %d already taken in Team %d"), (uint8)NewRole, PlayerInfo->TeamId);
		return false;
	}

	PlayerInfo->Role = NewRole;
	OnRep_PlayerTeamInfos();
	OnPlayerRoleChanged.Broadcast(PlayerId, NewRole);
	UE_LOG(LogTemp, Log, TEXT("[BFGameState] Player %d role → %d"), PlayerId, (uint8)NewRole);
	return true;
}

EBFPlayerRole ABFGameState::GetPlayerRole(int32 PlayerId) const
{
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.PlayerId == PlayerId) return Info.Role;
	}
	return EBFPlayerRole::None;
}

bool ABFGameState::HasPlayerSelectedRole(int32 PlayerId) const
{
	return GetPlayerRole(PlayerId) != EBFPlayerRole::None;
}

bool ABFGameState::IsRoleAvailableInTeam(uint8 TeamId, EBFPlayerRole InRole) const
{
	if (InRole == EBFPlayerRole::None) return true;

	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.TeamId == TeamId && Info.Role == InRole) return false;
	}
	return true;
}

int32 ABFGameState::GetPlayerWithRoleInTeam(uint8 TeamId, EBFPlayerRole InRole) const
{
	for (const FBFPlayerTeamInfo& Info : PlayerTeamInfos)
	{
		if (Info.TeamId == TeamId && Info.Role == InRole) return Info.PlayerId;
	}
	return -1;
}
