#include "Vehicle/Cart/BFCartPawn.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Character/Common/BFTeamComponent.h"
#include "Engine/Engine.h"

ABFCartPawn::ABFCartPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// ----- Replication -----
	bReplicates = true;
	NetUpdateFrequency = 60.f;
	MinNetUpdateFrequency = 30.f;

	Root = CreateDefaultSubobject<UBoxComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetCollisionProfileName(TEXT("Pawn"));
	Root->SetBoxExtent(FVector(48.0f, 30.0f, 50.0f));

	// 물리 시뮬을 쓰는 루트라면 컴포넌트도 복제 권장
	Root->SetIsReplicated(true);
	
	Root->bReplicatePhysicsToAutonomousProxy = true;
	
	TeamComp = CreateDefaultSubobject<UBFTeamComponent>(TEXT("TeamComp"));

	CartBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartBody"));
	CartBody->SetupAttachment(Root);

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(Root);

	WheelFRComp = CreateDefaultSubobject<USceneComponent>(TEXT("WheelFRComp"));
	WheelFRComp->SetupAttachment(Pivot);

	WheelFLComp = CreateDefaultSubobject<USceneComponent>(TEXT("WheelFLComp"));
	WheelFLComp->SetupAttachment(Pivot);

	WheelBRComp = CreateDefaultSubobject<USceneComponent>(TEXT("WheelBRComp"));
	WheelBRComp->SetupAttachment(Pivot);

	WheelBLComp = CreateDefaultSubobject<USceneComponent>(TEXT("WheelBLComp"));
	WheelBLComp->SetupAttachment(Pivot);

	WheelFRMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelFRMesh"));
	WheelFRMesh->SetupAttachment(WheelFRComp);

	WheelFLMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelFLMesh"));
	WheelFLMesh->SetupAttachment(WheelFLComp);

	WheelBRMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelBRMesh"));
	WheelBRMesh->SetupAttachment(WheelBRComp);

	WheelBLMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelBLMesh"));
	WheelBLMesh->SetupAttachment(WheelBLComp);

	CasterForkFRMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasterForkFRMesh"));
	CasterForkFRMesh->SetupAttachment(Pivot);
	CasterForkFRMesh->SetRelativeLocation(FVector(43.977692, 13.037226, 18.197203));

	CasterForkFLMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasterForkFLMesh"));
	CasterForkFLMesh->SetupAttachment(Pivot);
	CasterForkFLMesh->SetRelativeLocation(FVector(43.977692, -13.037030, 18.197218));

	CasterForkBRMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasterForkBRMesh"));
	CasterForkBRMesh->SetupAttachment(Pivot);
	CasterForkBRMesh->SetRelativeLocation(FVector(-31.720028, 26.393049, 18.197172));

	CasterForkBLMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasterForkBLMesh"));
	CasterForkBLMesh->SetupAttachment(Pivot);
	CasterForkBLMesh->SetRelativeLocation(FVector(-31.720029, -26.392992, 18.197203));

	PusherStandAnker = CreateDefaultSubobject<USceneComponent>(TEXT("PusherStandAnker"));
	PusherStandAnker->SetupAttachment(Pivot);

	HandleL = CreateDefaultSubobject<USceneComponent>(TEXT("HandleL"));
	HandleL->SetupAttachment(CartBody);

	HandleR = CreateDefaultSubobject<USceneComponent>(TEXT("HandleR"));
	HandleR->SetupAttachment(CartBody);
}

void ABFCartPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABFCartPawn, Rep_AccelAxis);
	DOREPLIFETIME(ABFCartPawn, Rep_SteerAxis);

	DOREPLIFETIME(ABFCartPawn, Rep_AccelerationInput);
	DOREPLIFETIME(ABFCartPawn, Rep_Acceleration);
	DOREPLIFETIME(ABFCartPawn, Rep_DriftSteer);
	DOREPLIFETIME(ABFCartPawn, Rep_DriftRotation);
	DOREPLIFETIME(ABFCartPawn, Rep_SteeringMultiplier);
}

void ABFCartPawn::SetCosmeticAccelInput(float Axis)
{
	Cosmetic_AccelInput = FMath::Clamp(Axis, -1.f, 1.f);
}

void ABFCartPawn::RequestUpright()
{
	if (!CanRequestReset())
		return;

	// 클라면 서버에 요청
	if (!HasAuthority())
	{
		Server_RequestUpright();
		return;
	}

	// 서버(또는 리슨서버 로컬)면 즉시 수행
	DoUprightReset_ServerAuth();
}

bool ABFCartPawn::IsFlipped() const
{
	// const FVector WorldUp = FVector::UpVector;
	// const FVector CartUp = GetActorUpVector();
	//
	// const float Dot = FVector::DotProduct(CartUp, WorldUp);
	// return Dot < FlipDotThreshold;
	
	const float Dot = FVector::DotProduct(GetActorUpVector(), FVector::UpVector);
	return Dot < UprightDotThreshold;
}

void ABFCartPawn::DoUprightReset_ServerAuth()
{
	if (!HasAuthority())
		return;

	if (!Root)
		return;

	// 연타 방지
	const double Now = GetWorld()->GetTimeSeconds();
	if (LastResetTimeSeconds > 0 && (Now - LastResetTimeSeconds) < ResetCooldown)
		return;

	// 물리 시뮬 중이 아니면 정책에 따라 처리(여기선 그냥 회전/위치만 보정 가능)
	const bool bSim = Root->IsSimulatingPhysics();

	// 속도가 너무 빠르면 리셋 금지(원하는 정책대로)
	const FVector LinVel = bSim ? Root->GetPhysicsLinearVelocity() : FVector::ZeroVector;
	if (LinVel.Size() > MaxSpeedToAllowReset)
		return;

	// 뒤집힘이 아니면 굳이 안 함(원하면 "옆으로 누움"도 포함하도록 threshold 조정)
	if (!IsFlipped())
		return;

	// ---------- 1) 바닥 찾기(LineTrace) ----------
	UWorld* World = GetWorld();
	if (!World)
		return;

	const FVector Start = GetActorLocation() + FVector::UpVector * TraceUpDistance;
	const FVector End   = GetActorLocation() - FVector::UpVector * TraceDownDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CartResetTrace), false, this);
	Params.bReturnPhysicalMaterial = false;

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Visibility, Params
	);

	// 디버그 원하면:
	// DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, 2.f);

	const FVector GroundNormal = bHit ? Hit.ImpactNormal.GetSafeNormal() : FVector::UpVector;

	// ---------- 2) 목표 회전 계산 ----------
	// "현재 전방(Forward)"을 바닥 평면에 투영해 Yaw 느낌 유지
	FVector Forward = GetActorForwardVector();
	Forward = FVector::VectorPlaneProject(Forward, GroundNormal).GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		// 극단적 케이스: Up/Forward가 꼬이면 임의로 X축 사용
		Forward = FVector::VectorPlaneProject(FVector::ForwardVector, GroundNormal).GetSafeNormal();
	}

	const FVector Up = bAlignToGroundNormal ? GroundNormal : FVector::UpVector;

	// Forward, Up으로 안정적인 회전 구성(우측 = Up x Forward)
	const FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal();
	const FVector OrthoForward = FVector::CrossProduct(Right, Up).GetSafeNormal();

	const FRotator TargetRot =
	FRotationMatrix::MakeFromXZ(OrthoForward, Up).Rotator();

	// ---------- 3) 목표 위치 계산 ----------
	// 바운딩 박스 기반으로 바닥에 박히지 않게 Lift
	const float Lift = Root->Bounds.BoxExtent.Z + ExtraLift;

	FVector TargetLoc = GetActorLocation();
	if (bHit)
	{
		TargetLoc = Hit.ImpactPoint + Up * Lift;
	}
	else
	{
		// 바닥 못 찾으면 현재 위치에서 살짝 들어올림
		TargetLoc = GetActorLocation() + Up * Lift;
	}

	// ---------- 4) 물리 속도 초기화 + Transform 적용 ----------
	// 네트워크/물리에서 “강제 순간이동”은 텔레포트 플래그가 중요함.
	// - 컴포넌트 이동/회전 시 TeleportPhysics 사용.
	if (bSim)
	{
		// 먼저 속도/각속도 제거 (각속도 단위: rad/s)
		Root->SetPhysicsLinearVelocity(FVector::ZeroVector, false);
		Root->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector, false);

		// 물리 바디를 텔레포트로 이동/회전
		Root->SetWorldLocationAndRotation(
			TargetLoc,
			TargetRot,
			false,
			nullptr,
			ETeleportType::TeleportPhysics
		);

		Root->WakeAllRigidBodies();
	}
	else
	{
		// 물리 시뮬이 아니면 Actor transform만 수정
		SetActorLocationAndRotation(TargetLoc, TargetRot, false, nullptr, ETeleportType::TeleportPhysics);
	}

	LastResetTimeSeconds = Now;
}

void ABFCartPawn::Server_RequestUpright_Implementation()
{
	// 서버에서: 실제로 이 Pawn의 소유자가 요청했는지 한 번 더 확인(권장)
	// - 컨트롤러 소유/플레이어 컨트롤 여부 등 프로젝트 규칙에 맞춰 강화 가능

	DoUprightReset_ServerAuth();
}

bool ABFCartPawn::CanRequestReset() const
{
	return IsLocallyControlled() || HasAuthority();
}

USceneComponent* ABFCartPawn::GetPusherStandAnkerComponent() const
{
	return PusherStandAnker;
}

FTransform ABFCartPawn::GetHandleLTransform() const
{
	return HandleL->GetComponentTransform();
}

FTransform ABFCartPawn::GetHandleRTransform() const
{
	return HandleR->GetComponentTransform();
}

void ABFCartPawn::BeginPlay()
{
	Super::BeginPlay();
	
	// 시작할 때 초기 위치 저장
	LastTickLocation = GetActorLocation();

	if (HasAuthority())
	{
		SetReplicateMovement(true);
	}
}

void ABFCartPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 원래대로 서버에서만 물리 틱을 돌립니다.
	if (HasAuthority())
	{
		ServerSimTick(DeltaSeconds);
	}

	Acceleration = Rep_Acceleration;

	FVector DeltaLoc = GetActorLocation() - LastTickLocation;
	LastTickLocation = GetActorLocation();

	if (DeltaLoc.SizeSquared() > 0.001f)
	{
		// 1. 서버 보정 패킷이 도착해서 실제로 이동한 프레임
		// 프레임 드랍으로 인한 속도 폭증(스파이크)을 막기 위해 DeltaSeconds 하한선 설정
		FVector InstantVelocity = DeltaLoc / FMath::Max(DeltaSeconds, 0.016f);
		CurrentVelocity = FMath::VInterpTo(CurrentVelocity, InstantVelocity, DeltaSeconds, 10.0f);
	}
	else
	{
		// 2. 패킷이 오지 않아 위치가 그대로인 프레임 (네트워크 딜레이)
		// 플레이어가 가속 중(Rep_AccelAxis)이거나 이미 서버 가속도(Rep_Acceleration)가 붙은 상태라면
		if (FMath::Abs(Rep_AccelAxis) > 0.01f || FMath::Abs(Rep_Acceleration) > 10.0f)
		{
			// 애니메이션 걷기 조건(GroundSpeed > 3.0)이 풀리지 않도록 최소 코스메틱 속도(5.1) 강제 유지
			if (CurrentVelocity.SizeSquared() < 25.0f)
			{
				CurrentVelocity = GetActorForwardVector() * 5.1f;
			}
		}
		else
		{
			// 정말로 멈춰야 하는 상황이면 부드럽게 감속
			CurrentVelocity = FMath::VInterpTo(CurrentVelocity, FVector::ZeroVector, DeltaSeconds, 15.0f);
		}
	}

	RotateMeshes(DeltaSeconds);
}

void ABFCartPawn::ServerSimTick(float DeltaSeconds)
{
	SuspensionCast(WheelFRComp);
	SuspensionCast(WheelFLComp);
	SuspensionCast(WheelBRComp);
	SuspensionCast(WheelBLComp);

	const float TargetAccel = IsOnGround() ? Rep_AccelAxis : 0.0f;
	Rep_AccelerationInput = FMath::FInterpTo(Rep_AccelerationInput, TargetAccel, DeltaSeconds, 0.5f);

	Rep_DriftSteer = FMath::Clamp(Rep_SteerAxis, -3.0f, 3.0f);

	AccelerateCart();
	CalculateAcceleration(DeltaSeconds);

	const double TorqueZ = Rep_DriftSteer * SteeringTorque * Rep_AccelerationInput * Rep_SteeringMultiplier;
	Root->AddTorqueInRadians(FVector(0.f, 0.f, TorqueZ));
}

float ABFCartPawn::GetAcceleration() const
{
	return Acceleration;
}

FVector ABFCartPawn::GetCurrentVelocity() const
{
	return CurrentVelocity;
}

void ABFCartPawn::SetAccelAxis_Server(float Axis)
{
	if (!HasAuthority()) return;
	Rep_AccelAxis = FMath::Clamp(Axis, -1.f, 1.f);
}

void ABFCartPawn::SetAccelAxis_Local(float Axis)
{
	Rep_AccelAxis = FMath::Clamp(Axis, -1.f, 1.f);
}

void ABFCartPawn::SetSteerAxis_Local(float Axis)
{
	Rep_DriftRotation.Yaw = FMath::Sign(Axis) * 25.0f;
	Rep_SteerAxis = Rep_SteeringMultiplier == 2.0f 
	? Axis
	: FMath::Max(Rep_DriftSteer, FMath::Abs(Axis)) * FMath::Sign(Rep_DriftRotation.Yaw);
}

void ABFCartPawn::SetDriving_Local(bool bDriving)
{
	bIsLocallyDriven = bDriving;
}

void ABFCartPawn::SetSteerAxis_Server(float Axis)
{
	if (!HasAuthority()) return;
	
	// TODO: 매직넘버 수정(25도 회전을 의도함)
	Rep_DriftRotation.Yaw = FMath::Sign(Axis) * 25.0f;
	
	Rep_SteerAxis = Rep_SteeringMultiplier == 2.0f 
	? Axis
	: FMath::Max(Rep_DriftSteer, FMath::Abs(Axis)) * FMath::Sign(Rep_DriftRotation.Yaw);
}

void ABFCartPawn::SetSteeringMultiplier_Server(const float Multiplier)
{
	if (!HasAuthority()) return;
	Rep_SteeringMultiplier = Multiplier;
}

void ABFCartPawn::SuspensionCast(USceneComponent* WheelComp) const
{
	FHitResult HitResult;

	const FVector Start = WheelComp->GetComponentLocation();
	const FVector End = Start + WheelComp->GetUpVector() * -WheelRadius;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	// 디버그 드로우는 로컬에서만 (서버 전용으로 켜면 원격 클라는 못 봅니다)
	if (IsLocallyControlled())
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0.05f, 0, 1.0f);
	}

	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);
	if (!bHit)
	{
		return;
	}

	const float HitDistance = HitResult.Distance;
	const float Normalized = FMath::GetRangePct(0.0f, WheelRadius, HitDistance);

	const FVector UnitDirection = (HitResult.TraceStart - HitResult.TraceEnd).GetSafeNormal();
	const FVector Force = (1.0f - Normalized) * UnitDirection * SuspensionForceMultiplier;

	Root->AddForceAtLocation(Force, WheelComp->GetComponentLocation());
}

void ABFCartPawn::CalculateAcceleration(float DeltaSeconds)
{
	Rep_Acceleration = FMath::Lerp(0.0f, MaxAcceleration, Rep_AccelerationInput)
		* FMath::Sign(Rep_AccelerationInput)
		* Rep_AccelerationInput;

	// 원래 코드의 “자연 감쇠” 유지(서버 기준)
	Rep_AccelerationInput = FMath::FInterpTo(Rep_AccelerationInput, 0.0f, DeltaSeconds, 1.0f);
}

void ABFCartPawn::AccelerateCart() const
{
	const FVector CartForward = Root->GetForwardVector();
	const float CartMass = Root->GetMass();

	FVector CurrentDownForce = FVector::ZeroVector;
	if (!IsOnGround())
	{
		CurrentDownForce.Z = DownForce;
	}

	const FVector Speed =
		CartForward * CartMass * Rep_AccelerationInput * CartSpeed * SpeedModifier
		+ CurrentDownForce;

	Root->AddForceAtLocation(Speed, Root->GetComponentLocation());
}

void ABFCartPawn::RotateMeshes(float DeltaSeconds)
{
	// 반드시 “복제된 값”만 사용 (클라/서버 동일)
	const FRotator WheelRotator = FRotator(Rep_Acceleration / (-1000.0f), 0.f, 0.f);

	WheelFRMesh->AddLocalRotation(WheelRotator);
	WheelFLMesh->AddLocalRotation(WheelRotator);
	WheelBLMesh->AddLocalRotation(WheelRotator);
	WheelBRMesh->AddLocalRotation(WheelRotator);

	// 드리프트 회전(현재 로직은 비어있어서 Rep_DriftRotation은 기본값일 것)
	const FRotator NewPivotRotation = FMath::RInterpTo(Pivot->GetRelativeRotation(), Rep_DriftRotation, DeltaSeconds,
	                                                   3.0f);
	Pivot->SetRelativeRotation(NewPivotRotation);

	const FRotator NewBodyRotation = FMath::RInterpTo(CartBody->GetRelativeRotation(), Rep_DriftRotation, DeltaSeconds,
	                                                  3.0f);
	CartBody->SetRelativeRotation(NewBodyRotation);
}

bool ABFCartPawn::IsOnGround() const
{
	FHitResult HitResult;

	const FVector Start = GetActorLocation();
	const FVector End = Start - GroundTraceEnd;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (IsLocallyControlled())
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Blue, false, 0.02f, 0, 1.0f);
	}

	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);
	return bHit;
}
