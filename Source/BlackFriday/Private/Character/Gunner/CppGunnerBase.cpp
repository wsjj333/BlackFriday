// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Gunner/CppGunnerBase.h"
//#include "Camera/CameraComponent.h"
//#include "Components/CapsuleComponent.h"
//#include "GameFramework/CharacterMovementComponent.h"
//#include "GameFramework/SpringArmComponent.h"
//#include "GameFramework/Controller.h"
//#include "EnhancedInputComponent.h"
//#include "EnhancedInputSubsystems.h"
//#include "InputActionValue.h"

ACppGunnerBase::ACppGunnerBase()
{
	//// Set size for collision capsule
	//GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	//// Don't rotate when the controller rotates. Let that just affect the camera.
	//bUseControllerRotationPitch = false;
	//bUseControllerRotationYaw = false;
	//bUseControllerRotationRoll = false;
	//// Configure character movement
	//GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	//GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate
	//// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	//// instead of recompiling to adjust them
	//GetCharacterMovement()->JumpZVelocity = 700.f;
	//GetCharacterMovement()->AirControl = 0.35f;
	//GetCharacterMovement()->MaxWalkSpeed = 500.f;
	//GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	//GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	//GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	//// Create a camera boom (pulls in towards the player if there is a collision)
	//CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	//CameraBoom->SetupAttachment(RootComponent);
	//CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	//CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	//// Create a follow camera
	//FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	//FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	//FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

}
FVector ACppGunnerBase::CalculateStartPositionOffset(
    const FVector& CurrentHandLocation,
    const FVector& TorsoLocation,
    const FVector& PlayerForward,
    const FVector& PlayerRight,
    float CircleRadius,
    float Tolerance
)
{
    // 플레이어 로컬 좌표계 구성
    FVector Forward = PlayerForward;
    Forward.Z = 0.0f;
    Forward.Normalize();

    FVector Right = PlayerRight;
    Right.Z = 0.0f;
    Right.Normalize();

    // 월드 위치를 플레이어 로컬 좌표로 변환
    FVector ToHand = CurrentHandLocation - TorsoLocation;
    const float LocalX = FVector::DotProduct(ToHand, Forward);
    const float LocalY = FVector::DotProduct(ToHand, Right);

    const float CurrentDistance = FMath::Sqrt(LocalX * LocalX + LocalY * LocalY);

    // 이미 원 위에 있는지 확인
    if (FMath::Abs(CurrentDistance - CircleRadius) <= Tolerance)
    {
        return FVector::ZeroVector;
    }

    // 중심에 있으면 전방으로 배치
    if (CurrentDistance < KINDA_SMALL_NUMBER)
    {
        return Forward * CircleRadius;
    }

    // 현재 방향 유지하면서 반지름만 조정
    const float Scale = CircleRadius / CurrentDistance;
    const float TargetLocalX = LocalX * Scale;
    const float TargetLocalY = LocalY * Scale;

    // 로컬 좌표를 월드 좌표로 변환
    const FVector TargetWorldPos = TorsoLocation +
        (Forward * TargetLocalX) +
        (Right * TargetLocalY);

    return TargetWorldPos - CurrentHandLocation;
}

FVector ACppGunnerBase::CalculateCircularMovementOffset(
    const FVector& CurrentHandLocation,
    const FVector& TorsoLocation,
    const FVector& PlayerForward,
    const FVector& PlayerRight,
    float MouseDeltaX,
    float CircleRadius,
    float Sensitivity,
    float DeltaTime
)
{
    // 입력이 너무 작으면 무시
    if (FMath::Abs(MouseDeltaX) < 0.05f)
    {
        return FVector::ZeroVector;
    }

    // 플레이어 로컬 좌표계 구성 (수평면만)
    FVector Forward = PlayerForward;
    Forward.Z = 0.0f;

    if (Forward.SizeSquared() < KINDA_SMALL_NUMBER)
    {
        Forward = FVector::ForwardVector;
    }
    else
    {
        Forward.Normalize();
    }

    FVector Right = PlayerRight;
    Right.Z = 0.0f;

    if (Right.SizeSquared() < KINDA_SMALL_NUMBER)
    {
        Right = FVector::RightVector;
    }
    else
    {
        Right.Normalize();
    }

    // 월드 위치를 플레이어 로컬 좌표로 변환
    const FVector ToHand = CurrentHandLocation - TorsoLocation;
    const float LocalX = FVector::DotProduct(ToHand, Forward);
    const float LocalY = FVector::DotProduct(ToHand, Right);

    const float CurrentDistance = FMath::Sqrt(LocalX * LocalX + LocalY * LocalY);

    if (CurrentDistance < KINDA_SMALL_NUMBER)
    {
        return FVector::ZeroVector;
    }

    // ?? 핵심: 로컬 좌표계에서 각도 계산 (0도 = 플레이어 정면)
    float CurrentAngle = FMath::Atan2(LocalY, LocalX);

    // 각도 제한: 플레이어 기준 전방 180도 + 후방 90도 (좌우 각 45도)
    // 로컬 좌표계에서:
    // 0도 = 정면
    // 90도 = 오른쪽
    // -90도 = 왼쪽
    // ±180도 = 후방

    const float MaxBackAngle = PI * 0.75f;  // 135도 (전방에서 후방으로 135도까지)
    const float EdgeDeadZone = 0.05f;

    // 경계 체크 (로컬 좌표계 기준)
    const bool bAtRightEdge = CurrentAngle >= (MaxBackAngle - EdgeDeadZone);
    const bool bAtLeftEdge = CurrentAngle <= (-MaxBackAngle + EdgeDeadZone);

    // 회전량 계산
    const float RotationRadians = FMath::DegreesToRadians(MouseDeltaX * Sensitivity);

    // 경계에서 같은 방향 입력 차단
    if ((bAtRightEdge && RotationRadians > 0.0f) ||
        (bAtLeftEdge && RotationRadians < 0.0f))
    {
        return FVector::ZeroVector;
    }

    // 새 각도 계산 및 클램핑 (로컬 좌표계 기준)
    float NewAngle = FMath::Clamp(
        CurrentAngle + RotationRadians,
        -MaxBackAngle,
        MaxBackAngle
    );

    // 각도 변화 확인
    if (FMath::Abs(NewAngle - CurrentAngle) < 0.001f)
    {
        return FVector::ZeroVector;
    }

    // ?? 핵심: 새 로컬 위치 계산 (로컬 좌표계)
    const float NewLocalX = FMath::Cos(NewAngle) * CircleRadius;
    const float NewLocalY = FMath::Sin(NewAngle) * CircleRadius;

    // 로컬 좌표를 월드 좌표로 변환
    const FVector NewWorldPos = TorsoLocation +
        (Forward * NewLocalX) +   // 플레이어 전방 방향으로 X
        (Right * NewLocalY);      // 플레이어 오른쪽 방향으로 Y

    // 월드 오프셋 반환
    FVector Offset = NewWorldPos - CurrentHandLocation;
    Offset.Z = 0.0f;

    return Offset;
}

//float ACppGunnerBase::ClampAngleToValidRange(float Angle, float ForwardAngle)
//{
//	// -PI ~ PI 범위로 정규화
//	while (Angle > PI) Angle -= 2.0f * PI;
//	while (Angle < -PI) Angle += 2.0f * PI;
//
//	const float HalfPI = PI * 0.5f;              // 90도
//	const float ThreeQuarterPI = PI * 0.75f;     // 135도
//
//	// 전방 180도 범위: -90도 ~ +90도
//	if (Angle >= -HalfPI && Angle <= HalfPI)
//	{
//		return Angle;
//	}
//
//	// 후방 영역 처리
//	if (Angle > HalfPI)
//	{
//		// 오른쪽 후방 (90도 ~ 180도)
//		if (Angle <= ThreeQuarterPI)
//		{
//			return Angle; // 135도까지 허용
//		}
//		else
//		{
//			return ThreeQuarterPI; // 135도에서 멈춤
//		}
//	}
//	else // Angle < -HalfPI
//	{
//		// 왼쪽 후방 (-90도 ~ -180도)
//		if (Angle >= -ThreeQuarterPI)
//		{
//			return Angle; // -135도까지 허용
//		}
//		else
//		{
//			return -ThreeQuarterPI; // -135도에서 멈춤
//		}
//	}
//}


//void ACppGunnerBase::BeginPlay()
//{
//	// Call the base class  
//	Super::BeginPlay();
//
//	//Add Input Mapping Context
//	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
//	{
//		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
//		{
//			Subsystem->AddMappingContext(DefaultMappingContext, 0);
//		}
//	}
//}
//
////////////////////////////////////////////////////////////////////////////
//// Input
//
//void ACppGunnerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
//{
//	// Set up action bindings
//	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
//
//		// Jumping
//		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
//		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
//
//		// Moving
//		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ACppGunnerBase::Move);
//
//		// Looking
//		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACppGunnerBase::Look);
//	}
//	else
//	{
//		//UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
//	}
//}
//
//void ACppGunnerBase::Move(const FInputActionValue& Value)
//{
//	// input is a Vector2D
//	FVector2D MovementVector = Value.Get<FVector2D>();
//
//	if (Controller != nullptr)
//	{
//		// find out which way is forward
//		const FRotator Rotation = Controller->GetControlRotation();
//		const FRotator YawRotation(0, Rotation.Yaw, 0);
//
//		// get forward vector
//		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
//
//		// get right vector 
//		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
//
//		// add movement 
//		AddMovementInput(ForwardDirection, MovementVector.Y);
//		AddMovementInput(RightDirection, MovementVector.X);
//	}
//}
//
//void ACppGunnerBase::Look(const FInputActionValue& Value)
//{
//	// input is a Vector2D
//	FVector2D LookAxisVector = Value.Get<FVector2D>();
//
//	if (Controller != nullptr)
//	{
//		// add yaw and pitch input to controller
//		AddControllerYawInput(LookAxisVector.X);
//		AddControllerPitchInput(LookAxisVector.Y);
//	}
//}