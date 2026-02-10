// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/BFPhysicsMovementComponent.h"
#include "Component/BFNetworkPhysicsComponent.h"

#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"

UBFPhysicsMovementComponent::UBFPhysicsMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	// 기본 튜닝값
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

	// Mesh 캐싱 (Anim/PhysicalAnimation 등)
	if (AActor* Owner = GetOwner())
	{
		CachedMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}

	// NetComp 캐싱(중요: Tick마다 Find 하지 않기)
	CachedNetComp = GetOwner()
		? GetOwner()->FindComponentByClass<UBFNetworkPhysicsComponent>()
		: nullptr;

	if (Prim)
	{
		SetUpdatedComponent(Prim);

		// 충돌 발생 시 네트워크 보정 트리거
		Prim->OnComponentHit.AddDynamic(
			this,
			&UBFPhysicsMovementComponent::OnComponentHit
		);
		
		Prim->SetAngularDamping(8.f);
		
		if (Prim->GetBodyInstance())
		{
			// Pitch/Roll 잠그고 Yaw만 허용
			Prim->GetBodyInstance()->bLockXRotation = true; // Roll
			Prim->GetBodyInstance()->bLockYRotation = true; // Pitch
			Prim->GetBodyInstance()->bLockZRotation = false; // Yaw 허용
		}
	}

	// NetworkPhysicsComponent 이후에 Tick되도록 순서 보장
	if (CachedNetComp)
	{
		AddTickPrerequisiteComponent(CachedNetComp);
	}

	// SetupUpperBodyPhysics(); // 현재 비활성
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

	const bool bJump = (Input.Buttons & 0x01) != 0;

	// 즉시 점프 (서버/로컬 공통)
	if (bJump && JumpCooldownTime <= 0.f && bGrounded)
	{
		FVector V = Prim->GetPhysicsLinearVelocity();
		V.Z = 0.f;
		Prim->SetPhysicsLinearVelocity(V);

		Prim->SetLinearDamping(0.1f);
		Prim->AddImpulse(FVector(0.f, 0.f, JumpImpulse), NAME_None, true);

		bGrounded = false;
		JumpCooldownTime = 0.3f;

		if (Prim->GetBodyInstance())
			Prim->GetBodyInstance()->WakeInstance();

		if (GetOwner())
			GetOwner()->ForceNetUpdate();
	}
}

void UBFPhysicsMovementComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	UE_LOG(LogTemp, Warning, TEXT("AngVelDeg=%s"), *Prim->GetPhysicsAngularVelocityInDegrees().ToCompactString());

	if (!Prim)
	{
		CachePrimitive();
		if (!Prim) return;
	}

	if (!UpdatedComponent)
	{
		SetUpdatedComponent(Prim);
	}

	const bool bSimulating = Prim->IsSimulatingPhysics();
	const FVector CurrentPhysVel = bSimulating ? Prim->GetPhysicsLinearVelocity() : FVector::ZeroVector;
	const FVector Loc = Prim->GetComponentLocation();

	// ================= 지면 판정 =================
	{
		const FVector TraceStart = Loc;
		const FVector TraceEnd = TraceStart - FVector::UpVector * (GroundTraceLength * 1.2f);

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(BFGroundTrace), false, GetOwner());

		const bool bHit = GetWorld()->LineTraceSingleByChannel(
			Hit,
			TraceStart,
			TraceEnd,
			GroundTraceChannel,
			Params
		);

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

	// ================= 타이머 =================
	if (JumpBufferTime > 0.f)   JumpBufferTime -= DeltaTime;
	if (JumpCooldownTime > 0.f) JumpCooldownTime -= DeltaTime;

	const bool bHasInput = !FMath::IsNearlyZero(MoveX) || !FMath::IsNearlyZero(MoveY);

	// 지면/공중에 따른 감쇠 조절
	if (bGrounded)
	{
		Prim->SetLinearDamping(bHasInput ? MovingLinearDamping : BrakingLinearDamping);
	}
	else
	{
		Prim->SetLinearDamping(0.1f);
	}

	// ================= 점프 처리 =================
	if (APawn* P = Cast<APawn>(GetOwner()))
	{
		// 로컬 또는 서버만 점프 판정 수행
		if (P->IsLocallyControlled() || P->HasAuthority())
		{
			if (bGrounded && bSimulating &&
				JumpBufferTime > 0.f &&
				JumpCooldownTime <= 0.f)
			{
				FVector V = Prim->GetPhysicsLinearVelocity();
				V.Z = 0.f;
				Prim->SetPhysicsLinearVelocity(V);

				Prim->SetLinearDamping(0.1f);
				Prim->AddImpulse(FVector(0.f, 0.f, JumpImpulse), NAME_None, true);

				bGrounded = false;
				JumpBufferTime = 0.f;
				JumpCooldownTime = 0.3f;

				if (Prim->GetBodyInstance())
					Prim->GetBodyInstance()->WakeInstance();

				if (P->HasAuthority())
					P->ForceNetUpdate();
			}
		}
	}

	// ================= 이동 힘 적용 =================
	if (bHasInput && bSimulating)
	{
		FVector LocalDir(MoveX, MoveY, 0.f);
		if (LocalDir.SizeSquared() > 1.f)
			LocalDir.Normalize();

		const FRotator YawRot(0.f, InputYawDeg, 0.f);
		FVector WorldDir = YawRot.RotateVector(LocalDir);

		if (bGrounded)
		{
			WorldDir = FVector::VectorPlaneProject(WorldDir, GroundNormal).GetSafeNormal();
		}
		else
		{
			WorldDir.Z = 0.f;
			WorldDir.Normalize();
			WorldDir *= AirControl;
		}

		const float CurrentSpeed2D = Prim->GetPhysicsLinearVelocity().Size2D();
		const float SpeedRatio = FMath::Clamp(CurrentSpeed2D / MaxSpeed, 0.f, 1.f);
		const float DynamicMultiplier = FMath::Lerp(AccelMultiplier, 1.0f, SpeedRatio);

		Prim->AddForce(WorldDir * MoveForce * DynamicMultiplier);

		// 최대 속도 클램프
		FVector NewVel = Prim->GetPhysicsLinearVelocity();
		const float NewSpeed2D = NewVel.Size2D();
		if (NewSpeed2D > MaxSpeed)
		{
			const float Scale = MaxSpeed / NewSpeed2D;
			NewVel.X *= Scale;
			NewVel.Y *= Scale;
			Prim->SetPhysicsLinearVelocity(NewVel);
		}
	}
	// 입력 없음 + 지면 → 제동
	else if (bGrounded && bSimulating)
	{
		Prim->SetLinearDamping(BrakingLinearDamping);

		FVector Vel = Prim->GetPhysicsLinearVelocity();
		if (Vel.SizeSquared2D() < 100.f)
		{
			Vel.X = 0.f;
			Vel.Y = 0.f;
			Prim->SetPhysicsLinearVelocity(Vel);
		}
	}

	// ================= 회전 보간 =================
	if (bSimulating)
	{
		const FRotator CurrentRot = Prim->GetComponentRotation();

		// --------- (A) OrientRotationToMovement ON: Velocity 소스 + 각속도 제어 ----------
		if (bOrientRotationToMovement && (bGrounded || bOrientInAir))
		{
			float TargetYaw = CurrentRot.Yaw;

			const FVector Vel = Prim->GetPhysicsLinearVelocity();
			const FVector Vel2D(Vel.X, Vel.Y, 0.f);

			if (Vel2D.Size() >= MinSpeedToOrient)
			{
				TargetYaw = Vel2D.Rotation().Yaw;
			}

			const float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, TargetYaw);

			float DesiredYawRateDeg =
				DeltaYaw / FMath::Max(DeltaTime, KINDA_SMALL_NUMBER);

			DesiredYawRateDeg = FMath::Clamp(
				DesiredYawRateDeg,
				-RotationRateYawDegPerSec,
				 RotationRateYawDegPerSec
			);

			FVector AngVel = Prim->GetPhysicsAngularVelocityInDegrees();
			AngVel.X = 0.f;
			AngVel.Y = 0.f;
			AngVel.Z = DesiredYawRateDeg;

			AngVel.Z = FMath::Clamp(AngVel.Z, -MaxYawSpinDegPerSec, MaxYawSpinDegPerSec);

			Prim->SetPhysicsAngularVelocityInDegrees(AngVel, false);
		}
		// --------- (B) OrientRotationToMovement OFF: ControlYaw 고정 따라가기 (불안정 방지) ----------
		else
		{
			// 목표는 컨트롤 yaw (InputYawDeg)
			const float TargetYaw = InputYawDeg;

			// CMC 느낌: 초당 RotationRate 만큼만 회전 (각속도 말고 Yaw 자체를 돌림)
			const float MaxStep = RotationRateYawDegPerSec * DeltaTime;
			const float NewYaw = FMath::FixedTurn(CurrentRot.Yaw, TargetYaw, MaxStep);

			// 물리 각속도는 충돌 스핀만 죽이는 용도로 0으로 눌러줌
			FVector AngVel = Prim->GetPhysicsAngularVelocityInDegrees();
			AngVel.X = 0.f;
			AngVel.Y = 0.f;
			AngVel.Z = 0.f;
			Prim->SetPhysicsAngularVelocityInDegrees(AngVel, false);

			// TeleportPhysics 금지: 물리랑 싸움/스핀 증폭 가능
			const FRotator NewRot(0.f, NewYaw, 0.f);
			Prim->SetWorldRotation(NewRot, false, nullptr, ETeleportType::None);
		}
	}

	// ================= 애니메이션용 속도 =================
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		const FVector TargetVel = CachedNetComp
			? CachedNetComp->GetReplicatedVelocity()
			: FVector::ZeroVector;

		SmoothAnimVelocity = FMath::VInterpTo(SmoothAnimVelocity, TargetVel, DeltaTime, 15.0f);
	}
	else
	{
		SmoothAnimVelocity = Prim->GetPhysicsLinearVelocity();
	}

	// ================= 디버그 =================
	if (bDebugMove)
	{
		DebugAcc += DeltaTime;
		if (DebugAcc >= DebugInterval)
		{
			DebugAcc = 0.f;
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[BFMove] Loc=%s Vel=%s JBuf=%.2f Cool=%.2f Ground=%d"),
				*Loc.ToCompactString(),
				*CurrentPhysVel.ToCompactString(),
				JumpBufferTime,
				JumpCooldownTime,
				bGrounded ? 1 : 0
			);
		}
	}

	// ================= 가속도(애님 힌트) =================
	APawn* Pawn = GetPawnOwner();
	if (!Pawn)
	{
		CurrentAcceleration = FVector::ZeroVector;
		return;
	}

	// NetComp가 제공하는 입력 기반 가속(Owner), 프록시는 속도 기반 힌트
	UBFNetworkPhysicsComponent* NetComp = CachedNetComp
		? CachedNetComp.Get()
		: Pawn->FindComponentByClass<UBFNetworkPhysicsComponent>();

	if (!NetComp)
	{
		CurrentAcceleration = FVector::ZeroVector;
		return;
	}

	if (Pawn->IsLocallyControlled())
	{
		const FVector InputWS = NetComp->GetMoveInputWorldSpace();
		const FVector AccelDir = InputWS.GetClampedToMaxSize(1.f);
		CurrentAcceleration = AccelDir * MaxAcceleration;
		return;
	}

	const FVector Vel = UpdatedComponent ? UpdatedComponent->GetComponentVelocity() : FVector::ZeroVector;
	CurrentAcceleration = (Vel.SizeSquared() > SMALL_NUMBER)
		? (Vel.GetSafeNormal() * MaxAcceleration)
		: FVector::ZeroVector;
}

void UBFPhysicsMovementComponent::SetCurrentInput(const FBFMoveInputNet& InInput)
{
	MoveX = InInput.MoveX / 32767.f;
	MoveY = InInput.MoveY / 32767.f;

	InputYawDeg = InInput.ControlYaw100 / 100.f;

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
	// AutonomousProxy는 입력 기반 추정 속도 반환(애님용)
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

void UBFPhysicsMovementComponent::OnComponentHit(
	UPrimitiveComponent* HitComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit
)
{
	// 서버: 강한 충돌 발생 시 네트워크 업데이트
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (NormalImpulse.SizeSquared() > 1000000.f)
		{
			GetOwner()->ForceNetUpdate();
		}
	}

	// 로컬 플레이어: Pawn 충돌 또는 강한 임펄스 시 보정 트리거
	if (OtherActor && OtherActor != GetOwner())
	{
		if (OtherActor->IsA<APawn>() || NormalImpulse.SizeSquared() > 1000000.f)
		{
			if (APawn* P = Cast<APawn>(GetOwner()))
			{
				if (P->IsLocallyControlled())
				{
					P->ForceNetUpdate();
				}
			}
		}
	}
}