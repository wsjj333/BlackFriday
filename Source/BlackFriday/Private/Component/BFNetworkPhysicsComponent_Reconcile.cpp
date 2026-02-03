#include "Component/BFNetworkPhysicsComponent.h"
#include "Component/BFPhysicsMovementComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerState.h"

// PD 제어기 보정
void UBFNetworkPhysicsComponent::ApplyOwnerReconcile(float DeltaTime)
{
    if (!Prim || !bHasOwnerState || !Prim->IsSimulatingPhysics()) return;

    const FVector ServerPos = (FVector)LastOwnerState.Pos;
    const FVector ServerVel = (FVector)LastOwnerState.LinVel;
    const FVector LocalPos = Prim->GetComponentLocation();
    const FVector LocalVel = Prim->GetPhysicsLinearVelocity();

    const FVector PosError = ServerPos - LocalPos;
    const float Dist = PosError.Size();

    if (Dist < OwnerDeadZone) return;

    if (Dist > OwnerTeleportDist)
    {
        Prim->SetWorldLocation(ServerPos, false, nullptr, ETeleportType::TeleportPhysics);
        Prim->SetPhysicsLinearVelocity(ServerVel);
        return;
    }

    // PD 제어
    FVector DesiredVel = ServerVel + (PosError * OwnerPosCorrectGain); 
    
    FVector VelError = DesiredVel - LocalVel;
    FVector Accel = VelError * OwnerVelCorrectGain;
    
    if (OwnerMaxCorrectionAccel > 0.f && Accel.SizeSquared() > OwnerMaxCorrectionAccel * OwnerMaxCorrectionAccel)
    {
        Accel = Accel.GetSafeNormal() * OwnerMaxCorrectionAccel;
    }
    
    if (Prim->IsSimulatingPhysics())
    {
        const float Mass = Prim->GetMass();
        Prim->AddForce(Accel * Mass);
    }
    
    const FRotator ServerRot = LastOwnerState.Rot;
    const FRotator NewRot = FMath::RInterpTo(Prim->GetComponentRotation(), ServerRot, DeltaTime, 10.0f);
    Prim->SetWorldRotation(NewRot, false, nullptr, ETeleportType::TeleportPhysics);
}

// 보간 로직
void UBFNetworkPhysicsComponent::ApplyRemoteSmoothing(float DeltaTime)
{
    if (!Prim) return;

    float CurrentPing = 0.05f;
    if (APawn* P = Cast<APawn>(GetOwner()))
    {
        if (P->GetPlayerState())
            CurrentPing = P->GetPlayerState()->GetPingInMilliseconds() * 0.001f;
    }
    CurrentPing = FMath::Clamp(CurrentPing, 0.0f, 0.4f);

    if (RemoteInterpSpeed > 0.f)
    {
        float TimeDiff = TargetState.ServerTime - PrevState.ServerTime;
        if (TimeDiff < 0.001f) TimeDiff = 0.033f;
        
        float AdjustedTime = TimeDiff + (CurrentPing * 0.2f);
        float CalculatedSpeed = 1.0f / AdjustedTime;
        float FinalSpeed = FMath::Clamp(CalculatedSpeed, RemoteInterpSpeed * 0.8f, RemoteInterpSpeed * 2.0f);
        
        SmoothAlpha += DeltaTime * FinalSpeed;
    }
    else
    {
        SmoothAlpha = 1.f;
    }

    if (SmoothAlpha >= 1.f)
    {
        FVector TargetPos = TargetState.Pos;
        FRotator TargetRot = TargetState.Rot;

        FVector NewPos = FMath::VInterpTo(Prim->GetComponentLocation(), TargetPos, DeltaTime, VelLerpSpeed);
        FRotator NewRot = FMath::RInterpTo(Prim->GetComponentRotation(), TargetRot, DeltaTime, VelLerpSpeed);

        Prim->SetWorldLocationAndRotation(NewPos, NewRot, false, nullptr, ETeleportType::TeleportPhysics);
        
        Prim->SetPhysicsLinearVelocity((FVector)TargetState.LinVel);
    }
    else
    {
        FVector StartPos = PrevState.Pos;
        FVector EndPos = TargetState.Pos;
        FVector NewPos = FMath::Lerp(StartPos, EndPos, SmoothAlpha);

        FRotator StartRot = PrevState.Rot;
        FRotator EndRot = TargetState.Rot;
        FRotator NewRot = FMath::Lerp(StartRot, EndRot, SmoothAlpha);

        Prim->SetWorldLocationAndRotation(NewPos, NewRot, false, nullptr, ETeleportType::TeleportPhysics);
        
        FVector LerpVel = FMath::Lerp(FVector(PrevState.LinVel), FVector(TargetState.LinVel), SmoothAlpha);
        Prim->SetPhysicsLinearVelocity(LerpVel);
    }
}