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

	// ===== 서버 → 클라이언트 RPC =====

	// 로딩 화면 표시 (서버에서 레벨 이동 직전 호출)
	UFUNCTION(Client, Reliable)
	void ClientShowLoadingScreen();

	// BP에서 구현: 실제 로딩 화면 표시 로직
	UFUNCTION(BlueprintImplementableEvent, Category = "BF|Player")
	void OnShowLoadingScreen();

	// 로딩 화면 숨기기 (서버에서 모든 플레이어 로딩 완료 시 호출)
	UFUNCTION(Client, Reliable)
	void ClientHideLoadingScreen();

	// BP에서 구현: 실제 로딩 화면 제거 로직
	UFUNCTION(BlueprintImplementableEvent, Category = "BF|Player")
	void OnHideLoadingScreen();

	// ===== 클라이언트에서 GameInstance 이름 자동 전송 =====

	// 로비 입장 시 GameInstance에 저장된 이름을 서버로 전송
	UFUNCTION(BlueprintCallable, Category = "BF|Player")
	void SendLocalPlayerNameToServer();

	// ===== 결과 데이터 (ServerTravel 후에도 PC에 유지) =====

	UFUNCTION(Client, Reliable)
	void ClientReceiveResultData(int32 WinnerTeamId, const TArray<float>& InTeamPayments, const TArray<int32>& InRoundWinners);

	UPROPERTY(BlueprintReadOnly, Category = "BF|Result")
	int32 ResultWinnerTeamId = -1;

	UPROPERTY(BlueprintReadOnly, Category = "BF|Result")
	TArray<float> ResultTeamPayments;

	UPROPERTY(BlueprintReadOnly, Category = "BF|Result")
	TArray<int32> ResultRoundWinners;
};
