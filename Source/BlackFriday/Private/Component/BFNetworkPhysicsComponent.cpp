// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/BFNetworkPhysicsComponent.h"
#include "Component/BFPhysicsMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

UBFNetworkPhysicsComponent::UBFNetworkPhysicsComponent()
{
	// 매 프레임 Tick 수행
	PrimaryComponentTick.bCanEverTick = true;

	// 물리 시뮬 이전에 Tick (입력 → 물리 → 결과 순서 보장)
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	// 컴포넌트 자체가 네트워크 복제 대상
	SetIsReplicatedByDefault(true);
}

void UBFNetworkPhysicsComponent::BeginPlay()
{
	Super::BeginPlay();
	
	CachedPawn = Cast<APawn>(GetOwner());

	// 이동 컴포넌트 / 물리 프리미티브 참조 캐싱
	CacheRefs();

	// 초기 상태 동기화 (스무딩 시작 시 튐 방지)
	PrevState   = RepState;
	TargetState = RepState;
	SmoothAlpha = 1.f;

	if (Prim)
	{
		APawn* P = Cast<APawn>(GetOwner());

		// 서버이거나 (또는) 로컬 + 클라 예측 사용 시 물리 시뮬 수행
		const bool bShouldSimulate =
			(P && (P->HasAuthority() || (P->IsLocallyControlled() && bUseClientPrediction)));

		if (bShouldSimulate)
		{
			// 실제 물리 시뮬레이션 담당
			Prim->SetCollisionProfileName(TEXT("PhysicsActor"));
			Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Prim->SetSimulatePhysics(true);

			// 수면 상태 방지
			if (Prim->GetBodyInstance())
				Prim->GetBodyInstance()->WakeInstance();
		}
		else
		{
			// 원격 프록시는 물리 끔 (서버 상태 수신 전용)
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
	// 이동 컴포넌트 캐싱
	if (!MoveComp)
		MoveComp = GetOwner()
			? GetOwner()->FindComponentByClass<UBFPhysicsMovementComponent>()
			: nullptr;

	// 물리 프리미티브 결정
	if (!Prim)
	{
		// 이동 컴포넌트에서 지정한 프리미티브 우선
		if (MoveComp && MoveComp->PhysicsPrimitiveOverride)
			Prim = MoveComp->PhysicsPrimitiveOverride;
		// 없으면 RootComponent 사용
		else if (AActor* Owner = GetOwner())
			Prim = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
	}
}

void UBFNetworkPhysicsComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 오너 제외 모든 클라에게 보내는 상태
	DOREPLIFETIME_CONDITION(
		UBFNetworkPhysicsComponent, RepState, COND_SkipOwner);

	// 오너 전용 상태 (보정용)
	DOREPLIFETIME_CONDITION(
		UBFNetworkPhysicsComponent, RepStateOwner, COND_OwnerOnly);
}

void UBFNetworkPhysicsComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CacheRefs();
	if (!Prim || !CachedPawn) return;

	APawn* P = CachedPawn;

	const bool bIsAuthority = P->HasAuthority();        // 서버
	const bool bIsLocal     = P->IsLocallyControlled(); // 로컬 플레이어

	// -------------------------------
	// 물리 시뮬레이션 활성/비활성 관리
	// -------------------------------
	const bool bShouldSimulate =
		bIsAuthority || (bIsLocal && bUseClientPrediction);

	if (bShouldSimulate)
	{
		// 서버 또는 예측 클라는 실제 물리 수행
		if (!Prim->IsSimulatingPhysics() ||
			Prim->GetCollisionEnabled() != ECollisionEnabled::QueryAndPhysics)
		{
			Prim->SetCollisionProfileName(TEXT("PhysicsActor"));
			Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Prim->SetSimulatePhysics(true);

			if (Prim->GetBodyInstance())
				Prim->GetBodyInstance()->WakeInstance();
		}
	}
	else
	{
		// 원격 프록시: 물리 비활성
		if (Prim->IsSimulatingPhysics())
			Prim->SetSimulatePhysics(false);

		ECollisionEnabled::Type TargetCol =
			bDisableCollisionWhenNotSimulating
			? ECollisionEnabled::NoCollision
			: ECollisionEnabled::QueryOnly;

		if (Prim->GetCollisionEnabled() != TargetCol)
			Prim->SetCollisionEnabled(TargetCol);
	}

	// -------------------------------
	// 입력 처리 (로컬만)
	// -------------------------------
	if (bIsLocal)
	{
		ClientFrameCounter++;

		// 입력 패킷 생성
		FBFMoveInputNet Input = BuildInputPacket();

		if (MoveComp)
			MoveComp->SetCurrentInput(Input);

		if (bIsAuthority)
		{
			// 리슨 서버: 서버 입력 직접 반영
			ServerInput = Input;

			// 점프 버튼 래치 해제
			if (Input.Buttons & 0x01)
				bJumpHoldLatched = false;
		}
		else
		{
			// 클라이언트: 일정 주기로 서버에 입력 전송
			InputSendAccum += DeltaTime;

			const float SendInterval =
				(InputSendHz > 1.f) ? (1.f / InputSendHz) : 0.f;

			if (InputSendAccum >= SendInterval)
			{
				InputSendAccum = 0.f;
				ServerReceiveInput(Input);

				if (Input.Buttons & 0x01)
					bJumpHoldLatched = false;
			}
		}
	}

	// -------------------------------
	// 서버: 상태 생성 및 복제
	// -------------------------------
	if (bIsAuthority)
	{
		if (MoveComp)
			MoveComp->SetCurrentInput(ServerInput);

		OwnerStateSendAccum += DeltaTime;
		ProxyStateSendAccum += DeltaTime;

		const float OwnerInterval =
			(OwnerStateSendHz > 1.f) ? (1.f / OwnerStateSendHz) : 0.f;

		const float ProxyInterval =
			(ProxyStateSendHz > 1.f) ? (1.f / ProxyStateSendHz) : 0.f;

		bool bBuilt = false;
		FBFPhysicsState NewState;

		// 오너 전용 상태
		if (OwnerInterval > 0.f && OwnerStateSendAccum >= OwnerInterval)
		{
			OwnerStateSendAccum = 0.f;
			NewState = BuildState();
			bBuilt = true;
			RepStateOwner = NewState;
		}

		// 프록시용 상태
		if (ProxyInterval > 0.f && ProxyStateSendAccum >= ProxyInterval)
		{
			ProxyStateSendAccum = 0.f;
			if (!bBuilt)
			{
				NewState = BuildState();
				bBuilt = true;
			}
			RepState = NewState;
		}
	}

	// -------------------------------
	// 클라이언트 상태 보정
	// -------------------------------
	if (!bIsAuthority && !bIsLocal)
	{
		// 다른 플레이어 (SimulatedProxy)
		ApplyRemoteSmoothing(DeltaTime);
	}
	else if (!bIsAuthority && bIsLocal)
	{
		// 자기 자신 (AutonomousProxy)
		if (!bUseClientPrediction)
			ApplyRemoteSmoothing(DeltaTime);
		else
			ApplyOwnerReconcile(DeltaTime);
	}
}

FBFPhysicsState UBFNetworkPhysicsComponent::BuildState() const
{
	// 서버 기준 물리 상태 스냅샷 생성
	FBFPhysicsState S;

	if (Prim)
	{
		S.Pos        = Prim->GetComponentLocation();
		S.Rot        = Prim->GetComponentRotation();
		S.LinVel     = Prim->GetPhysicsLinearVelocity();
		S.AngVelDeg  = Prim->GetPhysicsAngularVelocityInDegrees();
	}

	// 서버 시간 기록 (보간/보정용)
	S.ServerTime = GetWorld()->GetTimeSeconds();
	return S;
}

FVector UBFNetworkPhysicsComponent::GetMoveInputWorldSpace() const
{
	// 입력이 없으면 바로 0
	if (LocalMove.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	/*
	 * ControlYaw 기준으로 월드 방향 생성
	 * (CMC의 GetControlRotation().Yaw 와 동일 개념)
	 */
	const FRotator ControlRot(0.f, LocalYaw, 0.f);

	const FVector Forward =
		FRotationMatrix(ControlRot).GetUnitAxis(EAxis::X);

	const FVector Right =
		FRotationMatrix(ControlRot).GetUnitAxis(EAxis::Y);

	/*
	 * 입력 해석:
	 * X = Forward
	 * Y = Right
	 */
	FVector MoveWS =
		Forward * LocalMove.X +
		Right   * LocalMove.Y;

	// 대각선 입력 보정 (길이 1 초과 방지)
	return MoveWS.GetClampedToMaxSize(1.f);
}

void UBFNetworkPhysicsComponent::GetLastServerStateBP(
	FVector& OutPos,
	FRotator& OutRot,
	FVector& OutLinVel,
	FVector& OutAngVel,
	float& OutTime) const
{
	// 블루프린트 접근용 서버 상태 반환
	OutPos    = FVector(RepState.Pos);
	OutRot    = RepState.Rot;
	OutLinVel = FVector(RepState.LinVel);
	OutAngVel = FVector(RepState.AngVelDeg);
	OutTime   = RepState.ServerTime;
}

void UBFNetworkPhysicsComponent::SetHighPriorityMode(bool bEnable)
{
	if (APawn* P = Cast<APawn>(GetOwner()))
	{
		if (bEnable)
		{
			// 중요 상황 (전투, 충돌 등)
			P->NetUpdateFrequency    = 120.f;
			P->MinNetUpdateFrequency = 90.f;
		}
		else
		{
			// 평상시
			P->NetUpdateFrequency    = 30.f;
			P->MinNetUpdateFrequency = 10.f;
		}
	}
}
