// #include "Component/Cart/BFCartCosmeticComponent.h"
//
// #include "Vehicle/Cart/BFCartPawn.h"
// #include "Components/PrimitiveComponent.h"
// #include "Components/StaticMeshComponent.h"
// #include "Components/SceneComponent.h"
//
// #include "Vehicle/Cart/BFCartQuantize.h"
//
// UBFCartCosmeticComponent::UBFCartCosmeticComponent()
// {
// 	PrimaryComponentTick.bCanEverTick = true;
// 	PrimaryComponentTick.TickGroup = TG_PostPhysics;
// 	SetIsReplicatedByDefault(false);
// }
//
// void UBFCartCosmeticComponent::BeginPlay()
// {
// 	Super::BeginPlay();
// 	CacheRefs();
// }
//
// void UBFCartCosmeticComponent::CacheRefs()
// {
// 	Cart = Cast<ABFCartPawn>(GetOwner());
// 	if (!Cart.IsValid()) return;
//
// 	TWeakObjectPtr<USceneComponent> OutPivot;
// 	TWeakObjectPtr<UStaticMeshComponent> OutBody;
// 	TWeakObjectPtr<UPrimitiveComponent> OutRoot;
//
// 	// Cart->GatherCosmeticRefs(WheelMeshes, OutPivot, OutBody, OutRoot);
// 	Pivot = OutPivot;
// 	CartBody = OutBody;
// 	RootPrim = OutRoot;
// }
//
// void UBFCartCosmeticComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
// {
// 	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
//
// 	if (!Cart.IsValid() || !RootPrim.IsValid())
// 	{
// 		CacheRefs();
// 		if (!Cart.IsValid() || !RootPrim.IsValid()) return;
// 	}
//
// 	// --- Anim caches from physics ---
// 	const FVector Vel = RootPrim->GetPhysicsLinearVelocity();
// 	const float Speed2D = Vel.Size2D();
//
// 	float Accel = 0.f;
// 	if (DeltaTime > KINDA_SMALL_NUMBER)
// 	{
// 		Accel = (Speed2D - PrevSpeed2D) / DeltaTime;
// 	}
// 	PrevSpeed2D = Speed2D;
//
// 	Cart->SetAnimCaches(Accel, Vel);
//
// 	// --- Wheel spin (speed-based) ---
// 	const float Spin = (Speed2D * WheelSpinSpeedScale) * DeltaTime;
// 	const FRotator WheelRot(Spin, 0.f, 0.f);
//
// 	for (const auto& W : WheelMeshes)
// 	{
// 		if (W.IsValid()) W->AddLocalRotation(WheelRot);
// 	}
//
// 	// --- Visual steer (uses replicated input only) ---
// 	const float SteerAxis = BFCartQuantize::DequantizeAxis(Cart->GetInputStateQ().SteerAxisQ);
// 	const float TargetYaw = FMath::Sign(SteerAxis) * VisualSteerYawDeg;
// 	const FRotator TargetRot(0.f, TargetYaw, 0.f);
//
// 	if (Pivot.IsValid())
// 	{
// 		const FRotator NewPivot = FMath::RInterpTo(Pivot->GetRelativeRotation(), TargetRot, DeltaTime, RotationInterpSpeed);
// 		Pivot->SetRelativeRotation(NewPivot);
// 	}
// 	if (CartBody.IsValid())
// 	{
// 		const FRotator NewBody = FMath::RInterpTo(CartBody->GetRelativeRotation(), TargetRot, DeltaTime, RotationInterpSpeed);
// 		CartBody->SetRelativeRotation(NewBody);
// 	}
// }