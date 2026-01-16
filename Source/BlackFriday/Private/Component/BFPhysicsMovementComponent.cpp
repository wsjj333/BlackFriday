// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/BFPhysicsMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UBFPhysicsMovementComponent::UBFPhysicsMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UBFPhysicsMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	CachePrimitive();
}

void UBFPhysicsMovementComponent::CachePrimitive()
{
	if (PhysicsPrimitiveOverride)
	{
		Prim = PhysicsPrimitiveOverride;
	}
	else
	{
		// UpdatedComponent가 있으면 그걸, 없으면 Owner Root가 Primitive인지 확인
		if (UpdatedComponent)
		{
			Prim = Cast<UPrimitiveComponent>(UpdatedComponent);
		}
		if (!Prim)
		{
			if (AActor* Owner = GetOwner())
			{
				Prim = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
			}
		}
	}

	if (Prim)
	{
		SetUpdatedComponent(Prim);
	}
}

void UBFPhysicsMovementComponent::SetCurrentInput(const FBFMoveInputNet& InInput)
{
	CurrentInput = InInput;
}

bool UBFPhysicsMovementComponent::ShouldSimulatePhysicsMove() const
{
	const APawn* P = Cast<APawn>(GetOwner());
	if (!P) return false;

	// 서버는 권한 물리 시뮬
	if (P->HasAuthority()) return true;

	// 클라에서는 로컬 컨트롤러만 예측 시뮬
	return P->IsLocallyControlled();
}

void UBFPhysicsMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Prim)
	{
		CachePrimitive();
	}
	if (!Prim) return;

	// 물리 시뮬 대상이 아니면(원격 프록시) 힘 적용 안 함
	if (!ShouldSimulatePhysicsMove()) return;

	if (!Prim->IsSimulatingPhysics()) return;

	UpdateGroundInfo();
	ApplyForces(DeltaTime);
}

void UBFPhysicsMovementComponent::UpdateGroundInfo()
{
	if (!Prim) return;

	const FVector Start = Prim->GetComponentLocation();
	const FVector End = Start - FVector(0,0,GroundTraceLength);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BF_GroundTrace), false);
	Params.AddIgnoredActor(GetOwner());

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, GroundTraceChannel, Params);

	if (bHit)
	{
		const float Dot = FVector::DotProduct(Hit.ImpactNormal.GetSafeNormal(), FVector::UpVector);
		bGrounded = (Dot >= GroundedDotThreshold);
		GroundNormal = Hit.ImpactNormal.GetSafeNormal();
	}
	else
	{
		bGrounded = false;
		GroundNormal = FVector::UpVector;
	}
}

void UBFPhysicsMovementComponent::ApplyForces(float DeltaTime)
{
	const float MoveX = BF_UnpackAxis(CurrentInput.MoveX);
	const float MoveY = BF_UnpackAxis(CurrentInput.MoveY);
	const float Yaw = BF_UnpackYaw100(CurrentInput.ControlYaw100);

	const bool bJumpHeld = (CurrentInput.Buttons & 0x01) != 0;
	const bool bJumpPressedThisFrame = (bJumpHeld && ((PrevButtons & 0x01) == 0));

	PrevButtons = CurrentInput.Buttons;

	// 댐핑 스위치
	const bool bHasMoveInput = !FMath::IsNearlyZero(MoveX, 0.02f) || !FMath::IsNearlyZero(MoveY, 0.02f);
	Prim->SetLinearDamping(bHasMoveInput ? MovingLinearDamping : BrakingLinearDamping);

	// 이동 방향(카메라 yaw 기준)
	FVector LocalDir(MoveX, MoveY, 0.f);
	float InputMag = FMath::Min(1.f, LocalDir.Size());
	if (InputMag > KINDA_SMALL_NUMBER)
	{
		LocalDir /= InputMag;
	}

	const FRotator YawRot(0.f, Yaw, 0.f);
	FVector WorldDir = YawRot.RotateVector(LocalDir);

	if (bGrounded)
	{
		WorldDir = FVector::VectorPlaneProject(WorldDir, GroundNormal).GetSafeNormal();
	}
	else
	{
		WorldDir = WorldDir.GetSafeNormal();
		InputMag *= AirControl;
	}

	if (!WorldDir.IsNearlyZero())
	{
		const FVector Force = WorldDir * (MoveForce * InputMag);
		Prim->AddForce(Force, NAME_None, true);
	}

	// 점프(바닥에서만)
	if (bGrounded && bJumpPressedThisFrame)
	{
		const FVector Impulse = FVector::UpVector * JumpImpulse;
		Prim->AddImpulse(Impulse, NAME_None, true);
	}

	// 속도 제한(수평만)
	const FVector Vel = Prim->GetPhysicsLinearVelocity();
	const FVector Horiz(Vel.X, Vel.Y, 0.f);
	const float HorizSpeed = Horiz.Size();
	if (HorizSpeed > MaxSpeed)
	{
		const FVector ClampedHoriz = Horiz * (MaxSpeed / HorizSpeed);
		const FVector NewVel(ClampedHoriz.X, ClampedHoriz.Y, Vel.Z);
		Prim->SetPhysicsLinearVelocity(NewVel, false);
	}
}
