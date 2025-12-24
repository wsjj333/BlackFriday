#include "Vehicle/Cart/BFCartMovementComponent.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"

UBFCartMovementComponent::UBFCartMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UBFCartMovementComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UBFCartMovementComponent::SetThrottle(float Value)
{
    ThrottleInput = FMath::Clamp(Value, -1.f, 1.f);
}

void UBFCartMovementComponent::SetSteer(float Value)
{
    SteerInput = FMath::Clamp(Value, -1.f, 1.f);
}

void UBFCartMovementComponent::SetDriftHeld(bool bHeld)
{
    bDriftHeld = bHeld;
}

void UBFCartMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!PawnOwner || !UpdatedComponent)
    {
        return;
    }

    // 드리프트 입력 변화 감지
    const bool bJustPressed = (bDriftHeld && !bWasDriftHeld);
    const bool bJustReleased = (!bDriftHeld && bWasDriftHeld);
    bWasDriftHeld = bDriftHeld;

    if (bJustPressed)
    {
        TryStartDrift();
    }

    if (DriftState == ECartDriftState::Drifting)
    {
        UpdateDrift(DeltaTime);

        if (bJustReleased)
        {
            EndDriftAndBoost();
        }
    }
    
    // ===== Gravity =====
    if (!bIsGrounded)
    {
        Velocity.Z += GravityZ * DeltaTime;
        Velocity.Z = FMath::Clamp(Velocity.Z, -MaxFallSpeed, MaxFallSpeed);
    }
    else
    {
        // 바닥에 붙어 있으면 Z 속도 제거
        Velocity.Z = FMath::Min(Velocity.Z, 0.f);
    }

    UpdateArcadeMovement(DeltaTime);

    // 실제 이동 적용 (Sweep로 충돌 처리)
    if (!Velocity.IsNearlyZero())
    {
        const FVector Delta = Velocity * DeltaTime;
        FHitResult Hit;
        SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);

        if (Hit.IsValidBlockingHit())
        {
            // 벽에 부딪히면 미끄러지듯이 속도 반사/감쇠
            const FVector Normal = Hit.ImpactNormal.GetSafeNormal();
            Velocity = FVector::VectorPlaneProject(Velocity, Normal) * 0.55f;
            SlideAlongSurface(Delta, 1.f - Hit.Time, Normal, Hit);
        }
    }
    
    // 중력 체크
    FHitResult GroundHit;
    const FVector Start = UpdatedComponent->GetComponentLocation();
    const FVector End   = Start - FVector(0.f, 0.f, 20.f);

    bIsGrounded = GetWorld()->LineTraceSingleByChannel(
        GroundHit,
        Start,
        End,
        ECC_Visibility
    );
}

float UBFCartMovementComponent::ComputeTurnRateDegPerSec() const
{
    const float Speed = Velocity.Size();
    const float SpeedAlpha = FMath::Clamp(Speed / MaxSpeed, 0.f, 1.f);

    // 고속에서 조향 둔화
    const float Damping = FMath::Lerp(1.f, 1.f - HighSpeedSteerDamping, SpeedAlpha);

    float TurnRate = BaseTurnRateDegPerSec * Damping;

    if (DriftState == ECartDriftState::Drifting)
    {
        TurnRate *= DriftTurnMultiplier;
    }

    return TurnRate;
}

void UBFCartMovementComponent::UpdateArcadeMovement(float DeltaTime)
{
    // 부스트 적용
    const bool bBoosting = (BoostTimeRemaining > 0.f);
    const float EffectiveMaxSpeed = MaxSpeed + (bBoosting ? BoostMaxSpeedBonus : 0.f);
    const float EffectiveAccel = Accel + (bBoosting ? BoostAccelBonus : 0.f);

    if (bBoosting)
    {
        BoostTimeRemaining = FMath::Max(0.f, BoostTimeRemaining - DeltaTime);
        if (BoostTimeRemaining <= 0.f)
        {
            ActiveBoostTier = ECartBoostTier::None;
        }
    }

    // Forward 기준 가속/감속(마리오카트 느낌: 속도는 Forward로 수렴)
    const FVector Forward = UpdatedComponent->GetForwardVector();
    float Speed = FVector::DotProduct(Velocity, Forward); // Forward 방향 속도 성분

    if (ThrottleInput > 0.f)
    {
        Speed += EffectiveAccel * ThrottleInput * DeltaTime;
    }
    else if (ThrottleInput < 0.f)
    {
        Speed += BrakeDecel * ThrottleInput * DeltaTime; // 음수면 감속
    }
    else
    {
        // 입력 없을 때 코스팅 감속 (마리오카트는 생각보다 빨리 속도가 줄어듦)
        const float Decel = (DriftState == ECartDriftState::Drifting) ? (CoastDecel * 0.6f) : CoastDecel;

        // Speed를 0으로 일정하게 깎아나감
        Speed = FMath::FInterpConstantTo(Speed, 0.f, DeltaTime, Decel);

        if (FMath::Abs(Speed) < StopSpeedThreshold)
        {
            Speed = 0.f;
        }
    }

    // 드리프트 중 추가 속도 손실
    if (DriftState == ECartDriftState::Drifting)
    {
        Speed = FMath::Max(0.f, Speed - DriftSpeedLossPerSec * DeltaTime);
    }

    Speed = FMath::Clamp(Speed, -EffectiveMaxSpeed * 0.35f, EffectiveMaxSpeed);

    // 조향으로 회전
    const float TurnRate = ComputeTurnRateDegPerSec();
    const float YawDelta = TurnRate * SteerInput * DeltaTime;

    if (FMath::Abs(YawDelta) > KINDA_SMALL_NUMBER)
    {
        FRotator R = UpdatedComponent->GetComponentRotation();
        R.Yaw += YawDelta;
        UpdatedComponent->SetWorldRotation(R);
    }

    // 마리오카트식: 현재 Forward 방향으로 속도를 정렬(측방 속도 억제)
    const float PrevZ = Velocity.Z; // 중력/점프 등 Z는 따로 관리
    
    FVector NewVel = Forward * Speed;

    if (DriftState == ECartDriftState::Drifting)
    {
        // 드리프트 중에는 약간의 측방 슬립을 허용(조향 입력에 따라)
        const FVector Right = UpdatedComponent->GetRightVector();
        const float SideSlip = 420.f * SteerInput; // 느낌 값(튜닝 대상)
        NewVel += Right * SideSlip;
    }

    NewVel.Z = PrevZ;   // 핵심: Z 유지
    Velocity = NewVel;
}

void UBFCartMovementComponent::TryStartDrift()
{
    if (DriftState != ECartDriftState::None)
    {
        return;
    }

    const float Speed = Velocity.Size();
    if (Speed < MinSpeedToDrift)
    {
        return;
    }

    if (FMath::Abs(SteerInput) < MinSteerToStartDrift)
    {
        return;
    }

    DriftState = ECartDriftState::Drifting;
    DriftCharge = 0.f;
}

void UBFCartMovementComponent::UpdateDrift(float DeltaTime)
{
    // 조향 강할수록 더 빨리 차지
    const float SteerStrength = FMath::Clamp(FMath::Abs(SteerInput), 0.f, 1.f);
    const float Gain = DriftChargeRate * (0.35f + 0.65f * SteerStrength);
    DriftCharge += Gain * DeltaTime;
}

ECartBoostTier UBFCartMovementComponent::EvaluateBoostTier() const
{
    if (DriftCharge >= UltraThreshold) return ECartBoostTier::Ultra;
    if (DriftCharge >= SuperThreshold) return ECartBoostTier::Super;
    if (DriftCharge >= MiniThreshold)  return ECartBoostTier::Mini;
    return ECartBoostTier::None;
}

float UBFCartMovementComponent::GetBoostDuration(ECartBoostTier Tier) const
{
    switch (Tier)
    {
        case ECartBoostTier::Mini:  return MiniBoostDuration;
        case ECartBoostTier::Super: return SuperBoostDuration;
        case ECartBoostTier::Ultra: return UltraBoostDuration;
        default:                    return 0.f;
    }
}

void UBFCartMovementComponent::EndDriftAndBoost()
{
    const ECartBoostTier Tier = EvaluateBoostTier();

    DriftState = ECartDriftState::None;

    if (Tier != ECartBoostTier::None)
    {
        ActiveBoostTier = Tier;
        BoostTimeRemaining = GetBoostDuration(Tier);
    }

    DriftCharge = 0.f;
}
