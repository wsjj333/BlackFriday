#include "Component/BFNetworkPhysicsComponent.h"
#include "Component/BFPhysicsMovementComponent.h"
#include "Components/PrimitiveComponent.h"

void UBFNetworkPhysicsComponent::SetMoveInput(FVector2D Move) { LocalMove = Move; }
void UBFNetworkPhysicsComponent::SetControlYawDegrees(float YawDegrees) { LocalYaw = YawDegrees; }
void UBFNetworkPhysicsComponent::SetJumpHeld(bool bHeld) { bLocalJumpHeld = bHeld; if (bHeld) bJumpHoldLatched = true; }


FBFMoveInputNet UBFNetworkPhysicsComponent::BuildInputPacket() const
{
	FBFMoveInputNet In;
	In.MoveX = (int16)(FMath::Clamp(LocalMove.X, -1.f, 1.f) * 32767.f);
	In.MoveY = (int16)(FMath::Clamp(LocalMove.Y, -1.f, 1.f) * 32767.f);
	In.ControlYaw100 = (int16)(FRotator::NormalizeAxis(LocalYaw) * 100.f);
	if (bLocalJumpHeld || bJumpHoldLatched) In.Buttons |= 0x01;
	In.ClientFrame = ClientFrameCounter;
	return In;
}

// [핵심] 서버가 입력을 받자마자 즉시 실행 (Tick 대기 안 함)
void UBFNetworkPhysicsComponent::ServerReceiveInput_Implementation(FBFMoveInputNet Input)
{
	ServerInput = Input;

	// 무브먼트 컴포넌트에 즉시 적용 요청
	if (MoveComp)
	{
		MoveComp->SetCurrentInput(Input); // 데이터 갱신
		MoveComp->ApplyInputImmediately(Input); // ★ 물리 즉시 적용!
	}
	
	// 물리 깨우기 및 강제 넷 업데이트
	if (Prim && (Input.MoveX != 0 || Input.MoveY != 0 || (Input.Buttons & 0x01)))
	{
		if (!Prim->IsSimulatingPhysics()) Prim->SetSimulatePhysics(true);
		if (Prim->GetBodyInstance()) Prim->GetBodyInstance()->WakeInstance();
		GetOwner()->ForceNetUpdate(); 
	}
}

bool UBFNetworkPhysicsComponent::ServerReceiveInput_Validate(FBFMoveInputNet Input)
{
	return true;
}

// 기타 조회/보간 함수
FVector UBFNetworkPhysicsComponent::GetReplicatedVelocity() const
{
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
		return (FVector)RepStateOwner.LinVel;
	return (FVector)RepState.LinVel;
}

void UBFNetworkPhysicsComponent::OnRep_PhysicsState()
{
	PrevState = TargetState;
	TargetState = RepState;
	SmoothAlpha = 0.f;
	
	// 텔레포트
	if (Prim)
	{
		float DistSq = FVector::DistSquared((FVector)TargetState.Pos, Prim->GetComponentLocation());
		if (DistSq > TeleportDist * TeleportDist)
		{
			Prim->SetWorldLocationAndRotation((FVector)TargetState.Pos, TargetState.Rot, false, nullptr, ETeleportType::TeleportPhysics);
			SmoothAlpha = 1.0f;
		}
	}
}

void UBFNetworkPhysicsComponent::OnRep_PhysicsStateOwner()
{
	bHasOwnerState = true;
	LastOwnerState = RepStateOwner;

	if (!bUseClientPrediction)
	{
		PrevState = TargetState;
		TargetState = RepStateOwner;
		SmoothAlpha = 0.f;
		// 텔레포트 로직 동일...
	}
}
