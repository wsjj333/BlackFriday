#include "Component/BFNetworkPhysicsComponent.h"
#include "Component/BFPhysicsMovementComponent.h"
#include "Components/PrimitiveComponent.h"

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

    if (Dist >OwnerTeleportDist)
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

    if (RemoteInterpSpeed > 0.f)
    {
        float TimeDiff = TargetState.ServerTime - PrevState.ServerTime;
        if (TimeDiff < 0.001f) TimeDiff = 0.033f;

        float CalculatedSpeed = 1.0f / TimeDiff;
        float FinalSpeed = FMath::Clamp(CalculatedSpeed, RemoteInterpSpeed * 0.8f, RemoteInterpSpeed * 1.5f);
        SmoothAlpha += DeltaTime * FinalSpeed;
    }
    else
    {
        SmoothAlpha = 1.f;
    }

    float DistSq = FVector::DistSquared(Prim->GetComponentLocation(), (FVector)TargetState.Pos);
    if (DistSq < 1.0f)
    {
        Prim->SetWorldLocationAndRotation((FVector)TargetState.Pos, TargetState.Rot, false, nullptr, ETeleportType::TeleportPhysics);
        Prim->SetPhysicsLinearVelocity(FVector::ZeroVector);
        if (MoveComp) MoveComp->SetCurrentInput(FBFMoveInputNet());
		
        SmoothAlpha = 1.0f;
        return;
    }

    if (SmoothAlpha >= 1.f)
    {
        FVector TargetPos = (FVector)TargetState.Pos;
        FRotator TargetRot = TargetState.Rot;

        FVector NewPos = FMath::VInterpTo(Prim->GetComponentLocation(), TargetPos, DeltaTime, VelLerpSpeed);
        FRotator NewRot = FMath::RInterpTo(Prim->GetComponentRotation(), TargetRot, DeltaTime, VelLerpSpeed);

        Prim->SetWorldLocationAndRotation(NewPos, NewRot, false, nullptr, ETeleportType::TeleportPhysics);
        Prim->SetPhysicsLinearVelocity((FVector)TargetState.LinVel);
    }
    else
    {
        FVector StartPos = (FVector)PrevState.Pos;
        FVector EndPos = (FVector)TargetState.Pos;
        FVector NewPos = FMath::Lerp(StartPos, EndPos, SmoothAlpha);

        FRotator StartRot = PrevState.Rot;
        FRotator EndRot = TargetState.Rot;
        FRotator NewRot = FMath::Lerp(StartRot, EndRot, SmoothAlpha);

        Prim->SetWorldLocationAndRotation(NewPos, NewRot, false, nullptr, ETeleportType::TeleportPhysics);
		
        FVector LerpVel = FMath::Lerp((FVector)PrevState.LinVel, (FVector)TargetState.LinVel, SmoothAlpha);
        Prim->SetPhysicsLinearVelocity(LerpVel);
    }
}