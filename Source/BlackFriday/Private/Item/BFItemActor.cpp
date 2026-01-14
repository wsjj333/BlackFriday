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
	
	SetReplicateMovement(true);

	MaxLinearVelocity = 2500.0f;  // 너무 빠르지 않게
	MaxAngularVelocity = 720.0f;
	LinearDamping = 0.5f;   // 굴러다님 방지
	AngularDamping = 1.0f;  // 팽이처럼 도는 것 방지
	}

void ABFItemActor::BeginPlay()
{
	Super::BeginPlay();

	if (ItemMesh)
	{
		ItemMesh->SetLinearDamping(LinearDamping);
		ItemMesh->SetAngularDamping(AngularDamping);
	}
}

void ABFItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && ItemMesh && ItemMesh->IsSimulatingPhysics())
	{
		FVector Vel = ItemMesh->GetPhysicsLinearVelocity();
		float CurrentSpeedSq = Vel.SizeSquared();
		float MaxSpeedSq = MaxLinearVelocity * MaxLinearVelocity;

		if (CurrentSpeedSq > MaxSpeedSq)
		{
			ItemMesh->SetPhysicsLinearVelocity(Vel.GetSafeNormal() * MaxLinearVelocity);
		}

		FVector AngVel = ItemMesh->GetPhysicsAngularVelocityInDegrees();
		if (AngVel.SizeSquared() > (MaxAngularVelocity * MaxAngularVelocity))
		{
			ItemMesh->SetPhysicsAngularVelocityInDegrees(AngVel.GetSafeNormal() * MaxAngularVelocity);
		}
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

		// AttachToComponent(Cast<ACharacter>(Parent)->GetMesh(), AttachRules, SocketName);
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
		TimerDel.BindUObject(this, &ABFItemActor::RestoreCollision,Thrower);
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

