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

	if (IsLocalController())
	{
		// 마트 레벨 로드 완료 후 로딩화면 재표시 (ServerTravel로 위젯이 날아가므로)
		// bShouldShowLoadingScreen: ClientShowLoadingScreen RPC에서 설정됨 (서버/클라 모두 동작)
		if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
		{
			if (GI->GetShouldShowLoadingScreen())
			{
				OnShowLoadingScreen();
			}
		}

		// 약간의 딜레이 후 이름 전송 (네트워크 준비 대기)
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			SendLocalPlayerNameToServer();
		}, 0.5f, false);

		UE_LOG(LogTemp, Warning, TEXT("[BFPlayerController] BeginPlay - Local controller, will send name in 0.5s"));
	}
}

void ABFPlayerController::ServerSetPlayerName_Implementation(const FString& NewName)
{
	UE_LOG(LogTemp, Warning, TEXT("[BFPlayerController] ServerSetPlayerName called! Name: %s"), *NewName);

	if (ABFGameMode* GM = GetWorld()->GetAuthGameMode<ABFGameMode>())
	{
		GM->RequestSetPlayerName(this, NewName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[BFPlayerController] GameMode is NULL!"));
	}
}

void ABFPlayerController::ServerChangeTeam_Implementation(uint8 NewTeamId)
{
	UE_LOG(LogTemp, Warning, TEXT("[BFPlayerController] ServerChangeTeam called! TeamId: %d"), NewTeamId);

	if (ABFGameMode* GM = GetWorld()->GetAuthGameMode<ABFGameMode>())
	{
		GM->RequestChangeTeam(this, NewTeamId);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[BFPlayerController] GameMode is NULL!"));
	}
}

void ABFPlayerController::ServerChangeRole_Implementation(EBFPlayerRole NewRole)
{
	UE_LOG(LogTemp, Warning, TEXT("[BFPlayerController] ServerChangeRole called! Role: %d"), (uint8)NewRole);

	if (ABFGameMode* GM = GetWorld()->GetAuthGameMode<ABFGameMode>())
	{
		GM->RequestChangeRole(this, NewRole);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[BFPlayerController] GameMode is NULL!"));
	}
}

void ABFPlayerController::ServerNotifyReady_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[BFPlayerController] ServerNotifyReady called!"));

	if (ABFGameMode* GM = GetWorld()->GetAuthGameMode<ABFGameMode>())
	{
		GM->NotifyPlayerReady(this);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[BFPlayerController] GameMode is NULL!"));
	}
}

void ABFPlayerController::ServerCancelReady_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[BFPlayerController] ServerCancelReady called!"));

	if (ABFGameMode* GM = GetWorld()->GetAuthGameMode<ABFGameMode>())
	{
		GM->CancelPlayerReady(this);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[BFPlayerController] GameMode is NULL!"));
	}
}

void ABFPlayerController::ClientShowLoadingScreen_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[BFPlayerController] ClientShowLoadingScreen_Implementation - HasAuthority=%s, IsLocal=%s, Class=%s"),
		HasAuthority() ? TEXT("true") : TEXT("false"),
		IsLocalController() ? TEXT("true") : TEXT("false"),
		*GetClass()->GetName());

	if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
	{
		GI->SetShouldShowLoadingScreen(true);
	}
	OnShowLoadingScreen();
}

void ABFPlayerController::ClientHideLoadingScreen_Implementation()
{
	if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
	{
		GI->SetShouldShowLoadingScreen(false);
	}
	OnHideLoadingScreen();
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
	UE_LOG(LogTemp, Warning, TEXT("[BFPlayerController] SendLocalPlayerNameToServer called!"));

	if (!GetWorld() || !GetGameInstance())  return;
	
	if (UBFGameInstance* GI = GetGameInstance<UBFGameInstance>())
	{
		FString LocalName = GI->GetLocalPlayerName();
		UE_LOG(LogTemp, Warning, TEXT("[BFPlayerController] LocalPlayerName from GameInstance: %s"), *LocalName);

		if (!LocalName.IsEmpty())
		{
			ServerSetPlayerName(LocalName);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFPlayerController] LocalPlayerName is empty!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[BFPlayerController] GameInstance is NULL!"));
	}
}
