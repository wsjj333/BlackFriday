#include "Component/BFPhysicsMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Component/BFNetworkPhysicsComponent.h"
#include "Components/PrimitiveComponent.h"

UBFPhysicsMovementComponent::UBFPhysicsMovementComponent()
{
	// PrePhysics에서 Tick → 물리 힘 적용 타이밍 보장
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

	// 실제 물리 이동에 사용할 PrimitiveComponent 결정
	CachePrimitive();

	if (AActor* Owner = GetOwner())
	{
		// 애니메이션 / PhysicalAnimation용 메쉬 캐싱
		CachedMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (Prim)
	{
		// PawnMovementComponent 내부 로직용
		SetUpdatedComponent(Prim);

		// 충돌 발생 시 네트워크 보정 트리거
		Prim->OnComponentHit.AddDynamic(
			this,
			&UBFPhysicsMovementComponent::OnComponentHit
		);
	}

	// NetworkPhysicsComponent 이후에 Tick되도록 순서 보장
	if (UBFNetworkPhysicsComponent* NetComp =
		GetOwner()->FindComponentByClass<UBFNetworkPhysicsComponent>())
	{
		AddTickPrerequisiteComponent(NetComp);
	}

	// 상체 물리 애니메이션 (현재는 비활성)
	// SetupUpperBodyPhysics();
}

void UBFPhysicsMovementComponent::CachePrimitive()
{
	// 명시적 오버라이드 우선
	if (PhysicsPrimitiveOverride)
	{
		Prim = PhysicsPrimitiveOverride;
	}
	// 기본은 RootComponent
	else if (AActor* Owner = GetOwner())
	{
		Prim = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
	}
}

void UBFPhysicsMovementComponent::ApplyInputImmediately(const FBFMoveInputNet& Input)
{
	// 물리 시뮬레이션 중이 아닐 경우 무시
	if (!Prim || !Prim->IsSimulatingPhysics()) return;

	const bool bJump = (Input.Buttons & 0x01) != 0;

	// 즉시 점프 (서버/로컬 공통)
	if (bJump && JumpCooldownTime <= 0.f && bGrounded)
	{
		// 수평 속도 유지 + 수직만 초기화
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

	if (!Prim) return;
	if (!UpdatedComponent) SetUpdatedComponent(Prim);

	const bool bSimulating = Prim->IsSimulatingPhysics();
	const FVector CurrentPhysVel =
		bSimulating ? Prim->GetPhysicsLinearVelocity() : FVector::ZeroVector;

	const FVector Loc = Prim->GetComponentLocation();

	// ================= 지면 판정 =================
	{
		const FVector TraceStart = Loc;
		const FVector TraceEnd =
			TraceStart - FVector::UpVector * (GroundTraceLength * 1.2f);

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(GetOwner());

		const bool bHit =
			GetWorld()->LineTraceSingleByChannel(
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
	if (JumpBufferTime > 0.f) JumpBufferTime -= DeltaTime;
	if (JumpCooldownTime > 0.f) JumpCooldownTime -= DeltaTime;
	
	const bool bHasInput = !FMath::IsNearlyZero(MoveX) || !FMath::IsNearlyZero(MoveY); 

	// 지면/공중에 따른 감쇠 조절
	if (bGrounded)
		Prim->SetLinearDamping(
			bHasInput ? MovingLinearDamping : BrakingLinearDamping
		);
	else
		Prim->SetLinearDamping(0.1f);

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
				Prim->AddImpulse(
					FVector(0.f, 0.f, JumpImpulse),
					NAME_None,
					true
				);

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
	const FVector Velo = Prim->GetPhysicsLinearVelocity();
	// UE_LOG(LogTemp, Warning,
	// 	TEXT("Input=%s Move=(%.2f, %.2f) | Grounded=%d | Simulating=%d | Speed2D=%.2f"),
	// 	bHasInput ? TEXT("true") : TEXT("false"),
	// 	MoveX, MoveY,
	// 	bGrounded,
	// 	bSimulating,
	// 	Velo.Size2D()
	// );
	
	if (bHasInput && bSimulating)
	{
		// 로컬 공간(컨트롤 기준)의 입력 벡터를 만든다(Z는 이동 입력에선 안 쓰니 0)
		FVector LocalDir(MoveX, MoveY, 0.f);
		// 대각 입력 시 속도가 증가할 수 있으므로 정규화
		if (LocalDir.SizeSquared() > 1.f)
			LocalDir.Normalize();

		// 컨트롤 Yaw(시야 방향)를 기준으로 월드 이동 방향 계산
		const FRotator YawRot(0.f, InputYawDeg, 0.f);
		FVector WorldDir = YawRot.RotateVector(LocalDir);

		if (bGrounded)
		{
			// 경사면 투영
			WorldDir =
				FVector::VectorPlaneProject(WorldDir, GroundNormal)
				.GetSafeNormal();
		}
		else
		{
			WorldDir.Z = 0.f;
			WorldDir.Normalize();
			WorldDir *= AirControl;
		}

		// 속도에 따른 가속 감소
		const float CurrentSpeed2D =
			Prim->GetPhysicsLinearVelocity().Size2D();

		const float SpeedRatio =
			FMath::Clamp(CurrentSpeed2D / MaxSpeed, 0.f, 1.f);

		const float DynamicMultiplier =
			FMath::Lerp(AccelMultiplier, 1.0f, SpeedRatio);

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
		Vel = Prim->GetPhysicsLinearVelocity();
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
		const FRotator TargetRot(0.f, InputYawDeg, 0.f);
		const FRotator CurrentRot = Prim->GetComponentRotation();
		const FRotator NewRot =
			FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 15.0f);

		Prim->SetWorldRotation(
			NewRot,
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);
	}

	// ================= 애니메이션용 속도 =================
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		FVector TargetVel = FVector::ZeroVector;
		if (UBFNetworkPhysicsComponent* NetComp =
			GetOwner()->FindComponentByClass<UBFNetworkPhysicsComponent>())
		{
			TargetVel = NetComp->GetReplicatedVelocity();
		}

		SmoothAnimVelocity =
			FMath::VInterpTo(
				SmoothAnimVelocity,
				TargetVel,
				DeltaTime,
				15.0f
			);
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

	// ================= 가속도 =================
	APawn* Pawn = GetPawnOwner();
	if (!Pawn)
	{
		CurrentAcceleration = FVector::ZeroVector;
		return;
	}

	UBFNetworkPhysicsComponent* NetComp =
		Pawn->FindComponentByClass<UBFNetworkPhysicsComponent>();

	if (!NetComp)
	{
		CurrentAcceleration = FVector::ZeroVector;
		return;
	}

	/*
	 * === Owner (AutonomousProxy) ===
	 * CMC와 동일:
	 * 입력 벡터 → 정규화 → MaxAcceleration
	 */
	if (Pawn->IsLocallyControlled())
	{
		// NetComp가 제공하는 월드 이동 입력
		const FVector InputWS = NetComp->GetMoveInputWorldSpace();

		FVector AccelDir = InputWS.GetClampedToMaxSize(1.f);
		CurrentAcceleration = AccelDir * MaxAcceleration;
		return;
	}

	/*
	 * === SimulatedProxy ===
	 * CMC의 UpdateProxyAcceleration과 동일한 개념
	 * (애니메이션 힌트용)
	 */
	const FVector Vel = UpdatedComponent
		                    ? UpdatedComponent->GetComponentVelocity()
		                    : FVector::ZeroVector;

	if (Vel.SizeSquared() > SMALL_NUMBER)
	{
		CurrentAcceleration = Vel.GetSafeNormal() * MaxAcceleration;
	}
	else
	{
		CurrentAcceleration = FVector::ZeroVector;
	}
}

void UBFPhysicsMovementComponent::SetCurrentInput(
	const FBFMoveInputNet& InInput
)
{
	// 정규화된 입력값 복원
	// TODO: InInput의 MoveY와 MoveX 값이 뒤바뀌어서 들어오는데 원인 파악 중이라 임시로 두 값을 바꿔서 복원함 
	MoveX = InInput.MoveY / 32767.f;
	MoveY = InInput.MoveX / 32767.f;
	
	InputYawDeg = InInput.ControlYaw100 / 100.f;

	// 점프 입력 에지 감지
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
	// AutonomousProxy는 입력 기반 추정 속도 반환
	if (APawn* P = Cast<APawn>(GetOwner()))
	{
		if (P->GetLocalRole() == ROLE_AutonomousProxy)
		{
			if (FMath::Abs(MoveX) > 0.01f ||
				FMath::Abs(MoveY) > 0.01f)
			{
				return P->GetActorForwardVector() * MaxSpeed;
			}
			return FVector::ZeroVector;
		}
	}

	// SimulatedProxy는 보간된 속도 사용
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

	// 로컬 플레이어: Pawn 충돌 또는 강한 임펄스 시 보정
	if (OtherActor && OtherActor != GetOwner())
	{
		if (OtherActor->IsA<APawn>() ||
			NormalImpulse.SizeSquared() > 1000000.f)
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
