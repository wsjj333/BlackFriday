// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/BFPushComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UBFPushComponent::UBFPushComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	PushStrength = 800.0f;
	PushRange = 120.0f;
	bFlattenZ = true;
	CurrentIgnoredActor = nullptr;

    PushInterval = 0.05f;
    LastPushTime = 0.0f;
    ServerLastPushTime = 0.0f;
}

void UBFPushComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBFPushComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    AActor* Owner = GetOwner();
    APawn* OwnerPawn = Cast<APawn>(Owner);
    if (!OwnerPawn) return;
    if (!OwnerPawn->IsLocallyControlled()) return;
    
    UPrimitiveComponent* OwnerRoot = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
    if (!OwnerRoot) return;

    FVector Start = Owner->GetActorLocation();
    FVector Forward = Owner->GetActorForwardVector();
    FVector End = Start + (Forward * PushRange);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);

    FVector BoxHalfSize = FVector(10.0f, 40.0f, 80.0f); 
    FCollisionShape BoxShape = FCollisionShape::MakeBox(BoxHalfSize);

    bool bHit = GetWorld()->SweepSingleByChannel(
        Hit,
        Start,
        End,
        Owner->GetActorQuat(),
        ECC_Visibility,
        BoxShape,
        Params
    );

    DrawDebugBox(GetWorld(), Start + (Forward * (PushRange * 0.5f)), BoxHalfSize, Owner->GetActorQuat(), bHit ? FColor::Green : FColor::Red, false, -1.0f, 0, 2.0f);


    UPrimitiveComponent* HitComp = Hit.GetComponent();
    bool bIsPhysicsObject = bHit && HitComp && HitComp->IsSimulatingPhysics();

    if (bIsPhysicsObject)
    {
        AActor* HitActor = Hit.GetActor();
        
        if (CurrentIgnoredActor != HitActor)
        {
            if (CurrentIgnoredActor && IsValid(CurrentIgnoredActor))
            {
                OwnerRoot->IgnoreActorWhenMoving(CurrentIgnoredActor, false);
            }

            CurrentIgnoredActor = HitActor;
            OwnerRoot->IgnoreActorWhenMoving(CurrentIgnoredActor, true);
        }
    
        float CurrentTime = GetWorld()->GetTimeSeconds();
        if (CurrentTime - LastPushTime < PushInterval) return;
        LastPushTime = CurrentTime;
        
        FVector PushDir = Forward;
        if (bFlattenZ) PushDir.Z = 0.0f;
        PushDir.Normalize();

        float MySpeed = Owner->GetVelocity().Size();
        float BoxSpeed = HitComp->GetPhysicsLinearVelocity().Size();

        if (MySpeed > 10.0f && BoxSpeed < (MySpeed * 1.3f))
        {
            FVector FinalImpulse = PushDir * PushStrength * HitComp->GetMass();

            if (Owner->HasAuthority())
            {
                HitComp->AddImpulseAtLocation(FinalImpulse, Hit.Location);
            }
            else if (Owner->GetLocalRole() == ROLE_AutonomousProxy)
            {
                Server_ApplyPush(HitComp, FinalImpulse, Hit.Location);
            }
        }
    }
    else
    {
        if (CurrentIgnoredActor)
        {
            if (IsValid(CurrentIgnoredActor))
            {
                OwnerRoot->IgnoreActorWhenMoving(CurrentIgnoredActor, false);
            }
            CurrentIgnoredActor = nullptr;
        }
    }
}


void UBFPushComponent::Server_ApplyPush_Implementation(UPrimitiveComponent* HitComp, FVector PushForce,
                                                       FVector Location)
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - ServerLastPushTime < PushInterval) return;
    ServerLastPushTime = CurrentTime;
    if (HitComp && HitComp->IsSimulatingPhysics())
    {
        HitComp->AddImpulseAtLocation(PushForce, Location);
    }
}


