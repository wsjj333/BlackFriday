// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BFPushComponent.generated.h"

/**
 * UBFPushComponent
 *
 * 플레이어가 전방에 있는 물리 오브젝트를 "밀어내는" 기능을 담당하는 컴포넌트.
 * - 로컬 클라이언트에서만 판정 및 입력 처리
 * - 실제 물리 Impulse 적용은 서버 권한에서 수행
 * - 연속 푸시 방지를 위한 쿨타임(PushInterval) 관리
 */
UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFPushComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	/** 기본 생성자 */
	UBFPushComponent();

protected:
	/** 게임 시작 시 호출 */
	virtual void BeginPlay() override;

public:
	/**
	 * 매 프레임 호출
	 * - 전방 Sweep으로 물리 오브젝트 탐색
	 * - 푸시 조건 충족 시 서버 RPC 호출 또는 즉시 적용
	 */
	virtual void TickComponent(
		float DeltaTime,
		enum ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;
	
	/** 밀어내기 힘의 기본 세기 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Physics")
	float PushStrength;
	
	/** 전방 탐색 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Physics")
	float PushRange;
	
	/** 연속 푸시 방지를 위한 최소 간격 (초) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Physics")
	float PushInterval;
	
	/** Z축 성분 제거 여부 (수평 방향으로만 밀기 위함) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Physics")
	bool bFlattenZ;

private:
	/** 로컬 클라이언트 기준 마지막 푸시 시간 */
	float LastPushTime;

	/** 서버 기준 마지막 푸시 시간 (RPC 스팸 방지) */
	float ServerLastPushTime;
	
	/**
	 * 현재 밀고 있는 액터
	 * - 이동 중 충돌로 밀려나지 않도록 IgnoreActorWhenMoving 처리에 사용
	 */
	UPROPERTY()
	AActor* CurrentIgnoredActor;
	
	/**
	 * 서버에서 물리 Impulse를 적용하기 위한 RPC
	 * @param HitComp  푸시 대상 PrimitiveComponent
	 * @param PushForce 적용할 Impulse 벡터
	 * @param Location Impulse 적용 위치
	 */
	UFUNCTION(Server, Reliable)
	void Server_ApplyPush(UPrimitiveComponent* HitComp, FVector PushForce, FVector Location);
};
