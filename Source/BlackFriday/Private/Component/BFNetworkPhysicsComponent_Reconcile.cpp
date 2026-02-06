#include "Component/BFNetworkPhysicsComponent.h"
#include "Component/BFPhysicsMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerState.h"

/**
 * Owner(AutonomousProxy) 리컨실(Reconcile) 처리
 *
 * 목적:
 * - 클라이언트가 예측(simulation)한 물리 상태를
 *   서버에서 내려온 "권위 상태"에 서서히 수렴시키는 것
 *
 * 방식:
 * - 작은 오차: PD 제어기로 부드럽게 보정
 * - 큰 오차: 즉시 텔레포트
 *
 * 특징:
 * - 위치는 PD 제어 기반 Force 보정
 * - 회전은 물리 회전 충돌을 피하기 위해 보간(RInterp)
 */
void UBFNetworkPhysicsComponent::ApplyOwnerReconcile(float DeltaTime)
{
    // 필수 조건:
    // - 물리 컴포넌트 존재
    // - 서버 상태 수신됨
    // - 실제로 물리 시뮬레이션 중
    if (!Prim || !bHasOwnerState || !Prim->IsSimulatingPhysics()) return;

    // 서버에서 받은 최신 권위 상태
    const FVector ServerPos = (FVector)LastOwnerState.Pos;
    const FVector ServerVel = (FVector)LastOwnerState.LinVel;

    // 현재 로컬(예측) 상태
    const FVector LocalPos = Prim->GetComponentLocation();
    const FVector LocalVel = Prim->GetPhysicsLinearVelocity();

    // 위치 오차 벡터 (서버 - 로컬)
    const FVector PosError = ServerPos - LocalPos;
    const float Dist = PosError.Size();

    // DeadZone 이내면 시각적으로 의미 없는 오차 → 무시
    if (Dist < OwnerDeadZone) return;

    // 너무 큰 오차는 보정하지 않고 즉시 동기화 (텔레포트)
    if (Dist > OwnerTeleportDist)
    {
        Prim->SetWorldLocation(ServerPos, false, nullptr, ETeleportType::TeleportPhysics);
        Prim->SetPhysicsLinearVelocity(ServerVel);
        return;
    }

    /*
     * === PD 제어 기반 위치 보정 ===
     *
     * P(비례): 위치 오차에 비례한 속도 보정
     * D(미분): 현재 속도와 목표 속도의 차이를 줄이기 위한 가속
     */

    // 서버 속도 + 위치 오차 기반 보정 속도
    FVector DesiredVel = ServerVel + (PosError * OwnerPosCorrectGain);

    // 현재 속도 대비 목표 속도 오차
    FVector VelError = DesiredVel - LocalVel;

    // 속도 오차를 가속도로 변환 (D 항)
    FVector Accel = VelError * OwnerVelCorrectGain;

    // 최대 보정 가속도 제한 (물리 폭주 방지)
    if (OwnerMaxCorrectionAccel > 0.f &&
        Accel.SizeSquared() > OwnerMaxCorrectionAccel * OwnerMaxCorrectionAccel)
    {
        Accel = Accel.GetSafeNormal() * OwnerMaxCorrectionAccel;
    }

    // 물리 Force = Mass * Acceleration
    if (Prim->IsSimulatingPhysics())
    {
        const float Mass = Prim->GetMass();
        Prim->AddForce(Accel * Mass);
    }

    /*
     * === 회전 보정 ===
     *
     * - 회전은 Force 기반 보정보다
     *   RInterp로 직접 보간하는 편이 안정적
     * - TeleportPhysics를 사용해
     *   물리 충돌/관성 간섭 최소화
     */
    const FRotator ServerRot = LastOwnerState.Rot;
    const FRotator NewRot =
        FMath::RInterpTo(
            Prim->GetComponentRotation(),
            ServerRot,
            DeltaTime,
            10.0f
        );

    Prim->SetWorldRotation(NewRot, false, nullptr, ETeleportType::TeleportPhysics);
}

/**
 * Remote(SimulatedProxy) 스무딩 처리
 *
 * 목적:
 * - 네트워크로 점프된 상태들을
 *   부드럽게 보간하여 시각적 끊김 제거
 *
 * 특징:
 * - PrevState → TargetState 사이를 보간
 * - 핑(Ping)을 고려해 보간 속도 자동 조절
 */
void UBFNetworkPhysicsComponent::ApplyRemoteSmoothing(float DeltaTime)
{
    if (!Prim) return;

    /*
     * === 현재 핑 추정 ===
     *
     * - PlayerState에서 Ping(ms) 획득
     * - 너무 작은 값 / 큰 값은 Clamp
     */
    float CurrentPing = 0.05f; // 기본값 (50ms)
    if (APawn* P = Cast<APawn>(GetOwner()))
    {
        if (P->GetPlayerState())
        {
            CurrentPing = P->GetPlayerState()->GetPingInMilliseconds() * 0.001f;
        }
    }
    CurrentPing = FMath::Clamp(CurrentPing, 0.0f, 0.4f);

    /*
     * === SmoothAlpha 증가 속도 계산 ===
     *
     * - 서버 상태 간 시간 간격(TimeDiff)
     * - 핑을 일부 반영하여 목표 보간 속도 결정
     */
    if (RemoteInterpSpeed > 0.f)
    {
        float TimeDiff = TargetState.ServerTime - PrevState.ServerTime;
        if (TimeDiff < 0.001f)
        {
            // 서버 타임 이상치 방어
            TimeDiff = 0.033f;
        }

        // 핑이 높을수록 보간 시간을 늘려 튐 방지
        float AdjustedTime = TimeDiff + (CurrentPing * 0.2f);

        // 시간 기반 보간 속도 역수
        float CalculatedSpeed = 1.0f / AdjustedTime;

        // 너무 느리거나 빠르지 않게 제한
        float FinalSpeed =
            FMath::Clamp(
                CalculatedSpeed,
                RemoteInterpSpeed * 0.8f,
                RemoteInterpSpeed * 2.0f
            );

        SmoothAlpha += DeltaTime * FinalSpeed;
    }
    else
    {
        // 보간 비활성화 시 즉시 도착
        SmoothAlpha = 1.f;
    }

    /*
     * === 보간 완료 단계 ===
     * TargetState에 거의 도달했을 때
     */
    if (SmoothAlpha >= 1.f)
    {
        FVector TargetPos = TargetState.Pos;
        FRotator TargetRot = TargetState.Rot;

        // 최종 위치/회전은 VInterp/RInterp로 추가 완화
        FVector NewPos =
            FMath::VInterpTo(
                Prim->GetComponentLocation(),
                TargetPos,
                DeltaTime,
                VelLerpSpeed
            );

        FRotator NewRot =
            FMath::RInterpTo(
                Prim->GetComponentRotation(),
                TargetRot,
                DeltaTime,
                VelLerpSpeed
            );

        Prim->SetWorldLocationAndRotation(
            NewPos,
            NewRot,
            false,
            nullptr,
            ETeleportType::TeleportPhysics
        );

        // 서버 속도를 그대로 반영
        Prim->SetPhysicsLinearVelocity((FVector)TargetState.LinVel);
    }
    /*
     * === 보간 진행 중 ===
     * Prev → Target 상태 사이 선형 보간
     */
    else
    {
        FVector StartPos = PrevState.Pos;
        FVector EndPos = TargetState.Pos;
        FVector NewPos = FMath::Lerp(StartPos, EndPos, SmoothAlpha);

        FRotator StartRot = PrevState.Rot;
        FRotator EndRot = TargetState.Rot;
        FRotator NewRot = FMath::Lerp(StartRot, EndRot, SmoothAlpha);

        Prim->SetWorldLocationAndRotation(
            NewPos,
            NewRot,
            false,
            nullptr,
            ETeleportType::TeleportPhysics
        );

        // 속도도 동일한 알파로 보간
        FVector LerpVel =
            FMath::Lerp(
                FVector(PrevState.LinVel),
                FVector(TargetState.LinVel),
                SmoothAlpha
            );

        Prim->SetPhysicsLinearVelocity(LerpVel);
    }
}