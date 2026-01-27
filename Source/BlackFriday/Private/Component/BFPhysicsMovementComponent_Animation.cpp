#include "Component/BFPhysicsMovementComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "Components/SkeletalMeshComponent.h"

void UBFPhysicsMovementComponent::SetupUpperBodyPhysics()
{
	if (!bEnableUpperBodyPhysics || !CachedMesh || !GetOwner())
	{
		return;
	}

	// PhysicalAnimationComponent 생성
	PhysicalAnimationComp = NewObject<UPhysicalAnimationComponent>(GetOwner(), TEXT("PhysicalAnimationComp"));
	if (!PhysicalAnimationComp)
	{
		return;
	}

	PhysicalAnimationComp->RegisterComponent();
	PhysicalAnimationComp->SetSkeletalMeshComponent(CachedMesh);

	// Physical Animation 설정
	FPhysicalAnimationData AnimData;
	AnimData.bIsLocalSimulation = true;
	AnimData.OrientationStrength = OrientationStrength;
	AnimData.AngularVelocityStrength = AngularVelocityStrength;
	AnimData.PositionStrength = PositionStrength;
	AnimData.VelocityStrength = VelocityStrength;

	// 지정된 본 아래로 Physical Animation 적용
	PhysicalAnimationComp->ApplyPhysicalAnimationSettingsBelow(UpperBodyBoneName, AnimData);

	// 상체 본들만 물리 시뮬레이션 활성화
	CachedMesh->SetAllBodiesBelowSimulatePhysics(UpperBodyBoneName, true, true);
}
