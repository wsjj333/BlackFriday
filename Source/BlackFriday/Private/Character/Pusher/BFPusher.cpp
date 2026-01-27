#include "Character/Pusher/BFPusher.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Vehicle/Cart/BFCartPawn.h"
#include "Character/Common/BFCharacterAnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Vehicle/Cart/BFCartMovementComponent.h"

ABFPusher::ABFPusher()
{
	bReplicates = true;
	
	CartDrivingComp = CreateDefaultSubobject<UBFCartMovementComponent>(TEXT("CartDrivingComp"));

	GetCapsuleComponent()->SetCapsuleHalfHeight(110.0f);

	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -110.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f));
}

void ABFPusher::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABFPusher, CharacterType);
	DOREPLIFETIME(ABFPusher, Cart);
	DOREPLIFETIME(ABFPusher, bIsDriving);
}

void ABFPusher::BeginPlay()
{
	Super::BeginPlay();

	// 서버가 CharacterType을 소스로 갖고, 클라는 OnRep로 반영
	if (HasAuthority())
	{
		OnRep_CharacterType();
	}

	// 입력 매핑은 로컬만
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

void ABFPusher::RefreshAnimInstanceCache()
{
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		CachedAnimInstance = Cast<UBFCharacterAnimInstance>(MeshComp->GetAnimInstance());
	}
}

void ABFPusher::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!CachedAnimInstance)
	{
		RefreshAnimInstanceCache();
	}

	// IK는 “복제된 Cart의 Transform” 기반으로 각자 계산해도 OK
	if (!bIsDriving || !Cart || !CachedAnimInstance)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return;

	const FTransform HandleL_WS = Cart->GetHandleLTransform();
	const FTransform HandleR_WS = Cart->GetHandleRTransform();

	const FTransform HandleL_CS = HandleL_WS.GetRelativeTransform(MeshComp->GetComponentTransform());
	const FTransform HandleR_CS = HandleR_WS.GetRelativeTransform(MeshComp->GetComponentTransform());

	CachedAnimInstance->HandleTargetL_CS = HandleL_CS;
	CachedAnimInstance->HandleTargetR_CS = HandleR_CS;
}

void ABFPusher::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!IsLocallyControlled())
	{
		return;
	}

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABFPusher::HandleMoveInput);
	
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABFPusher::HandleLookInput);
	
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ABFPusher::OnJumpPressed);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ABFPusher::OnJumpReleased);
	
	EnhancedInput->BindAction(DriveModeAction, ETriggerEvent::Started, this, &ABFPusher::OnToggleDriveModePressed);

	EnhancedInput->BindAction(AccelerationAction, ETriggerEvent::Started, CartDrivingComp.Get(), &UBFCartMovementComponent::Input_AccelTriggered);
	EnhancedInput->BindAction(AccelerationAction, ETriggerEvent::Completed, CartDrivingComp.Get(), &UBFCartMovementComponent::Input_AccelEnded);
	EnhancedInput->BindAction(AccelerationAction, ETriggerEvent::Canceled, CartDrivingComp.Get(), &UBFCartMovementComponent::Input_AccelEnded);

	EnhancedInput->BindAction(SteerAction, ETriggerEvent::Triggered, CartDrivingComp.Get(), &UBFCartMovementComponent::Input_SteerTriggered);
	EnhancedInput->BindAction(SteerAction, ETriggerEvent::Completed, CartDrivingComp.Get(), &UBFCartMovementComponent::Input_SteerEnded);
}

void ABFPusher::HandleMoveInput(const FInputActionValue& Value)
{
	if (bIsDriving)
	{
		return;
	}
	
	const FVector2D MoveAxis = Value.Get<FVector2D>();

	const float MoveX = MoveAxis.X;
	const float MoveY = MoveAxis.Y;

	const FRotator ControlRot = GetControlRotation();
	FRotator RotForMove(0.0f, ControlRot.Yaw, 0.0f);

	FVector WorldDirection = UKismetMathLibrary::GetRightVector(RotForMove);
	AddMovementInput(WorldDirection, MoveX);

	WorldDirection = UKismetMathLibrary::GetForwardVector(RotForMove);
	AddMovementInput(WorldDirection, MoveY);
}

void ABFPusher::HandleLookInput(const FInputActionValue& Value)
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
	}
}

void ABFPusher::OnJumpPressed(const FInputActionValue& Value)
{
	Jump();
}

void ABFPusher::OnJumpReleased(const FInputActionValue& Value)
{
	StopJumping();
}

void ABFPusher::OnToggleDriveModePressed(const FInputActionValue& Value)
{
	if (!Cart) return;

	// 드라이빙 토글은 서버가 결정(복제)
	if (HasAuthority())
	{
		ServerToggleDrivingMode(); // 서버에서도 한 경로로
	}
	else
	{
		ServerToggleDrivingMode();
	}
}

void ABFPusher::ToggleDrivingMode()
{
	// 외부(BP)에서 호출해도 서버로 라우팅
	if (HasAuthority())
	{
		ServerToggleDrivingMode();
	}
	else
	{
		ServerToggleDrivingMode();
	}
}

void ABFPusher::ServerToggleDrivingMode_Implementation()
{
	bIsDriving = !bIsDriving;

	ApplyDrivingAttachment_Server(bIsDriving);

	// 서버 자신도 로컬 상태 반영
	OnRep_IsDriving();
}

void ABFPusher::ApplyDrivingAttachment_Server(bool bAttach)
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

		const FAttachmentTransformRules Rules(
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::KeepWorld,
			true);

		AttachToComponent(StandAnker, Rules);
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
}

void ABFPusher::SetCurrentSkeletalMeshAsset(EBFCharacterType NewCharacterType)
{
	if (HasAuthority())
	{
		ServerSetCharacterType(NewCharacterType);
	}
	else
	{
		ServerSetCharacterType(NewCharacterType);
	}
}

void ABFPusher::ServerSetCharacterType_Implementation(EBFCharacterType NewType)
{
	const bool bIsValid =
		StaticEnum<EBFCharacterType>()->IsValidEnumValue(static_cast<int64>(NewType)) &&
		NewType != EBFCharacterType::None;

	if (!bIsValid) return;

	CharacterType = NewType;
	OnRep_CharacterType();
}

void ABFPusher::OnRep_CharacterType()
{
	if (CharacterType == EBFCharacterType::None) return;

	if (TSoftObjectPtr<USkeletalMesh>* Found = CharacterMeshMap.Find(CharacterType))
	{
		if (USkeletalMesh* MeshAsset = Found->LoadSynchronous())
		{
			if (GetMesh())
			{
				GetMesh()->SetSkeletalMeshAsset(MeshAsset);
			}
		}
	}

	RefreshAnimInstanceCache();
}

void ABFPusher::SetCart(ABFCartPawn* NewCart)
{
	// 서버는 즉시 세팅 (RPC 금지)
	if (HasAuthority())
	{
		Cart = NewCart;
		OnRep_Cart();
		return;
	}

	// 클라는 "내가 소유한 Pusher"에서만 서버에 요청 가능
	if (!IsLocallyControlled())
	{
		return; // <- 핵심: 원격 Pusher(시뮬프록시)에서 RPC 호출 금지
	}

	ServerSetCart(NewCart);
}

void ABFPusher::ServerSetCart_Implementation(ABFCartPawn* NewCart)
{
	Cart = NewCart;
	OnRep_Cart();
}

void ABFPusher::OnRep_Cart()
{
	if (CartDrivingComp)
	{
		CartDrivingComp->SetCart(Cart);
	}

	// UI/캐시 갱신 같은 로컬 처리만
	RefreshAnimInstanceCache();
}

void ABFPusher::OnRep_IsDriving()
{
	ApplyDrivingState_Local(bIsDriving);

	if (CartDrivingComp)
	{
		CartDrivingComp->SetDriving(bIsDriving);
	}
}

void ABFPusher::HardClampControlRotation()
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
