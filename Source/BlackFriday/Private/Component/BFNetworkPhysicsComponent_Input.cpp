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

static bool IsNewerFrame(uint16 A, uint16 B)
{
	return (uint16)(A - B) < 32768;
}

void UBFNetworkPhysicsComponent::ServerReceiveInput_Implementation(FBFMoveInputNet Input)
{
	if (bHasRecvClientFrame && !IsNewerFrame(Input.ClientFrame, LastRecvClientFrame))
	{
		return;
	}
	
	bHasRecvClientFrame = true;
	LastRecvClientFrame = Input.ClientFrame;
	
	ServerInput = Input;

	if (MoveComp)
	{
		MoveComp->SetCurrentInput(Input);
		MoveComp->ApplyInputImmediately(Input);
	}
	
	if (Prim && (Input.MoveX != 0 || Input.MoveY != 0 || (Input.Buttons & 0x01)))
	{
		if (!Prim->IsSimulatingPhysics()) Prim->SetSimulatePhysics(true);
		if (Prim->GetBodyInstance()) Prim->GetBodyInstance()->WakeInstance();
	}
}

bool UBFNetworkPhysicsComponent::ServerReceiveInput_Validate(FBFMoveInputNet Input)
{
	return true;
}

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
}
