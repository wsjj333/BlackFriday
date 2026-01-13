#include "Vehicle/Cart/BFCartPawn.h"
#include "Vehicle/Cart/BFCartMovementComponent.h"
#include "Character/CartDriver/BFCartDriverCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"

#include "Kismet/GameplayStatics.h"

ABFCartPawn::ABFCartPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		AActor::StaticClass(),
		FoundActors
	);

	for (AActor* A : FoundActors)
	{
		// if (ABFCartDriverCharacter* Pusher = Cast<ABFCartDriverCharacter>(A))
		// {
		// 	CartDriver = Pusher;
		// 	break;
		// }
	}
}

void ABFCartPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ABFCartPawn::SuspensionCast(USceneComponent* WheelComp) const
{
	FHitResult HitResult;
	
	FVector Start = WheelComp->GetComponentLocation();
	FVector End = Start + WheelComp->GetUpVector() * -60.0f;
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	
	DrawDebugLine(
		GetWorld(),
		Start,
		End,
		FColor::Red,
		false,
		5.0f,
		0.1f,
		1.0f
		);
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
		);
	
	if (!bHit)
	{
		return;
	}
	
	float HitResultDistance = HitResult.Distance;
}
