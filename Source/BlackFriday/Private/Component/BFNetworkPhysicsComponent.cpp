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

	// 초기화
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
	DOREPLIFETIME_CONDITION(UBFNetworkPhysicsComponent, RepStateOwner, COND_None);
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
				LastSentInput = Input;
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

void UBFNetworkPhysicsComponent::ApplyOwnerReconcile(float DeltaTime)
{
	if (!Prim || !bHasOwnerState || !Prim->IsSimulatingPhysics()) return;

	const FVector ServerPos = (FVector)LastOwnerState.Pos;
	const FVector ServerVel = (FVector)LastOwnerState.LinVel;
	const FVector LocalPos = Prim->GetComponentLocation();
	const FVector LocalVel = Prim->GetPhysicsLinearVelocity();

	const float Dist = FVector::Dist(ServerPos, LocalPos);
	
	if (LocalVel.SizeSquared() < 10.f && Dist < 100.0f) 
	{
		if (ServerVel.SizeSquared() > 100.f)
		{
			return;
		}
	}
	
	if (Dist < 30.0f) return;

	if (Dist > TeleportDist)
	{
		Prim->SetWorldLocation(ServerPos, false, nullptr, ETeleportType::TeleportPhysics);
		Prim->SetPhysicsLinearVelocity(ServerVel);
		return;
	}

	FVector FixVel = (ServerPos - LocalPos) * OwnerPosCorrectGain;
	FVector NewVel = FMath::VInterpTo(LocalVel, ServerVel + FixVel, DeltaTime, 5.0f);
	Prim->SetPhysicsLinearVelocity(NewVel);
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

// ApplyRemoteSmoothing 구현 (Proxy용)
void UBFNetworkPhysicsComponent::ApplyRemoteSmoothing(float DeltaTime)
{
	if (!Prim) return;

	if (RemoteInterpSpeed > 0.f)
	{
		float TimeDiff = TargetState.ServerTime - PrevState.ServerTime;
		if (TimeDiff < 0.001f) TimeDiff = 0.033f;

		float CalculatedSpeed = 1.0f / TimeDiff;
		float FinalSpeed = FMath::Clamp(CalculatedSpeed, RemoteInterpSpeed * 0.8f, RemoteInterpSpeed * 1.5f);
		SmoothAlpha += DeltaTime * FinalSpeed;
	}
	else
	{
		SmoothAlpha = 1.f;
	}

	// [잔걸음/떨림 방지] 목표 지점에 거의 도착했으면 고정
	float DistSq = FVector::DistSquared(Prim->GetComponentLocation(), (FVector)TargetState.Pos);
	if (DistSq < 1.0f) // 1cm
	{
		Prim->SetWorldLocationAndRotation((FVector)TargetState.Pos, TargetState.Rot, false, nullptr, ETeleportType::TeleportPhysics);
		Prim->SetPhysicsLinearVelocity(FVector::ZeroVector); // 속도 제거
		if (MoveComp) MoveComp->SetCurrentInput(FBFMoveInputNet()); // 입력도 초기화 느낌으로
		
		SmoothAlpha = 1.0f;
		return;
	}

	if (SmoothAlpha >= 1.f)
	{
		FVector TargetPos = (FVector)TargetState.Pos;
		FRotator TargetRot = TargetState.Rot;

		FVector NewPos = FMath::VInterpTo(Prim->GetComponentLocation(), TargetPos, DeltaTime, VelLerpSpeed);
		FRotator NewRot = FMath::RInterpTo(Prim->GetComponentRotation(), TargetRot, DeltaTime, VelLerpSpeed);

		Prim->SetWorldLocationAndRotation(NewPos, NewRot, false, nullptr, ETeleportType::TeleportPhysics);
		Prim->SetPhysicsLinearVelocity((FVector)TargetState.LinVel);
	}
	else
	{
		FVector StartPos = (FVector)PrevState.Pos;
		FVector EndPos = (FVector)TargetState.Pos;
		FVector NewPos = FMath::Lerp(StartPos, EndPos, SmoothAlpha);

		FRotator StartRot = PrevState.Rot;
		FRotator EndRot = TargetState.Rot;
		FRotator NewRot = FMath::Lerp(StartRot, EndRot, SmoothAlpha);

		Prim->SetWorldLocationAndRotation(NewPos, NewRot, false, nullptr, ETeleportType::TeleportPhysics);
		
		// 보간 중 속도 처리
		FVector LerpVel = FMath::Lerp((FVector)PrevState.LinVel, (FVector)TargetState.LinVel, SmoothAlpha);
		Prim->SetPhysicsLinearVelocity(LerpVel);
	}
	
	// 애니메이션용 로컬 입력 시뮬레이션 (선택적)
	// APawn* P = CachedPawn;
	// if (P && bDriveOwnerAnimFromLocalInputWhenA && !P->IsLocallyControlled()) ...
	// (필요 시 복구, 일단 핵심 로직은 위에서 끝남)
}

void UBFNetworkPhysicsComponent::GetLastServerStateBP(FVector& OutPos, FRotator& OutRot, FVector& OutLinVel, FVector& OutAngVel, float& OutTime) const
{
	OutPos = (FVector)RepState.Pos;
	OutRot = RepState.Rot;
	OutLinVel = (FVector)RepState.LinVel;
	OutAngVel = (FVector)RepState.AngVelDeg;
	OutTime = RepState.ServerTime;
}