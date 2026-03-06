#include "Component/Pusher/BFPusherDriveComponent.h"

// Engine
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"

// Character
#include "Character/Common/BFTeamComponent.h"
#include "Character/Pusher/BFPusher.h"

// Vehicle
#include "Vehicle/Cart/BFCartMovementComponent.h"
#include "Vehicle/Cart/BFCartPawn.h"

UBFPusherDriveComponent::UBFPusherDriveComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UBFPusherDriveComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPusher = Cast<ABFPusher>(GetOwner());
	ResolveCartMovementComponent();
}

void UBFPusherDriveComponent::ResolveCartMovementComponent()
{
	const ABFPusher* Pusher = GetPusher();
	if (!Pusher) return;

	// Pusher에 붙어있는 카트 무브먼트 컴포넌트를 찾아 캐시
	CachedCartMovementComp = Pusher->FindComponentByClass<UBFCartMovementComponent>();
}

ABFPusher* UBFPusherDriveComponent::GetPusher() const
{
	return OwnerPusher.Get();
}

UBFCartMovementComponent* UBFPusherDriveComponent::GetCartMovementComponent() const
{
	return CachedCartMovementComp.Get();
}

void UBFPusherDriveComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBFPusherDriveComponent, Cart);
	DOREPLIFETIME(UBFPusherDriveComponent, bIsDriving);
	DOREPLIFETIME(UBFPusherDriveComponent, bOrientToMovement);
}

void UBFPusherDriveComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!bIsDriving) return;
	if (!Cart) return;
	if (!OwnerPusher) return;
	
	const float CartYaw = Cart->GetPusherStandAnchorComponent()->GetComponentRotation().Yaw;

	FRotator NewRot = OwnerPusher->GetActorRotation();
	NewRot.Yaw = CartYaw;

	OwnerPusher->SetActorRotation(NewRot);
}

void UBFPusherDriveComponent::ToggleDrivingMode()
{
	const ABFPusher* Pusher = GetPusher();
	if (!Pusher) return;

	if (!Cart) return;

	// “즉시 체감”이 필요하면 목표 상태를 먼저 로컬에 적용 가능
	const bool bWillDrive = !bIsDriving;

	// 로컬 체감: AutonomousProxy면 즉시 반영(서버 확정은 OnRep로 수렴)
	if (Pusher->IsLocallyControlled())
	{
		ApplyOrientToMovement(!bWillDrive);
	}

	// 서버에 토글 요청
	if (Pusher->HasAuthority())
	{
		Cart->SetOwner(Pusher->GetController());
		ServerToggleDrivingMode(); // 서버도 한 경로로 통일
	}
	else
	{
		// RPC는 “내가 소유한 Pusher(Autonomous)”에서만 가능
		if (Pusher->IsLocallyControlled())
		{
			ServerToggleDrivingMode();
		}
	}
}

void UBFPusherDriveComponent::SetCart(ABFCartPawn* NewCart)
{
	const ABFPusher* Pusher = GetPusher();
	if (!Pusher) return;

	// 서버는 즉시 세팅
	if (Pusher->HasAuthority())
	{
		Cart = NewCart;
		OnRep_Cart(); // 서버도 즉시 반영이 필요하면 직접 호출
		return;
	}
	
	// 클라는 “소유 로컬”에서만 요청
	if (!Pusher->IsLocallyControlled())
	{
		return;
	}
	
	ServerSetCart(NewCart);
}

void UBFPusherDriveComponent::SetOrientToMovement(bool bEnable)
{
	const ABFPusher* Pusher = GetPusher();
	if (!Pusher) return;

	// 로컬 즉시 반영(Autonomous 체감)
	if (Pusher->IsLocallyControlled())
	{
		ApplyOrientToMovement(bEnable);
	}

	// 서버 권한 확정
	if (Pusher->HasAuthority())
	{
		bOrientToMovement = bEnable;
		ApplyOrientToMovement(bEnable);
	}
	else
	{
		if (Pusher->IsLocallyControlled())
		{
			ServerSetOrientToMovement(bEnable);
		}
	}
}

void UBFPusherDriveComponent::ServerToggleDrivingMode_Implementation()
{
	ABFPusher* Pusher = GetPusher();
	if (!Pusher) return;
	
	if (!Cart) return;
	
	UBFTeamComponent* CartTeamComp = Cart->FindComponentByClass<UBFTeamComponent>();
	if (!CartTeamComp) return;
	
	UBFTeamComponent* PusherTeamComp = Pusher->FindComponentByClass<UBFTeamComponent>();
	if (!PusherTeamComp) return;
	
	if(CartTeamComp->GetTeamId() == 0)
	{
		CartTeamComp->SetTeamId(PusherTeamComp->GetTeamId());
	}
	
	if (CartTeamComp->GetTeamId() != PusherTeamComp->GetTeamId())
	{
		return;
	}

	// 서버에서 실제 부착/해제
	ApplyDrivingAttachment(!bIsDriving);

	// 서버 자신도 즉시 로컬 처리
	HandleDrivingStateChanged(!bIsDriving);
	
	bIsDriving = !bIsDriving;
	Cart->ToggleProxyBoxCollision(bIsDriving);
}

void UBFPusherDriveComponent::ServerSetCart_Implementation(ABFCartPawn* NewCart)
{
	Cart = NewCart;
	OnRep_Cart();
}

void UBFPusherDriveComponent::ServerSetOrientToMovement_Implementation(bool bEnable)
{
	bOrientToMovement = bEnable;
	ApplyOrientToMovement(bEnable);
}

void UBFPusherDriveComponent::OnRep_Cart()
{
	ResolveCartMovementComponent();

	if (CachedCartMovementComp)
	{
		CachedCartMovementComp->SetCart(Cart);
	}
}

void UBFPusherDriveComponent::OnRep_IsDriving()
{
	HandleDrivingStateChanged(bIsDriving);
	
	// 서버가 변경한 탑승 상태를 클라이언트 화면에도 즉시 적용하여 덜덜거림 방지
	ApplyDrivingAttachment(bIsDriving);
}

void UBFPusherDriveComponent::OnRep_OrientToMovement()
{
	ApplyOrientToMovement(bOrientToMovement);
}

void UBFPusherDriveComponent::HandleDrivingStateChanged(bool bNowDriving)
{
	// 카트 무브먼트 컴포넌트에 드라이빙 상태 전달
	if (CachedCartMovementComp)
	{
		CachedCartMovementComp->SetDriving(bNowDriving);
	}

	// 드라이빙이면 OrientToMovement를 끄고, 아니면 켜는 정책
	const bool bEnableOrient = !bNowDriving;

	// 서버/클라 모두 “체감”을 위해 즉시 적용
	ApplyOrientToMovement(bEnableOrient);

	// 서버는 bOrientToMovement를 소스로 유지하고 싶으면 여기서도 갱신
	// (정책이 ‘운전 상태에 종속’이라면 bOrientToMovement를 굳이 별도 복제 안 하고 지워도 됨)
	bOrientToMovement = bEnableOrient;
}

void UBFPusherDriveComponent::ApplyOrientToMovement(bool bEnable)
{
	ABFPusher* Pusher = GetPusher();
	if (!Pusher) return;
	
	if (!Cart) return;
	const float CartYaw = Cart->GetPivotComp()->GetComponentRotation().Yaw;

	FRotator NewRot = Pusher->GetActorRotation();
	NewRot.Yaw = CartYaw;

	Pusher->SetActorRotation(NewRot);
}

void UBFPusherDriveComponent::ApplyDrivingAttachment(bool bAttach)
{
	ABFPusher* Pusher = GetPusher();
	if (!Pusher) return;

	if (bAttach)
	{
		if (!Cart) return;

		USceneComponent* StandAnchor = Cart->GetPusherStandAnchorComponent();
		if (!StandAnchor) return;
		
		Pusher->SetActorRotation(StandAnchor->GetComponentRotation());
		
		Pusher->SetPhysicsEnabled(false);

		const FAttachmentTransformRules Rules(
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::KeepWorld,
			true);
		
		Pusher->AttachToComponent(StandAnchor, Rules);
	}
	else
	{
		Pusher->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Pusher->SetPhysicsEnabled(true);
	}
}
