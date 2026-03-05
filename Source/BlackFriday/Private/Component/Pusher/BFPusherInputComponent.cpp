#include "Component/Pusher/BFPusherInputComponent.h"

// Enhanced Input
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

// For movement/look math
#include "Character/Pusher/BFPusher.h"
#include "Component/Pusher/BFPusherDriveComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/PlayerController.h"

#include "Interfaces/BFInputSink.h"
#include "Vehicle/Cart/BFCartMovementComponent.h"
#include "Vehicle/Cart/BFCartPawn.h"

UBFPusherInputComponent::UBFPusherInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBFPusherInputComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPusher = Cast<ABFPusher>(GetOwner());

	AddMappingContextIfLocal();
}

ABFPusher* UBFPusherInputComponent::GetOwnerPusher()
{
	// 1) 캐시가 이미 있으면 그대로 사용
	if (OwnerPusher)
	{
		return OwnerPusher.Get();
	}

	// 2) 캐시가 없으면 지금 오너에서 다시 캐시
	OwnerPusher = Cast<ABFPusher>(GetOwner());
	return OwnerPusher.Get();
}

void UBFPusherInputComponent::AddMappingContextIfLocal()
{
	const ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher || !Pusher->IsLocallyControlled())
	{
		return;
	}

	const APlayerController* PC = Cast<APlayerController>(Pusher->GetController());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		return;
	}

	if (!PusherMappingContext)
	{
		return;
	}

	Subsystem->AddMappingContext(PusherMappingContext, 0);
}

void UBFPusherInputComponent::BindInput(UInputComponent* PlayerInputComponent)
{
	const ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Pusher->GetController());
	if (!PC)
	{
		return; // 아직 Possess 안 됨 → PawnClientRestart에서 다시 호출
	}

	if (!PC->IsLocalController())
	{
		return;
	}

	if (!PlayerInputComponent)
	{
		return;
	}

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		// 프로젝트 전제상 EnhancedInput 사용 중이니 여기서 assert 성격으로 처리
		ensureMsgf(
			false, TEXT("UBFPusherInputComponent::BindInput - PlayerInputComponent is not EnhancedInputComponent"));
		return;
	}

	// --- 기존 ABFPusher 바인딩 그대로 ---
	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this,
		                          &UBFPusherInputComponent::OnMoveInputTriggered);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this,
								  &UBFPusherInputComponent::OnMoveInputEnded);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Canceled, this,
								  &UBFPusherInputComponent::OnMoveInputEnded);
	}

	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this,
		                          &UBFPusherInputComponent::HandleLookInput);
	}

	if (JumpAction)
	{
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &UBFPusherInputComponent::OnJumpPressed);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &UBFPusherInputComponent::OnJumpReleased);
	}

	if (DriveModeAction)
	{
		EnhancedInput->BindAction(DriveModeAction, ETriggerEvent::Started, this,
		                          &UBFPusherInputComponent::OnToggleDriveModePressed);
	}

	// --- 카트 입력: 기존엔 CartDrivingComp.Get()에 직접 바인딩했지만
	// 컴포넌트 분리 첫 단계에서는 InputComp가 라우팅해도 됨 ---
	// (OwnerPusher->CartDrivingComp 접근이 private이면, ABFPusher에 GetCartDrivingComp() getter 하나 추가 추천)

	if (AccelerationAction)
	{
		EnhancedInput->BindAction(AccelerationAction, ETriggerEvent::Triggered, this,
								  &UBFPusherInputComponent::OnAccelTriggered);
		EnhancedInput->BindAction(AccelerationAction, ETriggerEvent::Completed, this,
		                          &UBFPusherInputComponent::OnAccelEnded);
		EnhancedInput->BindAction(AccelerationAction, ETriggerEvent::Canceled, this,
		                          &UBFPusherInputComponent::OnAccelEnded);
	}

	if (SteerAction)
	{
		EnhancedInput->BindAction(SteerAction, ETriggerEvent::Triggered, this,
		                          &UBFPusherInputComponent::OnSteerTriggered);
		EnhancedInput->BindAction(SteerAction, ETriggerEvent::Completed, this, &UBFPusherInputComponent::OnSteerEnded);
	}

	if (DriftAction)
	{
		EnhancedInput->BindAction(DriftAction, ETriggerEvent::Started, this, &UBFPusherInputComponent::OnDriftStarted);
		EnhancedInput->BindAction(DriftAction, ETriggerEvent::Canceled, this, &UBFPusherInputComponent::OnDriftEnded);
		EnhancedInput->BindAction(DriftAction, ETriggerEvent::Completed, this, &UBFPusherInputComponent::OnDriftEnded);
	}
	
	if (CartRecoverAction)
	{
		EnhancedInput->BindAction(CartRecoverAction, ETriggerEvent::Started, this, &UBFPusherInputComponent::OnRecoverCart);
	}
}

void UBFPusherInputComponent::EnsureMappingContext()
{
	AddMappingContextIfLocal();
}

void UBFPusherInputComponent::SetInputSink(const TScriptInterface<IBFInputSink>& InInputSink)
{
	InputSink = InInputSink;
}

// -------------------- Bound Functions --------------------

void UBFPusherInputComponent::OnMoveInputTriggered(const FInputActionValue& Value)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher)
	{
		return;
	}
	
	// 기존 코드와 동일: 운전 중이면 캐릭터 이동 입력 무시
	if (Pusher->IsDriving())
	{
		return;
	}
	
	const FVector2D MoveAxis = Value.Get<FVector2D>();
	const float MoveX = MoveAxis.X;
	const float MoveY = MoveAxis.Y;
	
	const FRotator ControlRot = Pusher->GetControlRotation();
	FRotator RotForMove(0.0f, ControlRot.Yaw, 0.0f);
	
	FVector WorldDirection = UKismetMathLibrary::GetRightVector(RotForMove);
	Pusher->AddMovementInput(WorldDirection, MoveX);
	
	WorldDirection = UKismetMathLibrary::GetForwardVector(RotForMove);
	Pusher->AddMovementInput(WorldDirection, MoveY);
	
	if (InputSink)
	{
		InputSink->SetMoveInput(FVector2D(MoveAxis.Y, MoveAxis.X));
	}
}

void UBFPusherInputComponent::OnMoveInputEnded()
{
	if (InputSink)
	{
		InputSink->SetMoveInput(FVector2D(0.0f, 0.0f));
	}
}

void UBFPusherInputComponent::HandleLookInput(const FInputActionValue& Value)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher || !Pusher->IsLocallyControlled())
	{
		return;
	}

	const FVector2D LookAxis = Value.Get<FVector2D>();
	const float LookX = LookAxis.X;
	const float LookY = LookAxis.Y;
	
	APlayerController* PC = Cast<APlayerController>(Pusher->GetController());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
	
	if (Pusher->IsDriving())
	{
		FRotator ControlRot = PC->GetControlRotation();
		ControlRot.Yaw += LookX;
		ControlRot.Pitch += LookY;
		
		PC->SetControlRotation(ControlRot);
	}
	else
	{
		Pusher->AddControllerYawInput(LookX);
		Pusher->AddControllerPitchInput(LookY);
		
		if (InputSink)
		{
			InputSink->SetControlYawDegrees(PC->GetControlRotation().Yaw);
		}
	}
}

void UBFPusherInputComponent::OnJumpPressed(const FInputActionValue& /*Value*/)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher) return;

	// Pusher->Jump();
	
	if (InputSink)
	{
		InputSink->SetJumpHeld(true);
	}
}

void UBFPusherInputComponent::OnJumpReleased(const FInputActionValue& /*Value*/)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher) return;

	// Pusher->StopJumping();
	
	if (InputSink)
	{
		InputSink->SetJumpHeld(false);
	}
}

void UBFPusherInputComponent::OnToggleDriveModePressed(const FInputActionValue& /*Value*/)
{
	const ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher) return;
	
	if (!Pusher->IsOverlappingCart()) return;

	if (UBFPusherDriveComponent* Drive = Pusher->FindComponentByClass<UBFPusherDriveComponent>())
	{
		Drive->ToggleDrivingMode();
	}
}

static UBFCartMovementComponent* ResolveCartMoveComp(const ABFPusher* Pusher)
{
	if (!Pusher) return nullptr;
	return Pusher->GetCartDrivingComp();
}

void UBFPusherInputComponent::OnAccelTriggered(const FInputActionValue& Value)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher || !Pusher->IsDriving()) return;

	if (UBFCartMovementComponent* CartMovementComp = ResolveCartMoveComp(Pusher))
	{
		CartMovementComp->Input_AccelTriggered(Value);
	}
}

void UBFPusherInputComponent::OnAccelEnded(const FInputActionValue& Value)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher || !Pusher->IsDriving()) return;

	if (UBFCartMovementComponent* CartMovementComp = ResolveCartMoveComp(Pusher))
	{
		CartMovementComp->Input_AccelEnded(Value);
	}
}

void UBFPusherInputComponent::OnSteerTriggered(const FInputActionValue& Value)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher || !Pusher->IsDriving()) return;

	if (UBFCartMovementComponent* CartMovementComp = ResolveCartMoveComp(Pusher))
	{
		CartMovementComp->Input_SteerTriggered(Value);
	}
}

void UBFPusherInputComponent::OnSteerEnded(const FInputActionValue& Value)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher || !Pusher->IsDriving()) return;

	if (UBFCartMovementComponent* CartMovementComp = ResolveCartMoveComp(Pusher))
	{
		CartMovementComp->Input_SteerEnded(Value);
	}
}

void UBFPusherInputComponent::OnDriftStarted(const FInputActionValue& Value)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher || !Pusher->IsDriving()) return;

	if (UBFCartMovementComponent* CartMovementComp = ResolveCartMoveComp(Pusher))
	{
		CartMovementComp->Input_DriftStarted(Value);
	}
}

void UBFPusherInputComponent::OnDriftEnded(const FInputActionValue& Value)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher || !Pusher->IsDriving()) return;

	if (UBFCartMovementComponent* CartMovementComp = ResolveCartMoveComp(Pusher))
	{
		CartMovementComp->Input_DriftEnded(Value);
	}
}

void UBFPusherInputComponent::OnRecoverCart()
{
	if (!OwnerPusher)
	{
		return;
	}
	
	ABFCartPawn* Cart = OwnerPusher->GetCart();
	
	if (!Cart)
	{
		return;
	}
	
	if (OwnerPusher->IsDriving())
	{
		return;
	}
	
	if (OwnerPusher->HasAuthority())
	{
		Cart->RequestUpright();
	}
	else
	{
		ServerRequestCartUpright();
	}
}

void UBFPusherInputComponent::ServerRequestCartUpright_Implementation()
{
	if (!OwnerPusher) return;

	ABFCartPawn* Cart = OwnerPusher->GetCart();
	if (!Cart) return;

	if (OwnerPusher->IsDriving()) return;

	Cart->RequestUpright();
}
 