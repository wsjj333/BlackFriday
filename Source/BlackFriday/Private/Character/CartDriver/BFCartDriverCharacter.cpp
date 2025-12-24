#include "Character/CartDriver/BFCartDriverCharacter.h"
#include "Vehicle/Cart/BFCartPawn.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SceneComponent.h"

ABFCartDriverCharacter::ABFCartDriverCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ABFCartDriverCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void ABFCartDriverCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Pulling 중에는 캐릭터를 핸들에 안정적으로 고정
    // (Attach만으로 충분한 경우도 많지만, 프로젝트별로 보정이 필요할 수 있어 유지)
    if (bIsPullingCart && CurrentCart)
    {
        // 캐릭터가 카트 정면을 바라보게(필요 시)
        const FRotator CartYaw(0.f, CurrentCart->GetActorRotation().Yaw, 0.f);
        SetActorRotation(CartYaw);
    }
}

void ABFCartDriverCharacter::InputMove(const FVector2D& MoveAxis)
{
    if (bIsPullingCart && CurrentCart)
    {
        RouteMoveToCart(MoveAxis);
    }
    else
    {
        RouteMoveToCharacter(MoveAxis);
    }
}

void ABFCartDriverCharacter::InputLook(const FVector2D& LookAxis)
{
    AddControllerYawInput(LookAxis.X);
    AddControllerPitchInput(LookAxis.Y);
}

void ABFCartDriverCharacter::InputDriftPressed()
{
    if (bIsPullingCart && CurrentCart)
    {
        CurrentCart->SetDriftHeld(true);
    }
}

void ABFCartDriverCharacter::InputDriftReleased()
{
    if (bIsPullingCart && CurrentCart)
    {
        CurrentCart->SetDriftHeld(false);
    }
}

void ABFCartDriverCharacter::InputInteract()
{
    if (bIsPullingCart)
    {
        StopPulling();
        return;
    }

    ABFCartPawn* Nearest = FindNearestCart();
    if (Nearest)
    {
        StartPulling(Nearest);
    }
}

void ABFCartDriverCharacter::StartPulling(ABFCartPawn* Cart)
{
    if (!Cart) return;

    CurrentCart = Cart;
    bIsPullingCart = true;

    // 캐릭터 이동은 끄고, 카트가 이동 주체
    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        CMC->StopMovementImmediately();
        CMC->DisableMovement();
    }

    // 핸들 기준(방향 포함) 컴포넌트 가져오기: 가능하면 Arrow를 반환하도록 Cart 쪽을 설계
    USceneComponent* HandleAttach = CurrentCart->GetHandleComponent();          // 최소 위치 기준
    USceneComponent* HandleFacing = CurrentCart->GetHandleFacingComponent();    // 권장: Arrow(또는 Scene) 반환

    if (!HandleAttach)
    {
        // 실패 시 안전 처리
        return;
    }

    // 방향 컴포넌트가 없으면 HandleAttach를 방향 기준으로도 사용
    if (!HandleFacing) HandleFacing = HandleAttach;

    // RootComponent 기준으로 Attach (명시적으로)
    if (USceneComponent* Root = GetRootComponent())
    {
        Root->AttachToComponent(
            HandleFacing,
            FAttachmentTransformRules::SnapToTargetNotIncludingScale
        );

        // 오프셋은 "붙은 컴포넌트 로컬" 기준으로 적용
        // PullAttachOffset: FVector(로컬 위치 오프셋)
        // PullAttachRotOffset: FRotator(로컬 회전 오프셋) - 필요하면 추가
        Root->SetRelativeLocation(PullAttachOffset);
        // Root->SetRelativeRotation(PullAttachRotOffset);
    }

    // 컨트롤 회전 정책(선택): Pull 중에는 컨트롤러 yaw로 캐릭터가 돌아가지 않게 고정하는 편이 보통 안정적
    bUseControllerRotationYaw = false;

    // 드리프트 입력 잔류 초기화
    CurrentCart->SetDriftHeld(false);
    CurrentCart->SetThrottle(0.f);
    CurrentCart->SetSteer(0.f);
}

void ABFCartDriverCharacter::StopPulling()
{
    if (!CurrentCart) return;

    // 카트 입력 정리
    CurrentCart->SetDriftHeld(false);
    CurrentCart->SetThrottle(0.f);
    CurrentCart->SetSteer(0.f);

    // Detach
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    // 캐릭터 이동 복구
    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        CMC->SetMovementMode(EMovementMode::MOVE_Walking);
    }

    bIsPullingCart = false;
    CurrentCart = nullptr;
}

ABFCartPawn* ABFCartDriverCharacter::FindNearestCart() const
{
    const UWorld* World = GetWorld();
    if (!World) return nullptr;

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(World, ABFCartPawn::StaticClass(), Found);

    const FVector MyLoc = GetActorLocation();
    ABFCartPawn* Best = nullptr;
    float BestDistSq = FindCartRadius * FindCartRadius;

    for (AActor* A : Found)
    {
        ABFCartPawn* Cart = Cast<ABFCartPawn>(A);
        if (!Cart) continue;

        const float DistSq = FVector::DistSquared(MyLoc, Cart->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Cart;
        }
    }
    return Best;
}

void ABFCartDriverCharacter::RouteMoveToCharacter(const FVector2D& MoveAxis)
{
    if (!Controller) return;

    const FRotator ControlRot = Controller->GetControlRotation();
    const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

    const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
    const FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

    AddMovementInput(Forward, MoveAxis.Y);
    AddMovementInput(Right,   MoveAxis.X);
}

void ABFCartDriverCharacter::RouteMoveToCart(const FVector2D& MoveAxis)
{
    if (!CurrentCart) return;

    // 마리오카트 느낌:
    //   Y = Throttle(전/후), X = Steer(좌/우)
    CurrentCart->SetThrottle(MoveAxis.Y);
    CurrentCart->SetSteer(MoveAxis.X);
}
