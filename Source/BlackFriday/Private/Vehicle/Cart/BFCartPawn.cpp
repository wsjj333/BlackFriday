#include "Vehicle/Cart/BFCartPawn.h"
#include "Vehicle/Cart/BFCartMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

ABFCartPawn::ABFCartPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
    SetRootComponent(Capsule);
    Capsule->InitCapsuleSize(55.f, 45.f);
    Capsule->SetCollisionProfileName(TEXT("Pawn"));

    CartMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartMesh"));
    CartMesh->SetupAttachment(Capsule);
    CartMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CartMesh->SetCollisionProfileName(TEXT("PhysicsActor"));

    Handle = CreateDefaultSubobject<USceneComponent>(TEXT("Handle"));
    Handle->SetupAttachment(Capsule);

    // 핸들 위치는 메시에 맞게 에디터에서 조정 권장
    // (예: X=80~120, Z=50 등)
    Handle->SetRelativeLocation(FVector(100.f, 0.f, 50.f));

    CartMovement = CreateDefaultSubobject<UBFCartMovementComponent>(TEXT("CartMovement"));
    CartMovement->SetUpdatedComponent(Capsule);

    AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void ABFCartPawn::BeginPlay()
{
    Super::BeginPlay();
}

void ABFCartPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

UPawnMovementComponent* ABFCartPawn::GetMovementComponent() const
{
    return CartMovement;
}

void ABFCartPawn::SetThrottle(float Value)
{
    if (CartMovement) CartMovement->SetThrottle(Value);
}

void ABFCartPawn::SetSteer(float Value)
{
    if (CartMovement) CartMovement->SetSteer(Value);
}

void ABFCartPawn::SetDriftHeld(bool bHeld)
{
    if (CartMovement) CartMovement->SetDriftHeld(bHeld);
}
