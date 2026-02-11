#include "Component/Cart/BFCartResetComponent.h"

#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"

UBFCartResetComponent::UBFCartResetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBFCartResetComponent::BeginPlay()
{
	Super::BeginPlay();
}

UPrimitiveComponent* UBFCartResetComponent::GetRootPrim() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Cast<UPrimitiveComponent>(Owner->GetRootComponent()) : nullptr;
}

bool UBFCartResetComponent::CanRequestReset() const
{
	const APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (!PawnOwner) return false;
	return PawnOwner->IsLocallyControlled() || PawnOwner->HasAuthority();
}

bool UBFCartResetComponent::IsUprightEnough() const
{
	const AActor* Owner = GetOwner();
	if (!Owner) return true;

	const float Dot = FVector::DotProduct(Owner->GetActorUpVector(), FVector::UpVector);
	return Dot >= UprightDotThreshold;
}

void UBFCartResetComponent::RequestUpright()
{
	if (!CanRequestReset()) return;

	if (!GetOwner()->HasAuthority())
	{
		Server_RequestUpright();
		return;
	}

	DoUprightReset_ServerAuth();
}

void UBFCartResetComponent::Server_RequestUpright_Implementation()
{
	DoUprightReset_ServerAuth();
}

void UBFCartResetComponent::DoUprightReset_ServerAuth()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UPrimitiveComponent* RootPrim = GetRootPrim();
	if (!RootPrim) return;

	const double Now = World->GetTimeSeconds();
	if (LastResetTimeSeconds > 0.0 && (Now - LastResetTimeSeconds) < ResetCooldown)
		return;

	const bool bSim = RootPrim->IsSimulatingPhysics();
	const FVector LinVel = bSim ? RootPrim->GetPhysicsLinearVelocity() : FVector::ZeroVector;
	if (LinVel.Size() > MaxSpeedToAllowReset)
		return;

	if (IsUprightEnough())
		return;

	const FVector Start = Owner->GetActorLocation() + FVector::UpVector * TraceUpDistance;
	const FVector End   = Owner->GetActorLocation() - FVector::UpVector * TraceDownDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CartResetTrace), false, Owner);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	const FVector GroundNormal = bHit ? Hit.ImpactNormal.GetSafeNormal() : FVector::UpVector;
	const FVector Up = bAlignToGroundNormal ? GroundNormal : FVector::UpVector;

	FVector Forward = Owner->GetActorForwardVector();
	Forward = FVector::VectorPlaneProject(Forward, Up).GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		Forward = FVector::VectorPlaneProject(FVector::ForwardVector, Up).GetSafeNormal();
	}

	const FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal();
	const FVector OrthoForward = FVector::CrossProduct(Right, Up).GetSafeNormal();
	const FRotator TargetRot = FRotationMatrix::MakeFromXZ(OrthoForward, Up).Rotator();

	const float Lift = RootPrim->Bounds.BoxExtent.Z + ExtraLift;
	const FVector TargetLoc = bHit ? (Hit.ImpactPoint + Up * Lift) : (Owner->GetActorLocation() + Up * Lift);

	if (bSim)
	{
		RootPrim->SetPhysicsLinearVelocity(FVector::ZeroVector, false);
		RootPrim->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector, false);

		RootPrim->SetWorldLocationAndRotation(TargetLoc, TargetRot, false, nullptr, ETeleportType::TeleportPhysics);
		RootPrim->WakeAllRigidBodies();
	}
	else
	{
		Owner->SetActorLocationAndRotation(TargetLoc, TargetRot, false, nullptr, ETeleportType::TeleportPhysics);
	}

	LastResetTimeSeconds = Now;
}