// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BFCheckoutManager.generated.h"

class ABFCheckoutCounter;
class ABFGameState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllCheckoutsProcessed, int32, RoundNumber);

/**
 * 마트 카운터 매니저
 *
 * - 레벨에 1개 배치, 에디터에서 Counters 배열에 카운터 4개 레퍼런스 연결
 * - BeginPlay 시 BFGameMode에 자신을 등록
 * - 라운드 종료 시 BFGameMode::EndRound() → ProcessCheckoutsAndDeactivate() 호출
 *   → 전체 활성 카운터 결제 처리 → BFGameState에 팀별 금액 누적
 *   → 라운드당 카운터 1개 비활성화 (1라운드: 4개→3개, 2라운드: 3개→2개)
 */
UCLASS()
class BLACKFRIDAY_API ABFCheckoutManager : public AActor
{
	GENERATED_BODY()

public:
	ABFCheckoutManager();

	virtual void BeginPlay() override;

	// 라운드 종료 시 결제 처리 + 카운터 1개 비활성화
	UFUNCTION(BlueprintCallable, Category = "BF|CheckoutManager")
	void ProcessCheckoutsAndDeactivate(int32 CurrentRound);

	// 현재 활성 카운터 수
	UFUNCTION(BlueprintPure, Category = "BF|CheckoutManager")
	int32 GetActiveCounterCount() const;

	// 라운드 시작 시 활성 카운터 목록 로그 출력
	void LogActiveCounters(int32 RoundNumber) const;

	UPROPERTY(BlueprintAssignable, Category = "BF|CheckoutManager")
	FOnAllCheckoutsProcessed OnAllCheckoutsProcessed;

protected:
	// 에디터에서 레벨에 배치된 카운터 레퍼런스 설정 (기본 4개)
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "BF|CheckoutManager")
	TArray<TObjectPtr<ABFCheckoutCounter>> Counters;
};
