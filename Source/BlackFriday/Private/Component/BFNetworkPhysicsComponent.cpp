// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/BFNetworkPhysicsComponent.h"
#include "Component/BFPhysicsMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

UBFNetworkPhysicsComponent::UBFNetworkPhysicsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetIsReplicatedByDefault(true);
}

void UBFNetworkPhysicsComponent::BeginPlay()
{
	Super::BeginPlay();
	CachedPawn = Cast<APawn>(GetOwner());
	CacheRefs();

	PrevState = TargetState = RepState;
	SmoothAlpha = 1.f;

	if (Prim)
	{
		APawn* P = Cast<APawn>(GetOwner());
		const bool bShouldSimulate = (P && (P->HasAuthority() || (P->IsLocallyControlled() && bUseClientPrediction)));

		if (bShouldSimulate)
		{
			Prim->SetCollisionProfileName(TEXT("PhysicsActor"));
			Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Prim->SetSimulatePhysics(true);
			if (Prim->GetBodyInstance()) Prim->GetBodyInstance()->WakeInstance();
		}
		else
		{
			Prim->SetSimulatePhysics(false);
			if (bDisableCollisionWhenNotSimulating)
				Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			else
				Prim->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}
}

void UBFNetworkPhysicsComponent::CacheRefs()
{
	if (!MoveComp)
		MoveComp = GetOwner() ? GetOwner()->FindComponentByClass<UBFPhysicsMovementComponent>() : nullptr;

	if (!Prim)
	{
		if (MoveComp && MoveComp->PhysicsPrimitiveOverride)
			Prim = MoveComp->PhysicsPrimitiveOverride;
		else if (AActor* Owner = GetOwner())
			Prim = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
	}
}

void UBFNetworkPhysicsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UBFNetworkPhysicsComponent, RepState, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UBFNetworkPhysicsComponent, RepStateOwner, COND_OwnerOnly);
}

void UBFNetworkPhysicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CacheRefs();
	if (!Prim || !CachedPawn) return;

	APawn* P = CachedPawn;
	const bool bIsAuthority = P->HasAuthority();
	const bool bIsLocal = P->IsLocallyControlled();
	
	// 물리 시뮬레이션 상태 관리
	const bool bShouldSimulate = bIsAuthority || (bIsLocal && bUseClientPrediction);
	if (bShouldSimulate)
	{
		if (!Prim->IsSimulatingPhysics() || Prim->GetCollisionEnabled() != ECollisionEnabled::QueryAndPhysics)
		{
			Prim->SetCollisionProfileName(TEXT("PhysicsActor"));
			Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Prim->SetSimulatePhysics(true);
			if (Prim->GetBodyInstance()) Prim->GetBodyInstance()->WakeInstance();
		}
	}
	else
	{
		if (Prim->IsSimulatingPhysics()) Prim->SetSimulatePhysics(false);
		
		ECollisionEnabled::Type TargetCol = bDisableCollisionWhenNotSimulating ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly;
		if (Prim->GetCollisionEnabled() != TargetCol) Prim->SetCollisionEnabled(TargetCol);
	}

	if (bIsLocal)
	{
		ClientFrameCounter++;
		FBFMoveInputNet Input = BuildInputPacket();
		
		if (MoveComp) MoveComp->SetCurrentInput(Input);

		if (bIsAuthority)
		{
			ServerInput = Input;
			if (Input.Buttons & 0x01) bJumpHoldLatched = false;
		}
		else
		{
			InputSendAccum += DeltaTime;
			const float SendInterval = (InputSendHz > 1.f) ? (1.f / InputSendHz) : 0.f;
			
			if (InputSendAccum >= SendInterval)
			{
				InputSendAccum = 0.f;
				ServerReceiveInput(Input);
				if (Input.Buttons & 0x01) bJumpHoldLatched = false;
			}
		}
	}

	if (bIsAuthority)
	{
		if (MoveComp) MoveComp->SetCurrentInput(ServerInput);

		OwnerStateSendAccum += DeltaTime;
		ProxyStateSendAccum += DeltaTime;

		const float OwnerInterval = (OwnerStateSendHz > 1.f) ? (1.f / OwnerStateSendHz) : 0.f;
		const float ProxyInterval = (ProxyStateSendHz > 1.f) ? (1.f / ProxyStateSendHz) : 0.f;

		bool bBuilt = false;
		FBFPhysicsState NewState;

		if (OwnerInterval > 0.f && OwnerStateSendAccum >= OwnerInterval)
		{
			OwnerStateSendAccum = 0.f;
			NewState = BuildState();
			bBuilt = true;
			RepStateOwner = NewState;
		}

		if (ProxyInterval > 0.f && ProxyStateSendAccum >= ProxyInterval)
		{
			ProxyStateSendAccum = 0.f;
			if (!bBuilt) { NewState = BuildState(); bBuilt = true; }
			RepState = NewState;
		}
	}

	if (!bIsAuthority && !bIsLocal)
	{
		ApplyRemoteSmoothing(DeltaTime);
	}
	else if (!bIsAuthority && bIsLocal)
	{
		if (!bUseClientPrediction) ApplyRemoteSmoothing(DeltaTime);
		else ApplyOwnerReconcile(DeltaTime);
	}
}

FBFPhysicsState UBFNetworkPhysicsComponent::BuildState() const
{
	FBFPhysicsState S;
	if (Prim)
	{
		S.Pos = Prim->GetComponentLocation();
		S.Rot = Prim->GetComponentRotation();
		S.LinVel = Prim->GetPhysicsLinearVelocity();
		S.AngVelDeg = Prim->GetPhysicsAngularVelocityInDegrees();
	}
	S.ServerTime = GetWorld()->GetTimeSeconds();
	return S;
}

void UBFNetworkPhysicsComponent::GetLastServerStateBP(FVector& OutPos, FRotator& OutRot, FVector& OutLinVel, FVector& OutAngVel, float& OutTime) const
{
	OutPos = (FVector)RepState.Pos;
	OutRot = RepState.Rot;
	OutLinVel = (FVector)RepState.LinVel;
	OutAngVel = (FVector)RepState.AngVelDeg;
	OutTime = RepState.ServerTime;
}

void UBFNetworkPhysicsComponent::SetHighPriorityMode(bool bEnable)
{
	if (APawn* P = Cast<APawn>(GetOwner()))
	{
		if (bEnable)
		{
			// 중요 상황
			P->NetUpdateFrequency = 120.f;
			P->MinNetUpdateFrequency = 90.f;
		}
		else
		{
			// 평상시
			P->NetUpdateFrequency = 30.f;
			P->MinNetUpdateFrequency = 10.f;
		}
	}
}