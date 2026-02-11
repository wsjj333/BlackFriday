// Fill out your copyright notice in the Description page of Project Settings.

#include "Item/BFItemActor.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"

ABFItemActor::ABFItemActor()
{
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	RootComponent = ItemMesh;

	ItemMesh->SetSimulatePhysics(true);
	ItemMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	ItemMesh->BodyInstance.bUseCCD = true;

	bReplicates = true;
	
	NetUpdateFrequency = 100.0f; 
	MinNetUpdateFrequency = 30.0f;
	NetPriority = 3.0f;
	
	SetReplicateMovement(false);

	MaxLinearVelocity = 2500.0f;
	MaxAngularVelocity = 720.0f;
	LinearDamping = 0.5f;   
	AngularDamping = 1.0f;  

	InterpSpeed = 15.0f;
	TeleportThreshold = 500.0f;
}

void ABFItemActor::BeginPlay()
{
	Super::BeginPlay();

	if (ItemMesh)
	{
		ItemMesh->SetLinearDamping(LinearDamping);
		ItemMesh->SetAngularDamping(AngularDamping);
	}

	if (HasAuthority())
	{
		ServerTransform = GetActorTransform();
	}
	TargetTransform = GetActorTransform();
}

void ABFItemActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABFItemActor, ServerTransform);
}

void ABFItemActor::OnRep_ServerTransform()
{
	float Dist = FVector::Dist(GetActorLocation(), ServerTransform.GetLocation());
	
	float MySpeed = GetVelocity().Size();
	bool bIsMoving = MySpeed > 10.0f;

	if (bIsMoving && Dist < 20.0f)
	{
		return;
	}

	TargetTransform = ServerTransform;
}

void ABFItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		if (ItemMesh && ItemMesh->IsSimulatingPhysics())
		{
			FVector Vel = ItemMesh->GetPhysicsLinearVelocity();
			if (Vel.SizeSquared() > FMath::Square(MaxLinearVelocity))
			{
				ItemMesh->SetPhysicsLinearVelocity(Vel.GetSafeNormal() * MaxLinearVelocity);
			}

			FVector AngVel = ItemMesh->GetPhysicsAngularVelocityInDegrees();
			if (AngVel.SizeSquared() > FMath::Square(MaxAngularVelocity))
			{
				ItemMesh->SetPhysicsAngularVelocityInDegrees(AngVel.GetSafeNormal() * MaxAngularVelocity);
			}
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
		
		FQuat NewRot = FMath::QInterpTo(CurrentRot, TargetRot, DeltaTime, InterpSpeed * 0.8f);
		
		SetActorLocationAndRotation(NewLoc, NewRot);
	}
}

void ABFItemActor::PickUp(AActor* Parent, FName Socketname)
{
	if (!HasAuthority()) return;

	if (Parent && ItemMesh)
	{
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
		AttachToComponent(Parent->GetRootComponent(), AttachRules, Socketname);
	}
}

void ABFItemActor::Throw(FVector ThrowVelocity, AActor* Thrower)
{
	if (!HasAuthority()) return;

	if (ItemMesh)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		ItemMesh->SetSimulatePhysics(true);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		if (Thrower)
		{
			ItemMesh->IgnoreActorWhenMoving(Thrower, true);
		}

		ItemMesh->AddImpulse(ThrowVelocity, NAME_None, true);

		FTimerDelegate TimerDel;
		TimerDel.BindUObject(this, &ABFItemActor::RestoreCollision, Thrower);
		GetWorld()->GetTimerManager().SetTimer(CollisionResetTimerHandle, TimerDel, 0.5f, false);
	}
}

void ABFItemActor::RestoreCollision(AActor* Thrower)
{
	if (IsValid(this) && IsValid(Thrower) && ItemMesh)
	{
		ItemMesh->IgnoreActorWhenMoving(Thrower, false);
	}
}