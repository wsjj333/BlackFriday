#include "Vehicle/Cart/BFCartPawn.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Character/CartDriver/BFCartDriverCharacter.h"
#include "Components/BoxComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

#include "Kismet/GameplayStatics.h"

ABFCartPawn::ABFCartPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<UBoxComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetCollisionProfileName(TEXT("Pawn"));
	Root->SetBoxExtent(FVector(48.0f, 30.0f, 45.0f));
	// Root->BodyInstance.bLockZRotation = true;

	CartBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartBody"));
	CartBody->SetupAttachment(Root);

	CartHandle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartHandle"));
	CartHandle->SetupAttachment(Root);

	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(Root);

	CasterForkFR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasterForkFR"));
	CasterForkFR->SetupAttachment(Pivot);

	CasterForkFL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasterForkFL"));
	CasterForkFL->SetupAttachment(Pivot);

	CasterForkBR = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasterForkBR"));
	CasterForkBR->SetupAttachment(Pivot);

	CasterForkBL = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasterForkBL"));
	CasterForkBL->SetupAttachment(Pivot);

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
}

void ABFCartPawn::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Subsystem->AddMappingContext(CartMappingContext, 0);
			}
		}
	}
	
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		AActor::StaticClass(),
		FoundActors
	);

	for (AActor* A : FoundActors)
	{
		// if (ABFCartDriverCharacter* Pusher = Cast<ABFCartDriverCharacter>(A))
		// {
		// 	CartDriver = Pusher;
		// 	break;
		// }
	}
}

void ABFCartPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EnhancedInput->BindAction(
		AccelerationAction,
		ETriggerEvent::Triggered,
		this,
		&ABFCartPawn::SetAccelerationInput
	);

	EnhancedInput->BindAction(
		SteeringAction,
		ETriggerEvent::Triggered,
		this,
		&ABFCartPawn::SteerCart
	);
}

void ABFCartPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// SuspensionCast(WheelFRComp);
	AccelerateCart();
	CalculateAcceleration();
	RotateMeshes();
	// SuspensionCast(WheelFLComp);
	// SuspensionCast(WheelBRComp);
	// SuspensionCast(WheelBLComp);
}

void ABFCartPawn::SuspensionCast(USceneComponent* WheelComp) const
{
	FHitResult HitResult;

	FVector Start = WheelComp->GetComponentLocation();
	FVector End = Start + WheelComp->GetUpVector() * -60.0f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	// DrawDebugLine(
	// 	GetWorld(),
	// 	Start,
	// 	End,
	// 	FColor::Red,
	// 	false,
	// 	5.0f,
	// 	0.1f,
	// 	1.0f
	// );

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	if (!bHit)
	{
		return;
	}

	// float HitResultDistance = HitResult.Distance;
	// float Normalized = FMath::GetRangePct(0.0f, 60.0f, HitResultDistance);
	//
	// FVector TraceStart = HitResult.TraceStart;
	// FVector TraceEnd = HitResult.TraceEnd;
	// FVector UnitDirection = (TraceEnd - TraceStart).GetSafeNormal();
	//
	// FVector Force = (1.0f - Normalized) * UnitDirection * SuspensionForceMultiplier;
	// FVector WheelCompLocation = WheelComp->GetComponentLocation();
	//
	// CartBody->AddForceAtLocation(Force, WheelCompLocation);
	//
	// UStaticMeshComponent* WheelMesh = Cast<UStaticMeshComponent>(WheelComp->GetChildComponent(0));
	// float RelativeLocationZ = WheelMesh->GetRelativeLocation().Z;
	// float TargetValue = HitResultDistance * -1.0f + 32.0f;
	//
	// float NewLocationZ = FMath::FInterpTo(
	// 	RelativeLocationZ,
	// 	TargetValue,
	// 	GetWorld()->GetDeltaSeconds(),
	// 	3.0f
	// );
	//
	// float RelativeLocationX = WheelMesh->GetRelativeLocation().X;
	// float RelativeLocationY = WheelMesh->GetRelativeLocation().Y;
	//
	// WheelMesh->SetRelativeLocation(FVector(RelativeLocationX, RelativeLocationY, NewLocationZ));
}

void ABFCartPawn::SetAccelerationInput(const FInputActionValue& Value)
{
	float TargetValue = 0.0f;

	if (IsOnGround())
	{
		TargetValue = Value.Get<float>();
	}

	AccelerationInput = FMath::FInterpTo(
		AccelerationInput,
		TargetValue,
		GetWorld()->GetDeltaSeconds(),
		0.5f
	);
}

void ABFCartPawn::SteerCart(const FInputActionValue& Value)
{
	const float ActionValue = Value.Get<float>();
	float TargetValue = ActionValue;
	
	if (bIsDrifting)
	{
		TargetValue = FMath::Max(DriftSteer, FMath::Abs(ActionValue)) * FMath::Sign(DriftRotation.Yaw);
	}
	
	DriftSteer = FMath::Clamp(TargetValue, -3.0f, 3.0f);
	
	const double TorqueZ = DriftSteer * SteeringTorque * AccelerationInput * SteeringMultiplier;
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Cyan,
			FString::Printf(
				TEXT("TorqueZ: %f"),
				TorqueZ
			)
		);
	}
	
	Root->AddTorqueInRadians(FVector(0.f, 0.f, TorqueZ));
}

void ABFCartPawn::RotateMeshes()
{
	FRotator WheelRotator = FRotator(Acceleration / (-1000.0f), 0.f, 0.f);

	WheelFRMesh->AddLocalRotation(WheelRotator);
	WheelFLMesh->AddLocalRotation(WheelRotator);
	WheelBLMesh->AddLocalRotation(WheelRotator);
	WheelBRMesh->AddLocalRotation(WheelRotator);

	FRotator NewPivotRotation = FMath::RInterpTo(
		Pivot->GetRelativeRotation(),
		DriftRotation,
		GetWorld()->GetDeltaSeconds(),
		3.0f
	);
	Pivot->SetRelativeRotation(NewPivotRotation);

	FRotator NewCartBodyRotation = FMath::RInterpTo(
		CartBody->GetRelativeRotation(),
		DriftRotation,
		GetWorld()->GetDeltaSeconds(),
		3.0f
	);
	CartBody->SetRelativeRotation(NewCartBodyRotation);
}

void ABFCartPawn::CalculateAcceleration()
{
	Acceleration = FMath::Lerp(0.0f, MaxAcceleration, AccelerationInput)
		* FMath::Sign(AccelerationInput)
		* AccelerationInput;

	AccelerationInput = FMath::FInterpTo(
		AccelerationInput,
		0.0f,
		GetWorld()->GetDeltaSeconds(),
		1.0f);
}

void ABFCartPawn::AccelerateCart() const
{
	const FVector CartForwardVector = Root->GetForwardVector();
	const float CartMass = Root->GetMass();

	FVector CurrentDownForce = FVector(0.0f, 0.0f, 0.0f);

	if (!IsOnGround())
	{
		CurrentDownForce.Z = DownForce;
	}

	const FVector Force = CartForwardVector * CartMass * AccelerationInput * CartSpeed * SpeedModifier +
		CurrentDownForce;

	// Root->AddForceAtLocation(Force, Root->GetComponentLocation());
	
	Root->AddForce(Force);

	// SetCartCenterOfMass();
}

void ABFCartPawn::SetCartCenterOfMass() const
{
	const FVector CenterOfMassOffset = FVector(
		CartCenterOfMess.X,
		CartCenterOfMess.Y,
		CartCenterOfMess.Z * AccelerationInput);

	Root->SetCenterOfMass(CenterOfMassOffset);
}

bool ABFCartPawn::IsOnGround() const
{
	FHitResult HitResult;

	FVector Start = GetActorLocation();
	FVector End = Start - GroundTraceEnd;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	DrawDebugLine(
		GetWorld(),
		Start,
		End,
		FColor::Blue,
		false,
		5.0f,
		0.1f,
		1.0f
	);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	if (!bHit)
	{
		return false;
	}

	return true;
}

void ABFCartPawn::StartDrift()
{
}

void ABFCartPawn::StopDrift()
{
}
