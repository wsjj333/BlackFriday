// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BFCheckoutCounter.generated.h"

class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCounterOccupied, ABFCheckoutCounter*, Counter, uint8, TeamId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCounterReleased, ABFCheckoutCounter*, Counter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCheckoutProcessed, uint8, TeamId, float, Amount);

UCLASS()
class BLACKFRIDAY_API ABFCheckoutCounter : public AActor
{
	GENERATED_BODY()

public:
	ABFCheckoutCounter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 결제 처리 - CheckoutManager가 라운드 종료 시 호출, 결제금액 반환
	UFUNCTION(BlueprintCallable, Category = "BF|Counter")
	float ProcessCheckout();

	// 카운터 비활성화 (라운드 종료 후 CheckoutManager가 1개씩 호출)
	UFUNCTION(BlueprintCallable, Category = "BF|Counter")
	void Deactivate();

	// 라운드 전환 시 점유 해제 (팀은 나가지 않아도 초기화)
	UFUNCTION(BlueprintCallable, Category = "BF|Counter")
	void ResetOccupation();

	UFUNCTION(BlueprintPure, Category = "BF|Counter")
	uint8 GetOccupyingTeamId() const { return OccupyingTeamId; }

	UFUNCTION(BlueprintPure, Category = "BF|Counter")
	bool IsCounterActive() const { return bIsActive; }
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BF|Counter")
	float GetTeamCartTotalPrice(uint8 TeamId);
	virtual float GetTeamCartTotalPrice_Implementation(uint8 TeamId);

	// 점유 팀 변경 시 (클라이언트 UI 갱신용)
	UPROPERTY(BlueprintAssignable, Category = "BF|Counter")
	FOnCounterOccupied OnCounterOccupied;

	// 점유 해제 시
	UPROPERTY(BlueprintAssignable, Category = "BF|Counter")
	FOnCounterReleased OnCounterReleased;

	// 결제 완료 시
	UPROPERTY(BlueprintAssignable, Category = "BF|Counter")
	FOnCheckoutProcessed OnCheckoutProcessed;

protected:
	virtual void BeginPlay() override;

	// 팀 진입 감지용 트리거 박스 (Overlap)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BF|Counter")
	TObjectPtr<UBoxComponent> TriggerZone;

	// 다른 팀 물리 차단용 박스 (Block) - 점유 시 활성화
	// BP에서 카운터 입구에 맞게 위치/크기 조절
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BF|Counter")
	TObjectPtr<UBoxComponent> EntryBarrier;

	// 현재 점유 팀 ID (255 = 빈 상태)
	UPROPERTY(ReplicatedUsing = OnRep_OccupyingTeamId, BlueprintReadOnly, Category = "BF|Counter")
	uint8 OccupyingTeamId = 255;

	// 카운터 활성화 여부
	UPROPERTY(ReplicatedUsing = OnRep_bIsActive, BlueprintReadOnly, Category = "BF|Counter")
	bool bIsActive = true;

	UFUNCTION()
	void OnRep_OccupyingTeamId();

	UFUNCTION()
	void OnRep_bIsActive();

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// TriggerZone 안에 있는 액터 목록 (서버 전용)
	UPROPERTY()
	TSet<TObjectPtr<AActor>> ActorsInZone;

private:
	// 점유 팀의 모든 액터가 Zone을 떠났는지 확인
	bool IsOccupyingTeamGone() const;
};
