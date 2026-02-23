#include "Character/Pusher/BFPusher.h"
#include "Components/CapsuleComponent.h"
#include "Component/BFPhysicsMovementComponent.h"
#include "Component/BFNetworkPhysicsComponent.h"
#include "Vehicle/Cart/BFCartMovementComponent.h"
#include "Character/Common/BFCharacterAnimInstance.h"
#include "Character/Common/BFTeamComponent.h"
#include "Character/Pusher/Components/BFCharacterAppearanceComponent.h"
#include "Character/Pusher/Components/BFPusherDriveComponent.h"
#include "Character/Pusher/Components/BFPusherInputComponent.h"
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
}

bool ABFPusher::IsDriving() const
{
	return PusherDriveComp ? PusherDriveComp->IsDriving() : false;
}

ABFCartPawn* ABFPusher::GetCart() const
{
	return PusherDriveComp ? PusherDriveComp->GetCart() : nullptr;
}

void ABFPusher::SetCart(ABFCartPawn* NewCart) const
{
	if (PusherDriveComp) PusherDriveComp->SetCart(NewCart);
}

void ABFPusher::BeginPlay()
{
	Super::BeginPlay();
	
	if (PusherInputComp && NetPhysicsComp)
	{
		PusherInputComp->SetInputSink(TScriptInterface<IBFInputSink>(NetPhysicsComp));
	}

	RefreshAnimInstanceCache();
}

void ABFPusher::RefreshAnimInstanceCache()
{
	if (const USkeletalMeshComponent* MeshComp = GetMesh())
	{	
		UAnimInstance* Current = MeshComp->GetAnimInstance();
		if (CachedAnimInstance != Current)
		{
			CachedAnimInstance = Cast<UBFCharacterAnimInstance>(Current);
		}
	}
}

void ABFPusher::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsDriving() || !GetCart() || !CachedAnimInstance)
	{
		return;
	}
	
	const USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return;

	// IK: 핸들 위치 계산
	const FTransform HandleL_WS = GetCart()->GetHandleLTransform();
	const FTransform HandleR_WS = GetCart()->GetHandleRTransform();

	const FTransform HandleL_CS = HandleL_WS.GetRelativeTransform(MeshComp->GetComponentTransform());
	const FTransform HandleR_CS = HandleR_WS.GetRelativeTransform(MeshComp->GetComponentTransform());

	CachedAnimInstance->HandleTargetL_CS = HandleL_CS;
	CachedAnimInstance->HandleTargetR_CS = HandleR_CS;
}

void ABFPusher::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!IsLocallyControlled())
	{
		return;
	}

	if (PusherInputComp)
	{
		PusherInputComp->BindInput(PlayerInputComponent);
	}
}

void ABFPusher::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void ABFPusher::SetPhysicsEnabled(bool bEnabled) const
{
	// bEnabled = true;
	if (CapsuleComp)
	{
		CapsuleComp->SetSimulatePhysics(bEnabled);
		if (bEnabled)
		{
			CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			if (CapsuleComp->GetBodyInstance())
			{
				CapsuleComp->GetBodyInstance()->WakeInstance();
			}
		}
		else
		{
			CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}

	// NetPhysicsComp가 매 틱마다 물리를 다시 활성화하는 것을 방지
	if (NetPhysicsComp)
	{
		NetPhysicsComp->SetComponentTickEnabled(bEnabled);
	}

	if (PhysicsMoveComp)
	{
		PhysicsMoveComp->SetComponentTickEnabled(bEnabled);
	}
}

void ABFPusher::AdjustActorLocationByZOffset()
{
	// 캡슐 중심 기준이므로 HalfHeight만큼 위로 오프셋
	if (CapsuleComp)
	{
		const float HalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();
		SetActorRelativeLocation(FVector(0.f, 0.f, HalfHeight - 10));
	}
}