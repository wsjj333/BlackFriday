#include "Vehicle/Cart/BFCartPawn.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
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

	// 소유 클라에서 물리 복제 스무딩이 필요하면(UE 버전에 따라 효과 차이 있음)
	// Root->bReplicatePhysicsToAutonomousProxy = true; // UPrimitiveComponent 멤버(버전에 따라 접근 가능)

	CartBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartBody"));
	CartBody->SetupAttachment(Root);

	CartHandle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartHandle"));
	CartHandle->SetupAttachment(Root);

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
	CasterForkFRMesh->SetRelativeLocation(FVector(43.977692,13.037226,18.197203));
	
	CasterForkFLMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasterForkFLMesh"));
	CasterForkFLMesh->SetupAttachment(Pivot);
	CasterForkFLMesh->SetRelativeLocation(FVector(43.977692,-13.037030,18.197218));
	
	CasterForkBRMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasterForkBRMesh"));
	CasterForkBRMesh->SetupAttachment(Pivot);
	CasterForkBRMesh->SetRelativeLocation(FVector(-31.720028,26.393049,18.197172));
	
	CasterForkBLMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasterForkBLMesh"));
	CasterForkBLMesh->SetupAttachment(Pivot);
	CasterForkBLMesh->SetRelativeLocation(FVector(-31.720029,-26.392992,18.197203));
	
	PusherStandAnker = CreateDefaultSubobject<USceneComponent>(TEXT("PusherStandAnker"));
	PusherStandAnker->SetupAttachment(Pivot);
	
	HandleL = CreateDefaultSubobject<USceneComponent>(TEXT("HandleL"));
	HandleL->SetupAttachment(CartHandle);
	
	HandleR = CreateDefaultSubobject<USceneComponent>(TEXT("HandleR"));
	HandleR->SetupAttachment(CartHandle);
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
	
	if (HasAuthority())
	{
		SetReplicateMovement(true);
	}

	// 입력 매핑은 “로컬 플레이어”에게만
	// if (IsLocallyControlled())
	// {
	// 	if (APlayerController* PC = Cast<APlayerController>(Controller))
	// 	{
	// 		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
	// 		{
	// 			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
	// 				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	// 			{
	// 				Subsystem->AddMappingContext(CartMappingContext, 0);
	// 			}
	// 		}
	// 	}
	// }
}

void ABFCartPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 소유 클라이언트만 바인딩
	if (!IsLocallyControlled())
	{
		return;
	}

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EnhancedInput->BindAction(AccelerationAction, ETriggerEvent::Triggered, this, &ABFCartPawn::SetAccelerationInput);
	EnhancedInput->BindAction(AccelerationAction, ETriggerEvent::Completed, this, &ABFCartPawn::OnAccelerationEnded);
	EnhancedInput->BindAction(AccelerationAction, ETriggerEvent::Canceled,  this, &ABFCartPawn::OnAccelerationEnded);
	
	EnhancedInput->BindAction(SteeringAction,     ETriggerEvent::Triggered, this, &ABFCartPawn::SteerCart);
	EnhancedInput->BindAction(SteeringAction,     ETriggerEvent::Completed, this, &ABFCartPawn::OnSteeringEnded);
	EnhancedInput->BindAction(SteeringAction,     ETriggerEvent::Canceled,  this, &ABFCartPawn::OnSteeringEnded);
	
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABFCartPawn::OnMouseLook);
}

void ABFCartPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (HasAuthority())
	{
		// “물리”는 서버에서만
		ServerSimTick(DeltaSeconds);
	}
	
	if (IsLocallyControlled())
	{
		// 카메라 회전은 로컬 전용
		HardClampControlRotation();
	}
	
	Acceleration = Rep_Acceleration;

	// “코스메틱”은 모든 곳에서 가능하나, 반드시 복제된 값 기반으로만
	RotateMeshes(DeltaSeconds);
}

void ABFCartPawn::ServerSimTick(float DeltaSeconds)
{
	// 서스펜션/가속/조향 등 물리는 서버 권한
	SuspensionCast(WheelFRComp);
	SuspensionCast(WheelFLComp);
	SuspensionCast(WheelBRComp);
	SuspensionCast(WheelBLComp);

	// 서버 입력축 -> 서버 스무딩 값 생성
	const float TargetAccel = IsOnGround() ? Rep_AccelAxis : 0.0f;
	Rep_AccelerationInput = FMath::FInterpTo(Rep_AccelerationInput, TargetAccel, DeltaSeconds, 0.5f);

	// 조향도 서버에서 “최종 조향값” 산출
	// 드리프트 로직이 비어있어서 현재는 단순 클램프만 유지
	Rep_DriftSteer = FMath::Clamp(Rep_SteerAxis, -3.0f, 3.0f);

	// 가속/힘 계산
	AccelerateCart();
	CalculateAcceleration(DeltaSeconds);

	// 토크 적용(서버만)
	const double TorqueZ = Rep_DriftSteer * SteeringTorque * Rep_AccelerationInput * SteeringMultiplier;
	Root->AddTorqueInRadians(FVector(0.f, 0.f, TorqueZ));

	// 디버그는 서버에서 찍으면 “서버 화면/로그”에만 의미가 있습니다.
	// 필요하면 IsLocallyControlled() 조건으로 클라 화면에만 출력하세요.
}

void ABFCartPawn::SetAccelerationInput(const FInputActionValue& Value)
{
	// if (!IsLocallyControlled())
	// {
	// 	return;
	// }

	const float Axis = Value.Get<float>();
	Server_SetAccelerationAxis(Axis);
}

void ABFCartPawn::SteerCart(const FInputActionValue& Value)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	const float Axis = Value.Get<float>();
	Server_SetSteeringAxis(Axis);
}

void ABFCartPawn::OnAccelerationEnded(const FInputActionValue& Value)
{
	if (!IsLocallyControlled()) return;
	Server_SetAccelerationAxis(0.f);
}

float ABFCartPawn::GetAcceleration() const
{
	return Acceleration;
}

void ABFCartPawn::OnSteeringEnded(const FInputActionValue& Value)
{
	if (!IsLocallyControlled()) return;
	Server_SetSteeringAxis(0.f);
}

void ABFCartPawn::OnMouseLook(const FInputActionValue& Value)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	const FVector2D LookAxis = Value.Get<FVector2D>();
	const float LookX = LookAxis.X;
	const float LookY = LookAxis.Y;

	FRotator ControlRot = PC->GetControlRotation();

	ControlRot.Yaw   += LookX;
	ControlRot.Pitch += LookY;

	PC->SetControlRotation(ControlRot);

	// 입력이 있을 때도 즉시 범위 보정
	HardClampControlRotation();
}

void ABFCartPawn::HardClampControlRotation()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController()) return;

	const FRotator Original = PC->GetControlRotation();
	FRotator Clamped = Original;

	Clamped.Pitch = FMath::Clamp(Clamped.Pitch, -90.f, 90.f);

	const float ActorYaw = GetActorRotation().Yaw;
	float OffsetYaw = FMath::FindDeltaAngleDegrees(ActorYaw, Clamped.Yaw);
	OffsetYaw = FMath::Clamp(OffsetYaw, -90.f, 90.f);
	Clamped.Yaw = ActorYaw + OffsetYaw;

	Clamped.Roll = 0.f;

	// 거의 동일하면 불필요한 Set 방지
	if (!Original.Equals(Clamped, 0.01f))
	{
		PC->SetControlRotation(Clamped);
	}
}

bool ABFCartPawn::Server_SetAccelerationAxis_Validate(float Axis) { return FMath::IsFinite(Axis) && FMath::Abs(Axis) <= 1.1f; }
void ABFCartPawn::Server_SetAccelerationAxis_Implementation(float Axis)
{
	Rep_AccelAxis = FMath::Clamp(Axis, -1.f, 1.f);
}

bool ABFCartPawn::Server_SetSteeringAxis_Validate(float Axis) { return FMath::IsFinite(Axis) && FMath::Abs(Axis) <= 1.1f; }
void ABFCartPawn::Server_SetSteeringAxis_Implementation(float Axis)
{
	Rep_SteerAxis = FMath::Clamp(Axis, -1.f, 1.f);
}

void ABFCartPawn::SuspensionCast(USceneComponent* WheelComp) const
{
	FHitResult HitResult;

	const FVector Start = WheelComp->GetComponentLocation();
	const FVector End   = Start + WheelComp->GetUpVector() * -WheelRadius;

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
	const float Normalized  = FMath::GetRangePct(0.0f, WheelRadius, HitDistance);

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

	const FVector Force =
		CartForward * CartMass * Rep_AccelerationInput * CartSpeed * SpeedModifier
		+ CurrentDownForce;

	Root->AddForceAtLocation(Force, Root->GetComponentLocation());
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
	const FRotator NewPivotRotation = FMath::RInterpTo(Pivot->GetRelativeRotation(), Rep_DriftRotation, DeltaSeconds, 3.0f);
	Pivot->SetRelativeRotation(NewPivotRotation);

	const FRotator NewBodyRotation  = FMath::RInterpTo(CartBody->GetRelativeRotation(), Rep_DriftRotation, DeltaSeconds, 3.0f);
	CartBody->SetRelativeRotation(NewBodyRotation);
}

bool ABFCartPawn::IsOnGround() const
{
	FHitResult HitResult;

	const FVector Start = GetActorLocation();
	const FVector End   = Start - GroundTraceEnd;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (IsLocallyControlled())
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Blue, false, 0.02f, 0, 1.0f);
	}

	const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);
	return bHit;
}
