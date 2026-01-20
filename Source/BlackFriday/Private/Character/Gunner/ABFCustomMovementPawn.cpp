// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Gunner/ABFCustomMovementPawn.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

AABFCustomMovementPawn::AABFCustomMovementPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    // 캡슐 컴포넌트 생성 (루트)
    CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
    RootComponent = CapsuleComponent;
    CapsuleComponent->InitCapsuleSize(42.0f, 96.0f);
    CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));
    CapsuleComponent->SetSimulatePhysics(false); // 물리 시뮬레이션 비활성화

    // 메시 컴포넌트 생성
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootComponent);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // 변수 초기화
    Velocity = FVector::ZeroVector;
    InputVector = FVector::ZeroVector;
    bIsGrounded = false;
    bWantsToJump = false;
    JumpCount = 0;
    LastGroundNormal = FVector::UpVector;
    GravityZ = 0.0f;
}

void AABFCustomMovementPawn::BeginPlay()
{
    Super::BeginPlay();

    // 월드 중력값 가져오기 (최적화: 매 프레임마다 가져오지 않음)
    if (UWorld* World = GetWorld())
    {
        GravityZ = World->GetGravityZ() * GravityScale;
    }

    // Enhanced Input Mapping Context 등록
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (InputMappingContext)
            {
                Subsystem->AddMappingContext(InputMappingContext, InputMappingPriority);
            }
        }
    }
}

void AABFCustomMovementPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 델타타임 검증 (최적화)
    if (DeltaTime < MIN_TICK_TIME)
    {
        return;
    }

    UpdateMovement(DeltaTime);
}

void AABFCustomMovementPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    // Enhanced Input Component로 캐스팅
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        // Move Action 바인딩 (Triggered: 입력이 활성화되어 있는 동안 매 프레임 호출)
        if (MoveAction)
        {
            EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AABFCustomMovementPawn::Move);
        }

        // Look Action 바인딩 (Triggered)
        if (LookAction)
        {
            EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AABFCustomMovementPawn::Look);
        }

        // Jump Action 바인딩 (Started: 입력 시작, Completed: 입력 종료)
        if (JumpAction)
        {
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AABFCustomMovementPawn::Jump);
            EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AABFCustomMovementPawn::StopJumping);
        }
    }
}

void AABFCustomMovementPawn::Move(const FInputActionValue& Value)
{
    // FInputActionValue에서 Vector2D 값 추출
    const FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller && !MovementVector.IsNearlyZero())
    {
        // 컨트롤러의 Yaw 회전만 사용 (수평 이동)
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // 전방 및 우측 방향 계산
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // 입력 벡터 업데이트 (X: 전후, Y: 좌우)
        InputVector = (ForwardDirection * MovementVector.Y) + (RightDirection * MovementVector.X);
    }
    else
    {
        InputVector = FVector::ZeroVector;
    }
}

void AABFCustomMovementPawn::Look(const FInputActionValue& Value)
{
    // 마우스/스틱 입력으로 카메라 회전
    const FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller)
    {
        // Yaw (좌우 회전)
        AddControllerYawInput(LookAxisVector.X);

        // Pitch (상하 회전)
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

void AABFCustomMovementPawn::Jump(const FInputActionValue& Value)
{
    bWantsToJump = true;
}

void AABFCustomMovementPawn::StopJumping(const FInputActionValue& Value)
{
    bWantsToJump = false;
}

void AABFCustomMovementPawn::UpdateMovement(float DeltaTime)
{
    // 1. 지면 감지
    FHitResult GroundHit;
    bool bWasGrounded = bIsGrounded;
    bIsGrounded = PerformGroundTrace(GroundHit);

    // 착지 시 점프 카운트 초기화
    if (bIsGrounded && !bWasGrounded)
    {
        JumpCount = 0;
    }

    // 2. 점프 처리
    if (bWantsToJump && JumpCount < MaxJumpCount)
    {
        Velocity.Z = JumpVelocity;
        JumpCount++;
        bWantsToJump = false;
        bIsGrounded = false;
    }

    // 3. 중력 적용
    ApplyGravity(DeltaTime);

    // 4. 입력 기반 이동 처리
    ProcessMovementInput(DeltaTime);

    // 5. 마찰력 적용
    ApplyFriction(DeltaTime);

    // 6. 실제 이동 수행
    MoveAndSlide(DeltaTime);

    // 입력 벡터 초기화 (다음 프레임을 위해)
    InputVector = FVector::ZeroVector;
}

void AABFCustomMovementPawn::ApplyGravity(float DeltaTime)
{
    if (!bIsGrounded)
    {
        // 중력 가속도 적용
        Velocity.Z += GravityZ * DeltaTime;

        // 최대 낙하 속도 제한
        Velocity.Z = FMath::Max(Velocity.Z, -MaxFallSpeed);
    }
    else
    {
        // 지면에 있을 때는 Z 속도를 0으로 (경사면에서는 약간 음수)
        if (Velocity.Z < 0.0f)
        {
            Velocity.Z = 0.0f;
        }
    }
}

void AABFCustomMovementPawn::ProcessMovementInput(float DeltaTime)
{
    if (InputVector.IsNearlyZero())
    {
        return;
    }

    // 입력 벡터 정규화
    FVector NormalizedInput = InputVector.GetSafeNormal();

    // 공중 제어력 조정
    float ControlPower = bIsGrounded ? 1.0f : AirControl;

    // 가속도 적용
    FVector Acceleration = NormalizedInput * MaxAcceleration * ControlPower * DeltaTime;

    // 수평 속도만 업데이트
    Velocity.X += Acceleration.X;
    Velocity.Y += Acceleration.Y;

    // 최대 속도 제한 (수평만)
    FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
    float CurrentSpeed = HorizontalVelocity.Size();

    if (CurrentSpeed > MoveSpeed)
    {
        HorizontalVelocity = HorizontalVelocity.GetSafeNormal() * MoveSpeed;
        Velocity.X = HorizontalVelocity.X;
        Velocity.Y = HorizontalVelocity.Y;
    }
}

bool AABFCustomMovementPawn::PerformGroundTrace(FHitResult& OutHit)
{
    if (!CapsuleComponent)
    {
        return false;
    }

    FVector StartLocation = GetActorLocation();
    FVector EndLocation = StartLocation - FVector(0, 0, CapsuleComponent->GetScaledCapsuleHalfHeight() + GroundTraceDistance);

    // 충돌 쿼리 파라미터 설정 (최적화: 복잡한 트레이스 비활성화)
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.bTraceComplex = false; // 단순 충돌만 사용

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        OutHit,
        StartLocation,
        EndLocation,
        ECC_Visibility,
        QueryParams
    );

    if (bHit)
    {
        // 경사면 각도 체크
        float SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(OutHit.Normal, FVector::UpVector)));

        if (SlopeAngle <= MaxWalkableSlope)
        {
            LastGroundNormal = OutHit.Normal;
            return true;
        }
    }

    return false;
}

void AABFCustomMovementPawn::ApplyFriction(float DeltaTime)
{
    float FrictionToApply = bIsGrounded ? GroundFriction : AirFriction;

    // 수평 속도에만 마찰력 적용
    FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
    float CurrentSpeed = HorizontalVelocity.Size();

    if (CurrentSpeed > 0.0f)
    {
        // 마찰력 계산
        float FrictionAmount = FrictionToApply * DeltaTime;
        float NewSpeed = FMath::Max(0.0f, CurrentSpeed - FrictionAmount);

        // 속도 업데이트
        if (NewSpeed > 0.0f)
        {
            float SpeedRatio = NewSpeed / CurrentSpeed;
            Velocity.X *= SpeedRatio;
            Velocity.Y *= SpeedRatio;
        }
        else
        {
            Velocity.X = 0.0f;
            Velocity.Y = 0.0f;
        }
    }
}

void AABFCustomMovementPawn::MoveAndSlide(float DeltaTime)
{
    if (Velocity.IsNearlyZero())
    {
        return;
    }

    FVector Delta = Velocity * DeltaTime;

    // 충돌 처리를 위한 Sweep 사용
    FHitResult Hit;
    FVector NewLocation = GetActorLocation() + Delta;

    // SafeMoveUpdatedComponent 대신 직접 이동 (최적화)
    if (CapsuleComponent)
    {
        CapsuleComponent->MoveComponent(Delta, GetActorRotation(), true, &Hit);

        // 충돌 시 슬라이드 처리
        if (Hit.bBlockingHit)
        {
            // 벽에 부딪혔을 때 슬라이드
            FVector Normal = Hit.Normal;
            FVector SlideVector = FVector::VectorPlaneProject(Delta, Normal);

            // 나머지 이동 수행
            if (!SlideVector.IsNearlyZero())
            {
                CapsuleComponent->MoveComponent(SlideVector, GetActorRotation(), true);
            }

            // 벽 충돌 시 해당 방향 속도 제거
            Velocity = FVector::VectorPlaneProject(Velocity, Normal);
        }
    }
}