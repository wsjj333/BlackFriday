#include "Component/Cart/BFCartResetComponent.h"

// Engine
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"

UBFCartResetComponent::UBFCartResetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UBoxComponent* UBFCartResetComponent::GetRootComp() const
{
	const AActor* Owner = GetOwner();
	return Owner ? Cast<UBoxComponent>(Owner->GetRootComponent()) : nullptr;
}

bool UBFCartResetComponent::CanRequestReset() const
{
	const APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (!PawnOwner) return false;
	return PawnOwner->IsLocallyControlled() || PawnOwner->HasAuthority();
}

bool UBFCartResetComponent::IsUprightEnough() const
{
	const AActor* Owner = GetOwner();
	if (!Owner) return true;

	const float Dot = FVector::DotProduct(Owner->GetActorUpVector(), FVector::UpVector);
	return Dot >= UprightDotThreshold;
}

void UBFCartResetComponent::RequestUpright()
{
	if (!CanRequestReset()) return;
	
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// 서버라면 바로 실행
	if (Owner->HasAuthority())
	{
		DoUprightReset_ServerAuth();
		return;
	}

	// 클라이언트라면 "내가 조종 중인 PC"를 넘겨서 서버 검증에 사용
	const APawn* PawnOwner = Cast<APawn>(Owner);
	APlayerController* PC = PawnOwner ? Cast<APlayerController>(PawnOwner->GetController()) : nullptr;
	
	if (!PC) return;

	Server_RequestUpright(PC);
}

void UBFCartResetComponent::Server_RequestUpright_Implementation(APlayerController* RequestingPC)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) return;

	APawn* PawnOwner = Cast<APawn>(Owner);
	if (!PawnOwner) return;

	// 요청한 PC가 실제로 이 Pawn을 조종하는 Controller인지 확인
	if (!RequestingPC || PawnOwner->GetController() != RequestingPC)
	{
		return; // 불일치면 무시
	}
	
	DoUprightReset_ServerAuth();
}

void UBFCartResetComponent::DoUprightReset_ServerAuth()
{
	// 정상 상태인지 확인
	if (IsUprightEnough()) return;
	
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UBoxComponent* RootComp = GetRootComp();
	if (!RootComp) return;

	// 리셋 스팸 방지용 쿨다운 체크
	const double Now = World->GetTimeSeconds();
	if (LastResetTimeSeconds > 0.0 && (Now - LastResetTimeSeconds) < ResetCooldown) return;
	
	// 플레이어가 달리는 중 리셋 버튼 누르는 것 방지
	const bool bSim = RootComp->IsSimulatingPhysics();
	const FVector LinVel = bSim ? RootComp->GetPhysicsLinearVelocity() : FVector::ZeroVector;
	if (LinVel.Size() > MaxSpeedToAllowReset) return;

	// 카트 아래의 지면을 찾는다
	const FVector Start = Owner->GetActorLocation() + FVector::UpVector * TraceUpDistance;
	const FVector End   = Owner->GetActorLocation() - FVector::UpVector * TraceDownDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CartResetTrace), false, Owner);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);

	// 바닥 노말 벡터를 찾는다
	const FVector GroundNormal = bHit ? Hit.ImpactNormal.GetSafeNormal() : FVector::UpVector;
	// 기준이 되는 상방 벡터 결정
	const FVector Up = bAlignToGroundNormal ? GroundNormal : FVector::UpVector;

	// 전방 벡터를 상방 벡터에 수직인 평면으로 투영
	FVector Forward = Owner->GetActorForwardVector();
	Forward = FVector::VectorPlaneProject(Forward, Up).GetSafeNormal();
	// 전방 벡터가 상방 벡터와 평행한 경우 영 벡터가 될 수 있으므로 체크함
	if (Forward.IsNearlyZero())
	{
		Forward = FVector::VectorPlaneProject(FVector::ForwardVector, Up).GetSafeNormal();
	}

	// 앞서 만든 두 벡터에 수직인 임시 벡터 생성
	const FVector Right = FVector::CrossProduct(Up, Forward).GetSafeNormal();
	// 외적 과정에서 부동 소수점으로 인해 생길 수 있는 오차를 제거하여 정확한 전방 벡터 재생성
	const FVector OrthoForward = FVector::CrossProduct(Right, Up).GetSafeNormal();
	// 목표 회전 생성
	const FRotator TargetRot = FRotationMatrix::MakeFromXZ(OrthoForward, Up).Rotator();

	// 땅에 박히지 않도록 살짝 띄움
	// Lift: 카트 높이의 절반 + 여유 거리(바퀴 높이 및 여유 거리 고려한 값)
	// 바닥을 찾은 경우 위쪽 방향으로 Lift만큼 올려주고, 아닐 경우(fallback) 현재 위치에서 위로 Lift만큼 올려줌
	const float Lift = RootComp->Bounds.BoxExtent.Z + ExtraLift;
	const FVector TargetLoc = bHit ? (Hit.ImpactPoint + Up * Lift) : (Owner->GetActorLocation() + Up * Lift);
	
	if (bSim)
	{
		// 속도 유지하면 다시 넘어질 수 있으므로 속도 초기화
		RootComp->SetPhysicsLinearVelocity(FVector::ZeroVector, false);
		RootComp->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector, false);

		RootComp->SetWorldLocationAndRotation(TargetLoc, TargetRot, false, nullptr, ETeleportType::TeleportPhysics);
		RootComp->WakeAllRigidBodies();
	}
	else
	{
		Owner->SetActorLocationAndRotation(TargetLoc, TargetRot, false, nullptr, ETeleportType::TeleportPhysics);
	}

	// 쿨타임 갱신
	LastResetTimeSeconds = Now;
}