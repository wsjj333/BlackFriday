#include "Character/CartDriver/BFCartDriverController.h"
#include "Character/CartDriver/BFCartDriverCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

ABFCartDriverController::ABFCartDriverController()
{
    bShowMouseCursor = false;
}

void ABFCartDriverController::BeginPlay()
{
    Super::BeginPlay();

    // 로컬 플레이어의 Enhanced Input Subsystem에 Mapping Context 추가
    if (ULocalPlayer* LP = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, DefaultMappingPriority);
            }
        }
    }
}

void ABFCartDriverController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EIC)
    {
        // 프로젝트가 EnhancedInputComponent를 사용하도록 PlayerController/Pawn의 InputComponent 설정이 되어 있어야 합니다.
        return;
    }

    // Move / Look: Triggered에서 계속 값이 들어옵니다.
    if (IA_Move)
    {
        EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ABFCartDriverController::OnMove);
        // 키를 떼면 0이 들어오지 않는 세팅도 있어서 Completed도 안전하게 묶어줌
        EIC->BindAction(IA_Move, ETriggerEvent::Completed, this, &ABFCartDriverController::OnMove);
        EIC->BindAction(IA_Move, ETriggerEvent::Canceled, this, &ABFCartDriverController::OnMove);
    }

    if (IA_Look)
    {
        EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ABFCartDriverController::OnLook);
    }

    // Drift: 눌렀을 때/뗐을 때
    if (IA_Drift)
    {
        EIC->BindAction(IA_Drift, ETriggerEvent::Started, this, &ABFCartDriverController::OnDriftStarted);
        EIC->BindAction(IA_Drift, ETriggerEvent::Completed, this, &ABFCartDriverController::OnDriftCompleted);
    }

    // Interact: 눌렀을 때 1회
    if (IA_Interact)
    {
        EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &ABFCartDriverController::OnInteractStarted);
    }
}

void ABFCartDriverController::OnMove(const FInputActionValue& Value)
{
    FVector2D MoveAxis = Value.Get<FVector2D>();
    
    // 안전장치: 값이 이상하게 남는 경우를 방지
    if (!Value.IsNonZero())
    {
        MoveAxis = FVector2D::ZeroVector;
    }

    if (ABFCartDriverCharacter* C = Cast<ABFCartDriverCharacter>(GetPawn()))
    {
        C->InputMove(MoveAxis);
    }
}

void ABFCartDriverController::OnLook(const FInputActionValue& Value)
{
    const FVector2D LookAxis = Value.Get<FVector2D>();

    if (ABFCartDriverCharacter* C = Cast<ABFCartDriverCharacter>(GetPawn()))
    {
        C->InputLook(LookAxis);
    }
}

void ABFCartDriverController::OnDriftStarted(const FInputActionValue& Value)
{
    if (ABFCartDriverCharacter* C = Cast<ABFCartDriverCharacter>(GetPawn()))
    {
        C->InputDriftPressed();
    }
}

void ABFCartDriverController::OnDriftCompleted(const FInputActionValue& Value)
{
    if (ABFCartDriverCharacter* C = Cast<ABFCartDriverCharacter>(GetPawn()))
    {
        C->InputDriftReleased();
    }
}

void ABFCartDriverController::OnInteractStarted(const FInputActionValue& Value)
{
    if (ABFCartDriverCharacter* C = Cast<ABFCartDriverCharacter>(GetPawn()))
    {
        C->InputInteract();
    }
}
