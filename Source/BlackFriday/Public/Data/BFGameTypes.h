// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BFGameTypes.generated.h"

// 게임 페이즈
UENUM(BlueprintType)
enum class EBFGamePhase : uint8
{
	Waiting      UMETA(DisplayName = "Waiting"),
	Countdown    UMETA(DisplayName = "Countdown"),
	Playing      UMETA(DisplayName = "Playing"),
	RoundEnd     UMETA(DisplayName = "RoundEnd"),
	GameEnd      UMETA(DisplayName = "GameEnd")
};

// 플레이어 역할
UENUM(BlueprintType)
enum class EBFPlayerRole : uint8
{
	None         UMETA(DisplayName = "None"),
	Pusher       UMETA(DisplayName = "Pusher"),
	Gunner       UMETA(DisplayName = "Gunner")
};

// 팀 스폰 위치
UENUM(BlueprintType)
enum class EBFSpawnLocation : uint8
{
	North        UMETA(DisplayName = "North"),
	South        UMETA(DisplayName = "South"),
	East         UMETA(DisplayName = "East"),
	West         UMETA(DisplayName = "West")
};

// 팀 설정 (팀 ID + 스폰 위치)
USTRUCT(BlueprintType)
struct FBFTeamSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	uint8 TeamId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	EBFSpawnLocation SpawnLocation = EBFSpawnLocation::North;

	FBFTeamSettings() {}
	FBFTeamSettings(uint8 InTeamId, EBFSpawnLocation InSpawnLocation)
		: TeamId(InTeamId), SpawnLocation(InSpawnLocation) {}
};

// 플레이어 팀 정보 (GameState 복제용)
USTRUCT(BlueprintType)
struct FBFPlayerTeamInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	int32 PlayerId = -1;

	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	uint8 TeamId = 255;  // 255 = 미선택

	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	EBFPlayerRole Role = EBFPlayerRole::None;

	// 레벨 이동 후에도 플레이어 식별용 (PlayerId는 레벨마다 바뀜)
	UPROPERTY(BlueprintReadOnly, Category = "BF|Team")
	FString UniqueNetId;

	FBFPlayerTeamInfo() {}
	FBFPlayerTeamInfo(int32 InPlayerId, const FString& InPlayerName = TEXT(""), uint8 InTeamId = 255,
		EBFPlayerRole InRole = EBFPlayerRole::None, const FString& InUniqueNetId = TEXT(""))
		: PlayerId(InPlayerId), PlayerName(InPlayerName), TeamId(InTeamId), Role(InRole), UniqueNetId(InUniqueNetId) {}
};

// 라운드 테마 (라운드별 아이템 카테고리)
UENUM(BlueprintType)
enum class EBFRoundTheme : uint8
{
	Food         UMETA(DisplayName = "식품"),
	Electronics  UMETA(DisplayName = "가전"),
	Fashion      UMETA(DisplayName = "패션")
};

// 팀별 결제 기록
USTRUCT(BlueprintType)
struct FBFTeamPaymentRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BF|Payment")
	uint8 TeamId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BF|Payment")
	float RoundPayment = 0.0f;   // 이번 라운드 결제금액

	UPROPERTY(BlueprintReadOnly, Category = "BF|Payment")
	float TotalPayment = 0.0f;   // 누적 결제금액
};
