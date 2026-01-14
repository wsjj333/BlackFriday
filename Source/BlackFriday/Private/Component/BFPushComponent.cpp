// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/BFPushComponent.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"

UBFPushComponent::UBFPushComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	PushStrength = 1500.0f;
	MaxForceLimit = 20000.0f;
	bFlattenZ = true;
}


void UBFPushComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->OnActorHit.AddDynamic(this, &UBFPushComponent::OnOwnerHit);
	}
}

void UBFPushComponent::OnOwnerHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!SelfActor || !SelfActor->HasAuthority()) return;

	UPrimitiveComponent* HitComp = Hit.GetComponent();
	if (HitComp && HitComp->IsSimulatingPhysics())
	{
		FVector MyVelocity = SelfActor->GetVelocity();
		FVector OtherVelocity = HitComp->GetPhysicsLinearVelocity();

		FVector TargetForce = (MyVelocity - OtherVelocity) * PushStrength;

		if (bFlattenZ)
		{
			TargetForce.Z = 0.0f;
		}

		TargetForce = TargetForce.GetClampedToMaxSize(MaxForceLimit);

		HitComp->AddForceAtLocation(TargetForce * HitComp->GetMass(), Hit.Location);
	}
}


