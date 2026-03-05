#include "Component/Pusher/BFCartOverlapDetectorComponent.h"

// Engine
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"

// Character
#include "Character/Pusher/BFPusher.h"

// Vehicle
#include "Vehicle/Cart/BFCartPawn.h"

UBFCartOverlapDetectorComponent::UBFCartOverlapDetectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CartOverlapSphere"));
	OverlapSphere->InitSphereRadius(SphereRadius);
	OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapSphere->SetCollisionObjectType(ECC_WorldDynamic);
	
	OverlapSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	
	OverlapSphere->SetGenerateOverlapEvents(true);
}

void UBFCartOverlapDetectorComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Owner Root에 Attach
	if (USceneComponent* Root = Owner->GetRootComponent())
	{
		OverlapSphere->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
	}
	
	OverlapSphere->SetSphereRadius(SphereRadius);
	
	OverlapSphere->OnComponentBeginOverlap.AddDynamic(this, &UBFCartOverlapDetectorComponent::HandleBeginOverlap);
	OverlapSphere->OnComponentEndOverlap.AddDynamic(this, &UBFCartOverlapDetectorComponent::HandleEndOverlap);

	if (Owner->HasAuthority())
	{
		ForceRecheckOverlap_ServerOnly();
	}
}

void UBFCartOverlapDetectorComponent::HandleBeginOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (!OtherActor || OtherActor == Owner)
	{
		return;
	}
	
	if (ABFCartPawn* Cart = Cast<ABFCartPawn>(OtherActor))
	{
		OverlappingCarts.AddUnique(Cart);
		RecalculateOverlapState_ServerOnly();
	}
}

void UBFCartOverlapDetectorComponent::HandleEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex
)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
		return;

	if (!OtherActor || OtherActor == Owner)
		return;

	if (ABFCartPawn* Cart = Cast<ABFCartPawn>(OtherActor))
	{
		OverlappingCarts.RemoveSingleSwap(Cart);
		RecalculateOverlapState_ServerOnly();
	}
}

void UBFCartOverlapDetectorComponent::RecalculateOverlapState_ServerOnly()
{
	check(GetOwner() && GetOwner()->HasAuthority());

	OverlappingCarts.RemoveAllSwap([](const TObjectPtr<ABFCartPawn>& C)
	{
		return !IsValid(C);
	});

	const bool bNew = (OverlappingCarts.Num() > 0);

	if (bIsOverlappingCart != bNew)
	{
		bIsOverlappingCart = bNew;
		
		ABFPusher* Pusher = Cast<ABFPusher>(GetOwner());
		if (!Pusher) return;
		
		if (Pusher->GetCart()) return;
		
		if (OverlappingCarts.Num() != 1) return;
		
		ABFCartPawn* Cart = OverlappingCarts[0];
		if (!IsValid(OverlappingCarts[0])) return;
		
		Pusher->SetCart(Cart);
	}
}

void UBFCartOverlapDetectorComponent::ForceRecheckOverlap_ServerOnly()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
		return;
	
	if (!OverlapSphere) return;

	TArray<AActor*> Actors;
	OverlapSphere->GetOverlappingActors(Actors, ABFCartPawn::StaticClass());

	OverlappingCarts.Reset();
	for (AActor* A : Actors)
	{
		if (ABFCartPawn* C = Cast<ABFCartPawn>(A))
		{
			if (C != Owner) // 혹시 Owner도 ABFCartPawn일 수 있으니 안전장치
			{
				OverlappingCarts.AddUnique(C);
			}
		}
	}

	RecalculateOverlapState_ServerOnly();
}

void UBFCartOverlapDetectorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBFCartOverlapDetectorComponent, bIsOverlappingCart);
	DOREPLIFETIME(UBFCartOverlapDetectorComponent, OverlappingCarts);
}
