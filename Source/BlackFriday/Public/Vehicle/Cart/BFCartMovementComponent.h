#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "BFCartMovementComponent.generated.h"

UENUM(BlueprintType)
enum class ECartDriftState : uint8
{
    None,
    Drifting
};

UENUM(BlueprintType)
enum class ECartBoostTier : uint8
{
    None,
    Mini,
    Super,
    Ultra
};

UCLASS(ClassGroup=(Movement), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFCartMovementComponent : public UPawnMovementComponent
{
    GENERATED_BODY()

public:
    UBFCartMovementComponent();

    // Input setters (Pawn/Controller에서 호출)
    void SetThrottle(float Value);   // -1..+1
    void SetSteer(float Value);      // -1..+1
    void SetDriftHeld(bool bHeld);

    // Debug/조회
    UFUNCTION(BlueprintCallable, Category="Cart|State")
    bool IsDrifting() const { return DriftState == ECartDriftState::Drifting; }

    UFUNCTION(BlueprintCallable, Category="Cart|State")
    float GetSpeed() const { return Velocity.Size(); }

    UFUNCTION(BlueprintCallable, Category="Cart|State")
    float GetDriftCharge() const { return DriftCharge; }

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    // ===== Tunables (마리오카트 느낌용) =====
    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Gravity")
    float GravityZ = -980.f; // cm/s^2 (UE 단위)

    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Gravity")
    float MaxFallSpeed = 4000.f;
    
    bool bIsGrounded = false;
    
    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Speed")
    float MaxSpeed = 2400.f; // cm/s (24 m/s)

    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Speed")
    float Accel = 8000.f;

    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Speed")
    float BrakeDecel = 11000.f;
    
    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Speed")
    float CoastDecel = 4500.f; // 입력 없을 때 초당 감속량 (cm/s^2 느낌으로 사용)

    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Speed")
    float StopSpeedThreshold = 10.f; // 이하면 0으로 스냅

    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Handling")
    float BaseTurnRateDegPerSec = 140.f;

    // 속도가 빠를수록 조향이 둔해지는 정도(0~1 권장)
    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Handling")
    float HighSpeedSteerDamping = 0.55f;
    
    // // 드리프트 시작 순간의 조향 방향 고정 (마리오카트 느낌)
    // UPROPERTY(EditAnywhere, Category="Cart|Tuning|Drift")
    // bool bLockDriftDirection = true;
    //
    // float LockedDriftSign = 0.f; // -1 or +1

    // 드리프트 중 조향 증폭
    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Drift")
    float DriftTurnMultiplier = 1.65f;

    // 드리프트 중 속도 손실 (초당)
    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Drift")
    float DriftSpeedLossPerSec = 120.f;

    // 드리프트 시작 최소 속도
    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Drift")
    float MinSpeedToDrift = 900.f;

    // 드리프트 시작 최소 조향 입력(절대값)
    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Drift")
    float MinSteerToStartDrift = 0.25f;

    // 차지 증가율(초당) – 조향 강도에 비례하여 가중
    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Drift")
    float DriftChargeRate = 1.0f;

    // 티어 임계값(차지 단위)
    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Boost")
    float MiniThreshold = 1.2f;

    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Boost")
    float SuperThreshold = 2.6f;

    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Boost")
    float UltraThreshold = 4.2f;

    // 부스트 효과
    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Boost")
    float BoostMaxSpeedBonus = 900.f;

    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Boost")
    float BoostAccelBonus = 7000.f;

    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Boost")
    float MiniBoostDuration = 0.55f;

    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Boost")
    float SuperBoostDuration = 0.9f;

    UPROPERTY(EditAnywhere, Category="Cart|Tuning|Boost")
    float UltraBoostDuration = 1.25f;

private:
    // ===== State =====
    float ThrottleInput = 0.f;
    float SteerInput = 0.f;
    bool  bDriftHeld = false;
    bool  bWasDriftHeld = false;

    ECartDriftState DriftState = ECartDriftState::None;
    float DriftCharge = 0.f;

    float BoostTimeRemaining = 0.f;
    ECartBoostTier ActiveBoostTier = ECartBoostTier::None;

private:
    void UpdateArcadeMovement(float DeltaTime);
    void TryStartDrift();
    void UpdateDrift(float DeltaTime);
    void EndDriftAndBoost();

    ECartBoostTier EvaluateBoostTier() const;
    float GetBoostDuration(ECartBoostTier Tier) const;

    float ComputeTurnRateDegPerSec() const;
};
