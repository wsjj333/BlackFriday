#include "Character/Pusher/BFPusher.h"


// Engine
#include "Components/CapsuleComponent.h"

// Character
#include "Character/Common/BFCharacterAnimInstance.h"
#include "Character/Common/BFTeamComponent.h"

// Components
#include "Component/BFNetworkPhysicsComponent.h"
#include "Component/BFPhysicsMovementComponent.h"
#include "Component/Common/BFCharacterAppearanceComponent.h"
#include "Component/Pusher/BFCartOverlapDetectorComponent.h"
#include "Component/Pusher/BFPusherDriveComponent.h"
#include "Component/Pusher/BFPusherInputComponent.h"

// Vehicle
#include "Vehicle/Cart/BFCartMovementComponent.h"
#include "Vehicle/Cart/BFCartPawn.h"

ABFPusher::ABFPusher()
{
	// 물리 이동 컴포넌트
	PhysicsMoveComp = CreateDefaultSubobject<UBFPhysicsMovementComponent>(TEXT("PhysicsMoveComp"));

	// 네트워크 물리 동기화 컴포넌트
	NetPhysicsComp = CreateDefaultSubobject<UBFNetworkPhysicsComponent>(TEXT("NetPhysicsComp"));

	// 카트 조종 컴포넌트
	CartDrivingComp = CreateDefaultSubobject<UBFCartMovementComponent>(TEXT("CartDrivingComp"));
	
	// 푸셔 조종 컴포넌트
	PusherInputComp = CreateDefaultSubobject<UBFPusherInputComponent>(TEXT("PusherInputComp"));
	
	// 운전 관련 컴포넌트
	PusherDriveComp = CreateDefaultSubobject<UBFPusherDriveComponent>(TEXT("PusherDriveComp"));
	
	// 외형 컴포넌트
	AppearanceComp = CreateDefaultSubobject<UBFCharacterAppearanceComponent>(TEXT("AppearanceComp"));
	
	// 팀 컴포넌트
	TeamComp = CreateDefaultSubobject<UBFTeamComponent>(TEXT("TeamComp"));
	
	// 카트 오버랩 처리 컴포넌트
	CartOverlapComp = CreateDefaultSubobject<UBFCartOverlapDetectorComponent>(TEXT("CartOverlapComp"));
}

bool ABFPusher::IsDriving() const
{
	return PusherDriveComp && PusherDriveComp->IsDriving();
}

bool ABFPusher::IsOverlappingCart() const
{
	return CartOverlapComp && CartOverlapComp->IsOverlappingCart();
}

ABFCartPawn* ABFPusher::GetCart() const
{
	return PusherDriveComp ? PusherDriveComp->GetCart() : nullptr;
}

void ABFPusher::SetCart(ABFCartPawn* NewCart)
{
	if (!PusherDriveComp) return;
	
	PusherDriveComp->SetCart(NewCart);
}

void ABFPusher::BeginPlay()
{
	Super::BeginPlay();
	
	if (PusherInputComp && NetPhysicsComp)
	{
		PusherInputComp->SetInputSink(TScriptInterface<IBFInputSink>(NetPhysicsComp));
	}

	RefreshAnimInstanceCache();
	
	// 카메라 가림 방지(실루엣)용 Custom Depth 활성화
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRenderCustomDepth(true);
	}
}

void ABFPusher::RefreshAnimInstanceCache()
{
	const USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return;
	
	UAnimInstance* CurrentAnimInstance = MeshComp->GetAnimInstance();
	if (!CurrentAnimInstance) return;
	
	if (CachedAnimInstance != CurrentAnimInstance)
	{
		CachedAnimInstance = Cast<UBFCharacterAnimInstance>(CurrentAnimInstance);
	}
}

void ABFPusher::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	ABFCartPawn* Cart = GetCart();
	const USkeletalMeshComponent* MeshComp = GetMesh();
	
	if (!IsDriving()) return;
	if (!Cart) return;
	if (!CachedAnimInstance) return;
	if (!MeshComp) return;

	// IK: 핸들 위치 계산
	const FTransform HandleL_WS = Cart->GetHandleLTransform();
	const FTransform HandleR_WS = Cart->GetHandleRTransform();

	const FTransform HandleL_CS = HandleL_WS.GetRelativeTransform(MeshComp->GetComponentTransform());
	const FTransform HandleR_CS = HandleR_WS.GetRelativeTransform(MeshComp->GetComponentTransform());

	CachedAnimInstance->HandleTargetL_CS = HandleL_CS;
	CachedAnimInstance->HandleTargetR_CS = HandleR_CS;
}

void ABFPusher::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!IsLocallyControlled()) return;
	if (!PusherInputComp) return;
	
	PusherInputComp->BindInput(PlayerInputComponent);
}

void ABFPusher::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ABFPusher::SetPhysicsEnabled(bool bEnabled)
{
	if (!CapsuleComp) return;
	
	if (NetPhysicsComp) NetPhysicsComp->SetComponentTickEnabled(bEnabled);
	if (PhysicsMoveComp) PhysicsMoveComp->SetComponentTickEnabled(bEnabled);
	
	if (CapsuleComp->IsSimulatingPhysics() == bEnabled) return;
	
	if (bEnabled)
	{
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CapsuleComp->SetSimulatePhysics(true);
		
		if (FBodyInstance* BodyInst = CapsuleComp->GetBodyInstance())
		{
			BodyInst->WakeInstance();
		}
	}
	else
	{
		CapsuleComp->SetSimulatePhysics(false);
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		
		CapsuleComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
		CapsuleComp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}
	
	if (NetPhysicsComp) NetPhysicsComp->SetComponentTickEnabled(bEnabled);
	if (PhysicsMoveComp) PhysicsMoveComp->SetComponentTickEnabled(bEnabled);
}

void ABFPusher::AdjustActorLocationByZOffset()
{
	if (!CapsuleComp) return;
	
	constexpr float GroundOffset = 10.f;
	
	// 캡슐 바닥이 기준면에 위치하도록 Z 보정 (penetration 방지용 offset 포함) 
	const float HalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();
	SetActorRelativeLocation(FVector(0.f, 0.f, HalfHeight - GroundOffset));
}