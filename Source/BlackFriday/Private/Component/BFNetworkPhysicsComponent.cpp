// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/BFNetworkPhysicsComponent.h"
#include "Component/BFPhysicsMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"

UBFNetworkPhysicsComponent::UBFNetworkPhysicsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	SetIsReplicatedByDefault(true);
}

void UBFNetworkPhysicsComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheRefs();

	// 초기 상태
	PrevState = TargetState = RepState;
	SmoothAlpha = 1.f;
}

void UBFNetworkPhysicsComponent::CacheRefs()
{
	if (!MoveComp)
	{
		MoveComp = GetOwner() ? GetOwner()->FindComponentByClass<UBFPhysicsMovementComponent>() : nullptr;
	}

	if (!Prim)
	{
		if (PhysicsPrimitiveOverride)
		{
			Prim = PhysicsPrimitiveOverride;
		}
		else if (AActor* Owner = GetOwner())
		{
			Prim = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
		}
	}
}

void UBFNetworkPhysicsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBFNetworkPhysicsComponent, RepState);
}

void UBFNetworkPhysicsComponent::SetMoveInput(FVector2D Move)
{
	LocalMove.X = FMath::Clamp(Move.X, -1.f, 1.f);
	LocalMove.Y = FMath::Clamp(Move.Y, -1.f, 1.f);
}

void UBFNetworkPhysicsComponent::SetControlYawDegrees(float YawDegrees)
{
	LocalYaw = YawDegrees;
}

void UBFNetworkPhysicsComponent::SetJumpHeld(bool bHeld)
{
	bLocalJumpHeld = bHeld;
}

void UBFNetworkPhysicsComponent::GetLastServerStateBP(FVector& Loc, FRotator& OutRot, FVector& OutLinVel, FVector& OutAngVelDeg, float& OutServerTime) const
{
	Loc = FVector(RepState.Pos);
	OutRot = RepState.Rot;
	OutLinVel = FVector(RepState.LinVel);
	OutAngVelDeg = FVector(RepState.AngVelDeg);
	OutServerTime = RepState.ServerTime;
}

FBFMoveInputNet UBFNetworkPhysicsComponent::BuildInputPacket() const
{
	FBFMoveInputNet P;
	P.MoveX = BF_PackAxis(LocalMove.X);
	P.MoveY = BF_PackAxis(LocalMove.Y);

	P.Buttons = 0;
	if (bLocalJumpHeld) P.Buttons |= 0x01;

	P.ControlYaw100 = BF_PackYaw100(LocalYaw);
	P.ClientFrame = ClientFrameCounter;

	return P;
}

FBFPhysicsState UBFNetworkPhysicsComponent::BuildState() const
{
	FBFPhysicsState S;
	if (!Prim) return S;

	S.Pos = Prim->GetComponentLocation();
	S.Rot = Prim->GetComponentRotation();
	S.LinVel = Prim->GetPhysicsLinearVelocity();
	S.AngVelDeg = Prim->GetPhysicsAngularVelocityInDegrees();
	S.ServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	return S;
}

void UBFNetworkPhysicsComponent::UpdateProxyPhysicsMode()
{
	APawn* P = Cast<APawn>(GetOwner());
	if (!P || !Prim) return;

	// 원격 프록시(클라에서 로컬 컨트롤 아닌 Pawn)는 물리 OFF로 “키네마틱” 처리
	const bool bShouldDisablePhysics = (!P->HasAuthority() && !P->IsLocallyControlled());
	if (bShouldDisablePhysics != bAppliedProxyPhysicsOff)
	{
		bAppliedProxyPhysicsOff = bShouldDisablePhysics;

		if (bAppliedProxyPhysicsOff)
		{
			Prim->SetSimulatePhysics(false);
		}
		else
		{
			// 로컬 컨트롤/서버는 물리 ON (이미 켜져있으면 그대로)
			Prim->SetSimulatePhysics(true);
		}
	}
}

void UBFNetworkPhysicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CacheRefs();
	if (!Prim) return;

	UpdateProxyPhysicsMode();

	APawn* P = Cast<APawn>(GetOwner());
	if (!P) return;

	// 로컬 입력 -> 로컬 예측(손맛)
	if (!P->HasAuthority() && P->IsLocallyControlled())
	{
		ClientFrameCounter++;

		const FBFMoveInputNet Input = BuildInputPacket();

		// 로컬 예측용으로 MoveComp에 입력 반영
		if (MoveComp)
		{
			MoveComp->SetCurrentInput(Input);
		}

		// 일정 Hz로 서버에 입력 송신
		InputSendAccum += DeltaTime;
		const float SendInterval = (InputSendHz > 1.f) ? (1.f / InputSendHz) : 0.f;

		if (SendInterval > 0.f && InputSendAccum >= SendInterval)
		{
			InputSendAccum = 0.f;

			// 아주 간단한 변화 감지(필요하면 제거)
			if (Input.MoveX != LastSentInput.MoveX ||
				Input.MoveY != LastSentInput.MoveY ||
				Input.Buttons != LastSentInput.Buttons ||
				Input.ControlYaw100 != LastSentInput.ControlYaw100)
			{
				LastSentInput = Input;
				ServerReceiveInput(Input);
			}
		}
	}

	// 서버: 권한 물리 시뮬 + 상태 송신
	if (P->HasAuthority())
	{
		// 서버에서 MoveComp가 이 입력으로 Tick에서 힘을 적용하게 함
		if (MoveComp)
		{
			MoveComp->SetCurrentInput(ServerInput);
		}

		StateSendAccum += DeltaTime;
		const float StateInterval = (StateSendHz > 1.f) ? (1.f / StateSendHz) : 0.f;

		if (StateInterval > 0.f && StateSendAccum >= StateInterval)
		{
			StateSendAccum = 0.f;
			RepState = BuildState(); // ReplicatedUsing -> 클라에서 OnRep 호출
		}
	}

	// 원격 프록시: 보간
	if (!P->HasAuthority() && !P->IsLocallyControlled())
	{
		ApplyRemoteSmoothing(DeltaTime);
	}
}

void UBFNetworkPhysicsComponent::ServerReceiveInput_Implementation(FBFMoveInputNet InInput)
{
	// 서버에서 입력값 클램프(안전장치)
	const float X = BF_UnpackAxis(InInput.MoveX);
	const float Y = BF_UnpackAxis(InInput.MoveY);
	InInput.MoveX = BF_PackAxis(X);
	InInput.MoveY = BF_PackAxis(Y);

	ServerInput = InInput;
}

void UBFNetworkPhysicsComponent::OnRep_PhysicsState()
{
	APawn* P = Cast<APawn>(GetOwner());
	if (!P || !Prim) return;

	if (P->IsLocallyControlled())
	{
		// 로컬 보정
		ApplyLocalCorrection(RepState);
	}
	else
	{
		// 원격 보간 타겟 갱신
		PrevState = TargetState;
		TargetState = RepState;
		SmoothAlpha = 0.f;
	}
}

void UBFNetworkPhysicsComponent::ApplyLocalCorrection(const FBFPhysicsState& S)
{
	if (!Prim || !Prim->IsSimulatingPhysics()) return;

	const FVector CurPos = Prim->GetComponentLocation();
	const FVector Err = (FVector)S.Pos - CurPos;
	const float Dist = Err.Size();

	if (Dist > TeleportDist)
	{
		Prim->SetWorldLocationAndRotation(S.Pos, S.Rot, false, nullptr, ETeleportType::TeleportPhysics);
		Prim->SetPhysicsLinearVelocity(S.LinVel);
		Prim->SetPhysicsAngularVelocityInDegrees(S.AngVelDeg);
		return;
	}

	const FVector CurVel = Prim->GetPhysicsLinearVelocity();
	const FVector DesiredVel = S.LinVel;

	// 위치 오차를 “추가 속도”로 바꿔서 자연스럽게 끌어당김
	const FVector PosFixVel = Err * VelCorrectGain;
	const FVector TargetVel = DesiredVel + PosFixVel;

	const FVector NewVel = FMath::VInterpTo(CurVel, TargetVel, 1.f/60.f, VelLerpSpeed);
	Prim->SetPhysicsLinearVelocity(NewVel, false);

	// 회전은 v0에서는 강제 보정 최소화
}

void UBFNetworkPhysicsComponent::ApplyRemoteSmoothing(float DeltaTime)
{
	if (!Prim) return;

	// TargetState가 없을 때 대비
	if (SmoothAlpha >= 1.f)
	{
		Prim->SetWorldLocationAndRotation(TargetState.Pos, TargetState.Rot, false, nullptr, ETeleportType::None);
		return;
	}

	// 시간 기반 보간(지금은 단순 alpha 증가)
	SmoothAlpha = FMath::Min(1.f, SmoothAlpha + DeltaTime * RemoteInterpSpeed);

	const FVector Pos = FMath::Lerp((FVector)PrevState.Pos, (FVector)TargetState.Pos, SmoothAlpha);
	const FRotator Rot = FMath::Lerp(PrevState.Rot, TargetState.Rot, SmoothAlpha);

	Prim->SetWorldLocationAndRotation(Pos, Rot, false, nullptr, ETeleportType::None);
}