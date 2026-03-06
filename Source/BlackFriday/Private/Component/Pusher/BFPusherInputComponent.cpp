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
	if (OwnerPusher)
	{
		return OwnerPusher.Get();
	}
	
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
	if (!LocalPlayer) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem) return;

	if (!PusherMappingContext) return;
	Subsystem->AddMappingContext(PusherMappingContext, 0);
}

void UBFPusherInputComponent::BindInput(UInputComponent* PlayerInputComponent)
{
	const ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher) return;

	APlayerController* PC = Cast<APlayerController>(Pusher->GetController());
	if (!PC) return;

	if (!PC->IsLocalController()) return;
	if (!PlayerInputComponent) return;

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		ensureMsgf(
			false, TEXT("UBFPusherInputComponent::BindInput - PlayerInputComponent is not EnhancedInputComponent"));
		return;
	}
	
	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this,
		                          &UBFPusherInputComponent::OnMoveTriggered);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this,
								  &UBFPusherInputComponent::OnMoveEnded);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Canceled, this,
								  &UBFPusherInputComponent::OnMoveEnded);
	}

	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this,
		                          &UBFPusherInputComponent::OnLookTriggered);
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

	if (AccelerationAction)
	{
		EnhancedInput->BindAction(AccelerationAction, ETriggerEvent::Started, this,
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

void UBFPusherInputComponent::OnMoveTriggered(const FInputActionValue& Value)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher) return;
	
	// 운전 중이면 캐릭터 이동 입력 무시
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

void UBFPusherInputComponent::OnMoveEnded(const FInputActionValue& Value)
{
	if (InputSink)
	{
		InputSink->SetMoveInput(FVector2D(0.0f, 0.0f));
	}
}

void UBFPusherInputComponent::OnLookTriggered(const FInputActionValue& Value)
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
	
	if (InputSink)
	{
		InputSink->SetJumpHeld(true);
	}
}

void UBFPusherInputComponent::OnJumpReleased(const FInputActionValue& /*Value*/)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher) return;
	
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
	if (!Pusher || !Pusher->IsDriving())
	{
		return;
	}

	if (UBFCartMovementComponent* CartMovementComp = ResolveCartMoveComp(Pusher))
	{
		CartMovementComp->Input_AccelEnded(Value);
	}
}

void UBFPusherInputComponent::OnSteerTriggered(const FInputActionValue& Value)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher || !Pusher->IsDriving())
	{
		return;
	}

	if (UBFCartMovementComponent* CartMovementComp = ResolveCartMoveComp(Pusher))
	{
		CartMovementComp->Input_SteerTriggered(Value);
	}
}

void UBFPusherInputComponent::OnSteerEnded(const FInputActionValue& Value)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher || !Pusher->IsDriving())
	{
		return;
	}

	if (UBFCartMovementComponent* CartMovementComp = ResolveCartMoveComp(Pusher))
	{
		CartMovementComp->Input_SteerEnded(Value);
	}
}

void UBFPusherInputComponent::OnDriftStarted(const FInputActionValue& Value)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher || !Pusher->IsDriving())
	{
		return;
	}

	if (UBFCartMovementComponent* CartMovementComp = ResolveCartMoveComp(Pusher))
	{
		CartMovementComp->Input_DriftStarted(Value);
	}
}

void UBFPusherInputComponent::OnDriftEnded(const FInputActionValue& Value)
{
	ABFPusher* Pusher = GetOwnerPusher();
	if (!Pusher || !Pusher->IsDriving())
	{
		return;
	}

	if (UBFCartMovementComponent* CartMovementComp = ResolveCartMoveComp(Pusher))
	{
		CartMovementComp->Input_DriftEnded(Value);
	}
}

void UBFPusherInputComponent::OnRecoverCart(const FInputActionValue& Value)
{
	if (!OwnerPusher) return;
	
	ABFCartPawn* Cart = OwnerPusher->GetCart();
	if (!Cart) return;
	
	if (OwnerPusher->IsDriving())
	{
		return;
	}
	
	Cart->RequestUpright();
}
