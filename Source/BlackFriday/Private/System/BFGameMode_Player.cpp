// Fill out your copyright notice in the Description page of Project Settings.

#include "System/BFGameMode.h"
#include "System/BFGameState.h"
#include "System/BFGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

void ABFGameMode::RequestChangeTeam(APlayerController* Player, uint8 NewTeamId)
{
	if (!Player || !Player->PlayerState || !BFGameState) return;

	int32 PlayerId = Player->PlayerState->GetPlayerId();

	if (NewTeamId >= BFGameState->GetTotalTeamCount())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFGameMode] Invalid team ID: %d"), NewTeamId);
		return;
	}

	BFGameState->SetPlayerTeam(PlayerId, NewTeamId);

	UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Player %d changed to Team %d"), PlayerId, NewTeamId);

	if (bAutoStartWhenReady && bWaitingForPlayers && AreAllPlayersReady())
	{
		StartGameCountdown();
	}
}

bool ABFGameMode::HaveAllPlayersSelectedTeam() const
{
	if (!BFGameState) return false;

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
	if (!BFGameState) return;

	BFGameState->SetTeamCount(NewTeamCount);
}

bool ABFGameMode::RequestChangeRole(APlayerController* Player, EBFPlayerRole NewRole)
{
	if (!Player || !Player->PlayerState || !BFGameState) return false;

	int32 PlayerId = Player->PlayerState->GetPlayerId();
	bool bSuccess = BFGameState->SetPlayerRole(PlayerId, NewRole);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("[BFGameMode] Player %d changed role to %d"), PlayerId, (uint8)NewRole);

		if (bAutoStartWhenReady && bWaitingForPlayers && AreAllPlayersReady())
		{
			StartGameCountdown();
		}
	}

	return bSuccess;
}

bool ABFGameMode::HaveAllPlayersSelectedRole() const
{
	if (!BFGameState) return false;

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
	if (!Player || !Player->PlayerState || !BFGameState) return;

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

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->PlayerState)
		{
			if (PC->PlayerState->GetUniqueId().ToString() == TargetUniqueNetId)
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
	if (!GI) return Result;

	TArray<FString> TeamUniqueNetIds;
	for (const FBFPlayerTeamInfo& Info : GI->GetSavedPlayerInfos())
	{
		if (Info.TeamId == TeamId)
		{
			TeamUniqueNetIds.Add(Info.UniqueNetId);
		}
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->PlayerState)
		{
			if (TeamUniqueNetIds.Contains(PC->PlayerState->GetUniqueId().ToString()))
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
