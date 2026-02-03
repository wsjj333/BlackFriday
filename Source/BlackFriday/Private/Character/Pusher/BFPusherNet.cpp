#include "Character/Pusher/BFPusherNet.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Component/BFPhysicsMovementComponent.h"
#include "Component/BFNetworkPhysicsComponent.h"
#include "Vehicle/Cart/BFCartMovementComponent.h"
#include "Vehicle/Cart/BFCartPawn.h"
#include "Character/Common/BFCharacterAnimInstance.h"
#include "Character/Pusher/Components/BFCharacterAppearanceComponent.h"
#include "Character/Pusher/Components/BFPusherDriveComponent.h"
#include "Character/Pusher/Components/BFPusherInputComponent.h"
#include "Net/UnrealNetwork.h"

ABFPusherNet::ABFPusherNet()
{
	bReplicates = true;

	// 캡슐 컴포넌트 (루트, 물리 시뮬레이션)
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(42.f, 90.f);
	CapsuleComp->SetCollisionProfileName(TEXT("PhysicsActor"));
	CapsuleComp->SetSimulatePhysics(true);
	CapsuleComp->SetEnableGravity(true);
	CapsuleComp->BodyInstance.bLockXRotation = true;
	CapsuleComp->BodyInstance.bLockYRotation = true;
	CapsuleComp->BodyInstance.SetMassOverride(80.f);
	SetRootComponent(CapsuleComp);

	// 스켈레탈 메시 컴포넌트
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(CapsuleComp);
	MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -110.f));
	MeshComp->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 물리 이동 컴포넌트
	PhysicsMoveComp = CreateDefaultSubobject<UBFPhysicsMovementComponent>(TEXT("PhysicsMoveComp"));

	// 네트워크 물리 동기화 컴포넌트
	NetPhysicsComp = CreateDefaultSubobject<UBFNetworkPhysicsComponent>(TEXT("NetPhysicsComp"));

	// 카트 조종 컴포넌트
	CartDrivingComp = CreateDefaultSubobject<UBFCartMovementComponent>(TEXT("CartDrivingComp"));
	
	PusherInputComp = CreateDefaultSubobject<UBFPusherInputComponent>(TEXT("PusherInputComp"));
	
	PusherDriveComp = CreateDefaultSubobject<UBFPusherDriveComponent>(TEXT("PusherDriveComp"));
	
	AppearanceComp = CreateDefaultSubobject<UBFCharacterAppearanceComponent>(TEXT("AppearanceComp"));
}

void ABFPusherNet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABFPusherNet, CharacterType);
	DOREPLIFETIME(ABFPusherNet, Cart);
	DOREPLIFETIME(ABFPusherNet, bIsDriving);
}

void ABFPusherNet::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		OnRep_CharacterType();
	}

	if (IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
					LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
				{
					Subsystem->AddMappingContext(PusherMappingContext, 0);
				}
			}
		}
	}

	RefreshAnimInstanceCache();
}

void ABFPusherNet::RefreshAnimInstanceCache()
{
	if (MeshComp)
	{
		CachedAnimInstance = Cast<UBFCharacterAnimInstance>(MeshComp->GetAnimInstance());
	}
}

void ABFPusherNet::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!CachedAnimInstance)
	{
		RefreshAnimInstanceCache();
	}

	if (!bIsDriving || !Cart || !CachedAnimInstance)
	{
		return;
	}

	// IK: 핸들 위치 계산
	const FTransform HandleL_WS = Cart->GetHandleLTransform();
	const FTransform HandleR_WS = Cart->GetHandleRTransform();

	const FTransform HandleL_CS = HandleL_WS.GetRelativeTransform(MeshComp->GetComponentTransform());
	const FTransform HandleR_CS = HandleR_WS.GetRelativeTransform(MeshComp->GetComponentTransform());

	CachedAnimInstance->HandleTargetL_CS = HandleL_CS;
	CachedAnimInstance->HandleTargetR_CS = HandleR_CS;
}

void ABFPusherNet::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!IsLocallyControlled())
	{
		return;
	}

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABFPusherNet::HandleMoveInput);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &ABFPusherNet::HandleMoveInput);

	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABFPusherNet::HandleLookInput);

	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ABFPusherNet::OnJumpPressed);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ABFPusherNet::OnJumpReleased);

	EnhancedInput->BindAction(DriveModeAction, ETriggerEvent::Started, this, &ABFPusherNet::OnToggleDriveModePressed);

	EnhancedInput->BindAction(AccelerationAction, ETriggerEvent::Started, CartDrivingComp.Get(), &UBFCartMovementComponent::Input_AccelTriggered);
	EnhancedInput->BindAction(AccelerationAction, ETriggerEvent::Completed, CartDrivingComp.Get(), &UBFCartMovementComponent::Input_AccelEnded);
	EnhancedInput->BindAction(AccelerationAction, ETriggerEvent::Canceled, CartDrivingComp.Get(), &UBFCartMovementComponent::Input_AccelEnded);

	EnhancedInput->BindAction(SteerAction, ETriggerEvent::Triggered, CartDrivingComp.Get(), &UBFCartMovementComponent::Input_SteerTriggered);
	EnhancedInput->BindAction(SteerAction, ETriggerEvent::Completed, CartDrivingComp.Get(), &UBFCartMovementComponent::Input_SteerEnded);
}

void ABFPusherNet::HandleMoveInput(const FInputActionValue& Value)
{
	if (bIsDriving)
	{
		return;
	}

	const FVector2D MoveAxis = Value.Get<FVector2D>();

	if (NetPhysicsComp)
	{
		// Enhanced Input: X=좌우(A/D), Y=앞뒤(W/S)
		// BFPhysicsMovementComponent: X=앞뒤(Forward), Y=좌우(Right)
		NetPhysicsComp->SetMoveInput(FVector2D(MoveAxis.Y, MoveAxis.X));
	}
}

void ABFPusherNet::HandleLookInput(const FInputActionValue& Value)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	const FVector2D LookAxis = Value.Get<FVector2D>();
	const float LookX = LookAxis.X;
	const float LookY = LookAxis.Y;

	if (bIsDriving)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (!PC || !PC->IsLocalController())
		{
			return;
		}

		FRotator ControlRot = PC->GetControlRotation();
		ControlRot.Yaw += LookX;
		ControlRot.Pitch += LookY;

		PC->SetControlRotation(ControlRot);
		HardClampControlRotation();
	}
	else
	{
		AddControllerYawInput(LookX);
		AddControllerPitchInput(LookY);

		// 네트워크 컴포넌트에 Yaw 전달
		if (NetPhysicsComp)
		{
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				NetPhysicsComp->SetControlYawDegrees(PC->GetControlRotation().Yaw);
			}
		}
	}
}

void ABFPusherNet::OnJumpPressed(const FInputActionValue& Value)
{
	if (NetPhysicsComp)
	{
		NetPhysicsComp->SetJumpHeld(true);
	}
}

void ABFPusherNet::OnJumpReleased(const FInputActionValue& Value)
{
	if (NetPhysicsComp)
	{
		NetPhysicsComp->SetJumpHeld(false);
	}
}

void ABFPusherNet::OnToggleDriveModePressed(const FInputActionValue& Value)
{
	if (!Cart) return;

	ServerToggleDrivingMode();
}

void ABFPusherNet::ToggleDrivingMode()
{
	ServerToggleDrivingMode();
}

void ABFPusherNet::ServerToggleDrivingMode_Implementation()
{
	bIsDriving = !bIsDriving;

	ApplyDrivingAttachment_Server(bIsDriving);

	OnRep_IsDriving();
}

void ABFPusherNet::ApplyDrivingAttachment_Server(bool bAttach)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bAttach)
	{
		if (!Cart) return;

		USceneComponent* StandAnker = Cart->GetPusherStandAnkerComponent();
		if (!StandAnker) return;

		// 물리 시뮬레이션 비활성화
		SetPhysicsEnabled(false);

		const FAttachmentTransformRules Rules(
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::KeepWorld,
			true);

		AttachToComponent(StandAnker, Rules);

		// 캡슐 중심 기준이므로 HalfHeight만큼 위로 오프셋
		if (CapsuleComp)
		{
			const float HalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();
			SetActorRelativeLocation(FVector(0.f, 0.f, HalfHeight));
		}
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		// 물리 시뮬레이션 재활성화
		SetPhysicsEnabled(true);
	}
}

void ABFPusherNet::SetPhysicsEnabled(bool bEnabled)
{
	if (CapsuleComp)
	{
		CapsuleComp->SetSimulatePhysics(bEnabled);
		if (bEnabled)
		{
			CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			if (CapsuleComp->GetBodyInstance())
			{
				CapsuleComp->GetBodyInstance()->WakeInstance();
			}
		}
		else
		{
			CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}

	// NetPhysicsComp가 매 틱마다 물리를 다시 활성화하는 것을 방지
	if (NetPhysicsComp)
	{
		NetPhysicsComp->SetComponentTickEnabled(bEnabled);
	}

	if (PhysicsMoveComp)
	{
		PhysicsMoveComp->SetComponentTickEnabled(bEnabled);
	}
}

void ABFPusherNet::SetCurrentSkeletalMeshAsset(EBFCharacterType NewCharacterType)
{
	ServerSetCharacterType(NewCharacterType);
}

void ABFPusherNet::ServerSetCharacterType_Implementation(EBFCharacterType NewType)
{
	const bool bIsValid =
		StaticEnum<EBFCharacterType>()->IsValidEnumValue(static_cast<int64>(NewType)) &&
		NewType != EBFCharacterType::None;

	if (!bIsValid) return;

	CharacterType = NewType;
	OnRep_CharacterType();
}

void ABFPusherNet::OnRep_CharacterType()
{
	if (CharacterType == EBFCharacterType::None) return;

	if (TSoftObjectPtr<USkeletalMesh>* Found = CharacterMeshMap.Find(CharacterType))
	{
		if (USkeletalMesh* MeshAsset = Found->LoadSynchronous())
		{
			if (MeshComp)
			{
				MeshComp->SetSkeletalMeshAsset(MeshAsset);
			}
		}
	}

	RefreshAnimInstanceCache();
}

void ABFPusherNet::SetCart(ABFCartPawn* NewCart)
{
	if (HasAuthority())
	{
		Cart = NewCart;
		OnRep_Cart();
		return;
	}

	if (!IsLocallyControlled())
	{
		return;
	}

	ServerSetCart(NewCart);
}

void ABFPusherNet::ServerSetCart_Implementation(ABFCartPawn* NewCart)
{
	Cart = NewCart;
	OnRep_Cart();
}

void ABFPusherNet::OnRep_Cart()
{
	if (CartDrivingComp)
	{
		CartDrivingComp->SetCart(Cart);
	}

	RefreshAnimInstanceCache();
}

void ABFPusherNet::OnRep_IsDriving()
{
	if (CartDrivingComp)
	{
		CartDrivingComp->SetDriving(bIsDriving);
	}

	// 클라이언트에서도 물리 상태 동기화
	if (!HasAuthority())
	{
		SetPhysicsEnabled(!bIsDriving);
	}
}

void ABFPusherNet::HardClampControlRotation()
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

	if (!Original.Equals(Clamped, 0.01f))
	{
		PC->SetControlRotation(Clamped);
	}
}
