// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BFGameState.h"
#include "BFPlayerController.generated.h"

UCLASS()
class BLACKFRIDAY_API ABFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABFPlayerController();

	virtual void BeginPlay() override;

	// ===== 클라이언트 → 서버 RPC =====

	// 이름 설정 요청 (로비 입장 시 호출)
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "BF|Player")
	void ServerSetPlayerName(const FString& NewName);

	// 팀 변경 요청
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "BF|Player")
	void ServerChangeTeam(uint8 NewTeamId);

	// 역할 변경 요청
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "BF|Player")
	void ServerChangeRole(EBFPlayerRole NewRole);

	// 준비 완료 알림
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "BF|Player")
	void ServerNotifyReady();

	// 준비 취소
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "BF|Player")
	void ServerCancelReady();

	// 호스트 전용: 게임 시작 요청
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "BF|Player")
	void ServerHostStartGame();

	// ===== 클라이언트에서 GameInstance 이름 자동 전송 =====

	// 로비 입장 시 GameInstance에 저장된 이름을 서버로 전송
	UFUNCTION(BlueprintCallable, Category = "BF|Player")
	void SendLocalPlayerNameToServer();
};
