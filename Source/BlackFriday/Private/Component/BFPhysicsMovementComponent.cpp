// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/BFPhysicsMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Component/BFNetworkPhysicsComponent.h"
#include "Components/PrimitiveComponent.h"

UBFPhysicsMovementComponent::UBFPhysicsMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	
	// 기본값 튜닝
	MoveForce = 180000.f;
	BrakingLinearDamping = 10.0f;
	MovingLinearDamping = 1.0f;
	MaxSpeed = 800.f;
	GroundTraceLength = 120.f;
}

void UBFPhysicsMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	CachePrimitive();

	// 1. 유령 컴포넌트 삭제
	if (AActor* Owner = GetOwner())
	{
		TArray<UPawnMovementComponent*> AllMoveComps;
		Owner->GetComponents(AllMoveComps);
		for (UPawnMovementComponent* MC : AllMoveComps)
		{
			if (MC && MC != this) MC->DestroyComponent(); 
		}
	}

	// 2. 메시 설정
	if (AActor* Owner = GetOwner())
	{
		USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
		
		if (Mesh && Prim)
		{
			Mesh->SetSimulatePhysics(false);
			Mesh->SetAllBodiesSimulatePhysics(false);
			Mesh->SetCollisionProfileName(TEXT("NoCollision"));
			Mesh->AttachToComponent(Prim, FAttachmentTransformRules::SnapToTargetIncludingScale);
			Mesh->SetRelativeLocation(FVector(0.f, 0.f, -90));
			Mesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		}
	}

	if (Prim)
	{
		SetUpdatedComponent(Prim);
	}

	if (auto* NetComp = GetOwner()->FindComponentByClass<UBFNetworkPhysicsComponent>())
	{
		AddTickPrerequisiteComponent(NetComp);
	}
	
	// [수정] FBodyInstance 에러 해결
	// 코드에서 서브스테핑을 강제하는 부분은 제거했습니다.
	// 대신 DefaultEngine.ini 에서 설정을 켜야 합니다. (아래 설명 참조)
	if (Prim && Prim->GetBodyInstance())
	{
		FBodyInstance* BI = Prim->GetBodyInstance();
		// CCD는 여기서 설정 가능
		BI->SetUseCCD(true);
		
		// Solver Iteration은 설정 가능하면 높임 (버전 따라 다를 수 있음, 에러나면 이 두 줄도 삭제)
		BI->PositionSolverIterationCount = 8;
		BI->VelocitySolverIterationCount = 2; 
	}
}

void UBFPhysicsMovementComponent::CachePrimitive()
{
	if (PhysicsPrimitiveOverride)
	{
		Prim = PhysicsPrimitiveOverride;
	}
	else if (AActor* Owner = GetOwner())
	{
		Prim = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
	}
}

void UBFPhysicsMovementComponent::ApplyInputImmediately(const FBFMoveInputNet& Input)
{
	if (!Prim || !Prim->IsSimulatingPhysics()) return;
	
	bool bJump = (Input.Buttons & 0x01) != 0;
	if (bJump && JumpCooldownTime <= 0.f && bGrounded)
	{
		FVector V = Prim->GetPhysicsLinearVelocity();
		V.Z = 0.f;
		Prim->SetPhysicsLinearVelocity(V);
		Prim->SetLinearDamping(0.1f);
		Prim->AddImpulse(FVector(0.f, 0.f, JumpImpulse), NAME_None, true);
		
		bGrounded = false;
		JumpCooldownTime = 0.3f;
		if (Prim->GetBodyInstance()) Prim->GetBodyInstance()->WakeInstance();
	}
}

void UBFPhysicsMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Prim) return;
	if (!UpdatedComponent) SetUpdatedComponent(Prim);

	const bool bSimulating = Prim->IsSimulatingPhysics();
	const FVector CurrentPhysVel = bSimulating ? Prim->GetPhysicsLinearVelocity() : FVector::ZeroVector;
	const FVector Loc = Prim->GetComponentLocation();

	// 1. 바닥 체크
	{
		const FVector TraceStart = Loc;
		float CheckLength = GroundTraceLength;
		
		if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
		{
			CheckLength *= 1.2f; 
		}

		const FVector TraceEnd = TraceStart - (FVector::UpVector * CheckLength);
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(GetOwner());

		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, GroundTraceChannel, Params);
		if (bHit && Hit.Normal.Z > GroundedDotThreshold)
		{
			bGrounded = true;
			GroundNormal = Hit.Normal;
		}
		else
		{
			bGrounded = false;
			GroundNormal = FVector::UpVector;
		}
	}

	// 2. 타이머
	if (JumpBufferTime > 0.f) JumpBufferTime -= DeltaTime;
	if (JumpCooldownTime > 0.f) JumpCooldownTime -= DeltaTime;
	
	// 3. 마찰력 제어
	bool bHasInput = (MoveX != 0.f || MoveY != 0.f);
	if (bGrounded)
		Prim->SetLinearDamping(bHasInput ? MovingLinearDamping : BrakingLinearDamping);
	else
		Prim->SetLinearDamping(0.1f);

	// 4. 로컬 클라이언트 점프
	if (APawn* P = Cast<APawn>(GetOwner()))
	{
		if (P->IsLocallyControlled())
		{
			if (bGrounded && bSimulating && JumpBufferTime > 0.f && JumpCooldownTime <= 0.f)
			{
				FVector V = Prim->GetPhysicsLinearVelocity();
				V.Z = 0.f;
				Prim->SetPhysicsLinearVelocity(V);
				Prim->SetLinearDamping(0.1f);
				Prim->AddImpulse(FVector(0.f, 0.f, JumpImpulse), NAME_None, true);
				
				bGrounded = false;
				JumpBufferTime = 0.f;
				JumpCooldownTime = 0.3f;
				if (Prim->GetBodyInstance()) Prim->GetBodyInstance()->WakeInstance();
			}
		}
	}
	
	// 5. 이동 힘 가하기 (Physics Force)
	if (bHasInput && bSimulating)
	{
		// Tick에서도 지속적으로 힘을 줘야 부드러움 (서버는 ApplyInputImmediately도 하고 이것도 함)
		FVector LocalDir(MoveX, MoveY, 0.f);
		if (LocalDir.SizeSquared() > 1.f) LocalDir.Normalize();

		const FRotator YawRot(0.f, InputYawDeg, 0.f);
		FVector WorldDir = YawRot.RotateVector(LocalDir);

		if (bGrounded)
			WorldDir = FVector::VectorPlaneProject(WorldDir, GroundNormal).GetSafeNormal();
		else
		{
			WorldDir.Z = 0.f;
			WorldDir.Normalize();
			WorldDir *= AirControl;
		}

		if (CurrentPhysVel.Size2D() < MaxSpeed)
		{
			Prim->AddForce(WorldDir * MoveForce);
		}
		FVector NewVel = Prim->GetPhysicsLinearVelocity();
		float NewSpeed2D = NewVel.Size2D();
		if (NewSpeed2D > MaxSpeed)
		{
			// Z축(낙하 속도)은 건드리지 말고 수평 속도만 비율대로 줄임
			float Scale = MaxSpeed / NewSpeed2D;
			NewVel.X *= Scale;
			NewVel.Y *= Scale;
			Prim->SetPhysicsLinearVelocity(NewVel);
		}
	}
	
	// 6. 스무싱
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		FVector TargetVel = FVector::ZeroVector;
		if (auto* NetComp = GetOwner()->FindComponentByClass<UBFNetworkPhysicsComponent>())
		{
			TargetVel = NetComp->GetReplicatedVelocity();
		}
		SmoothAnimVelocity = FMath::VInterpTo(SmoothAnimVelocity, TargetVel, DeltaTime, 15.0f);
	}
	else
	{
		if (Prim) SmoothAnimVelocity = Prim->GetPhysicsLinearVelocity();
	}
	
	// 7. 디버그
	if (bDebugMove)
	{
		DebugAcc += DeltaTime;
		if (DebugAcc >= DebugInterval)
		{
			DebugAcc = 0.f;
			UE_LOG(LogTemp, Warning, TEXT("[BFMove] Loc=%s Vel=%s JBuf=%.2f Cool=%.2f Ground=%d"),
				*Loc.ToCompactString(),
				*CurrentPhysVel.ToCompactString(),
				JumpBufferTime,
				JumpCooldownTime,
				bGrounded ? 1 : 0
			);
		}
	}
}

void UBFPhysicsMovementComponent::SetCurrentInput(const FBFMoveInputNet& InInput)
{
	MoveX = (float)InInput.MoveX / 32767.f;
	MoveY = (float)InInput.MoveY / 32767.f;
	InputYawDeg = (float)InInput.ControlYaw100 / 100.f;

	const bool bNewJumpHeld = (InInput.Buttons & 0x01) != 0;
	if (JumpCooldownTime <= 0.f && bNewJumpHeld && !bPrevJumpHeld)
	{
		JumpBufferTime = 0.2f;
	}
	bPrevJumpHeld = bNewJumpHeld;
	bJumpHeld = bNewJumpHeld;
}

FVector UBFPhysicsMovementComponent::GetBFVelocity() const
{
	if (APawn* P = Cast<APawn>(GetOwner()))
	{
		if (P->GetLocalRole() == ROLE_AutonomousProxy)
		{
			if (FMath::Abs(MoveX) > 0.01f || FMath::Abs(MoveY) > 0.01f)
			{
				return P->GetActorForwardVector() * MaxSpeed;
			}
			return FVector::ZeroVector;
		}
	}
	return SmoothAnimVelocity;
}