// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/Cart/BFCartNetActor.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"

ABFCartNetActor::ABFCartNetActor()
{
	PrimaryActorTick.bCanEverTick = true;

	CartMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CartMesh"));
	RootComponent = CartMesh;

	CartMesh->SetSimulatePhysics(true);
	CartMesh->SetCollisionProfileName(TEXT("PhysicsActor"));

	bReplicates = true;
	NetUpdateFrequency = 100.0f;
	MinNetUpdateFrequency = 30.0f;
	NetPriority = 3.0f;
	
	SetReplicateMovement(false);

	InterpSpeed = 15.0f;
	TeleportThreshold = 500.0f;
}

void ABFCartNetActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		ServerTransform = GetActorTransform();
	}
	TargetTransform = GetActorTransform();
}

void ABFCartNetActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABFCartNetActor, ServerTransform);
}

void ABFCartNetActor::OnRep_ServerTransform()
{
	TargetTransform = ServerTransform;
}

void ABFCartNetActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		FVector CurrentVelocity = CartMesh->GetPhysicsLinearVelocity();
		float CurrentSpeed = CurrentVelocity.Size();

		if (CurrentSpeed > MaxSpeedLimit)
		{
			FVector ClampedVelocity = CurrentVelocity.GetSafeNormal() * MaxSpeedLimit;
			CartMesh->SetPhysicsLinearVelocity(ClampedVelocity);
		}
		
		ServerTransform = GetActorTransform();
		return;
	}

	FVector CurrentLoc = GetActorLocation();
	FQuat CurrentRot = GetActorQuat();

	FVector TargetLoc = TargetTransform.GetLocation();
	FQuat TargetRot = TargetTransform.GetRotation();

	float DistSq = FVector::DistSquared(CurrentLoc, TargetLoc);
	if (DistSq > (TeleportThreshold * TeleportThreshold))
	{
		SetActorLocationAndRotation(TargetLoc, TargetRot);
	}
	else
	{
		FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc, DeltaTime, InterpSpeed);
		FQuat NewRot = FMath::QInterpTo(CurrentRot, TargetRot, DeltaTime, InterpSpeed);
		SetActorLocationAndRotation(NewLoc, NewRot);
	}
}



