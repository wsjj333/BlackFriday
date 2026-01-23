#include "Character/Pusher/BFPusher.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Data/Enums/BFCharacterType.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Vehicle/Cart/BFCartPawn.h"
#include "Character/Common/BFCharacterAnimInstance.h"
#include "Kismet/KismetMathLibrary.h"

ABFPusher::ABFPusher()
{
	GetCapsuleComponent()->SetCapsuleHalfHeight(110.0f);
	
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -110.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f));
	
	// 캐릭터를 지정하지 않았을 경우 초기값 사용
	CharacterType = EBFCharacterType::AfroHairMan;
}

void ABFPusher::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (!bIsDriving || !Cart)
	{
		return;
	}
	
	USkeletalMeshComponent* MeshComp = GetMesh();
	
	const FTransform HandleL_WS = Cart->GetHandleLTransform();
	const FTransform HandleR_WS = Cart->GetHandleRTransform();
	
	// World Space -> Component Space
	const FTransform HandleL_CS = HandleL_WS.GetRelativeTransform(MeshComp->GetComponentTransform());
	const FTransform HandleR_CS = HandleR_WS.GetRelativeTransform(MeshComp->GetComponentTransform());
	
	if (CachedAnimInstance)
	{
		CachedAnimInstance->HandleTargetL_CS = HandleL_CS;
		CachedAnimInstance->HandleTargetR_CS = HandleR_CS;
	}
}

void ABFPusher::BeginPlay()
{
	Super::BeginPlay();
	
	SetCurrentSkeletalMeshAsset(CharacterType);
	
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
	
	if (const USkeletalMeshComponent* MeshComp = GetMesh())
	{
		CachedAnimInstance =
			Cast<UBFCharacterAnimInstance>(MeshComp->GetAnimInstance());
	}
}

void ABFPusher::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// 소유 클라이언트만 바인딩
	if (!IsLocallyControlled())
	{
		return;
	}
	
	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	// EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABFPusher::HandleMoveInput);
	
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABFPusher::HandleMoveInput);
	// EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &ABFPusher::HandleMoveInput);
	// EnhancedInput->BindAction(MoveAction, ETriggerEvent::Canceled,  this, &ABFPusher::HandleMoveInput);
	
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABFPusher::HandleLookInput);
	
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ABFPusher::OnJumpPressed);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ABFPusher::OnJumpReleased);
	
	EnhancedInput->BindAction(DriveModeAction, ETriggerEvent::Started, this, &ABFPusher::OnToggleDriveModePressed);
}

void ABFPusher::HandleMoveInput(const FInputActionValue& Value)
{
	if (bIsDriving)
	{
		Cart->SetAccelerationInput(Value);
		return;
	}
	
	const FVector2D MoveAxis = Value.Get<FVector2D>();
	
	const float MoveX = MoveAxis.X;
	const float MoveY = MoveAxis.Y;
	
	// 좌우 이동
	const FRotator ControlRot = GetControlRotation();
	FRotator RotForMove(0.0f, ControlRot.Yaw, ControlRot.Roll);
	FVector WorldDirection = UKismetMathLibrary::GetRightVector(RotForMove);
	AddMovementInput(WorldDirection, MoveX);
	
	// 전후 이동
	RotForMove = FRotator(0.0f, ControlRot.Yaw, 0.0f);
	WorldDirection = UKismetMathLibrary::GetForwardVector(RotForMove);
	AddMovementInput(WorldDirection, MoveY);
}


void ABFPusher::HandleLookInput(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();
	
	const float LookX = LookAxis.X;
	const float LookY = LookAxis.Y;
	
	AddControllerYawInput(LookX);
	AddControllerPitchInput(LookY);
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
	ToggleDrivingMode();
	
	if (!Cart)
	{
		return;
	}
	
	USceneComponent* StandAnker = Cart->GetPusherStandAnkerComponent();
	const FAttachmentTransformRules Rules(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepWorld,
		true);
	AttachToComponent(StandAnker, Rules);
}

void ABFPusher::SetCurrentSkeletalMeshAsset(EBFCharacterType NewCharacterType)
{
	bool bIsValid =
	StaticEnum<EBFCharacterType>()->IsValidEnumValue(
		static_cast<int64>(NewCharacterType)
	)
	&& NewCharacterType != EBFCharacterType::None;
	
	if (!bIsValid)
	{
		return;
	}
	
	CharacterType = NewCharacterType;
	GetMesh()->SetSkeletalMeshAsset(CharacterMeshMap[CharacterType].LoadSynchronous());
}

void ABFPusher::ToggleDrivingMode()
{
	bIsDriving = !bIsDriving;
}

bool ABFPusher::IsDriving() const
{
	return bIsDriving;
}

ABFCartPawn* ABFPusher::GetCart() const
{
	return Cart;
}

void ABFPusher::SetCart(ABFCartPawn* NewCart)
{
	Cart = NewCart;
}
