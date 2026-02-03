// Fill out your copyright notice in the Description page of Project Settings.

#include "System/BFPlayerController.h"
#include "System/BFGameMode.h"
#include "System/BFGameInstance.h"
#include "Kismet/GameplayStatics.h"

ABFPlayerController::ABFPlayerController()
{
}

void ABFPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ABFPlayerController::ServerSetPlayerName_Implementation(const FString& NewName)
{
	if (ABFGameMode* GM = GetWorld()->GetAuthGameMode<ABFGameMode>())
	{
		GM->RequestSetPlayerName(this, NewName);
	}
}

void ABFPlayerController::ServerChangeTeam_Implementation(uint8 NewTeamId)
{
	if (ABFGameMode* GM = GetWorld()->GetAuthGameMode<ABFGameMode>())
	{
		GM->RequestChangeTeam(this, NewTeamId);
	}
}

void ABFPlayerController::ServerChangeRole_Implementation(EBFPlayerRole NewRole)
{
	if (ABFGameMode* GM = GetWorld()->GetAuthGameMode<ABFGameMode>())
	{
		GM->RequestChangeRole(this, NewRole);
	}
}

void ABFPlayerController::ServerNotifyReady_Implementation()
{
	if (ABFGameMode* GM = GetWorld()->GetAuthGameMode<ABFGameMode>())
	{
		GM->NotifyPlayerReady(this);
	}
}

void ABFPlayerController::ServerHostStartGame_Implementation()
{
	// 호스트(서버)인지 확인
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFPlayerController] Only host can start the game"));
		return;
	}

	if (ABFGameMode* GM = GetWorld()->GetAuthGameMode<ABFGameMode>())
	{
		GM->HostStartGame();
	}
}

void ABFPlayerController::SendLocalPlayerNameToServer()
{
	if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
	{
		FString LocalName = GI->GetLocalPlayerName();
		if (!LocalName.IsEmpty())
		{
			ServerSetPlayerName(LocalName);
		}
	}
}
