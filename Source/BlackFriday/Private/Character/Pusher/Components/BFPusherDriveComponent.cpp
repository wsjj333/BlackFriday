#include "Character/Pusher/Components/BFPusherDriveComponent.h"

#include "Character/Pusher/BFPusher.h"
#include "Vehicle/Cart/BFCartPawn.h"
#include "Vehicle/Cart/BFCartMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/SceneComponent.h"

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

	// 서버는 소스로 갖고 즉시 반영하고 싶을 때 (선택)
	// 여기서는 별도 처리 불필요. OnRep은 클라에서만 자동이고,
	// 서버는 상태 바꿀 때 직접 HandleDrivingStateChanged/Apply...를 호출하도록 설계.
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
	
	if (!bIsDriving)
	{
		return;
	}
	
	if (Cart && OwnerPusher)
	{
		const float CartYaw = Cart->GetPusherStandAnkerComponent()->GetComponentRotation().Yaw;

		FRotator NewRot = OwnerPusher->GetActorRotation();
		NewRot.Yaw = CartYaw;

		OwnerPusher->SetActorRotation(NewRot);
	}
}

// -------------------- Public API --------------------

void UBFPusherDriveComponent::ToggleDrivingMode()
{
	const ABFPusher* Pusher = GetPusher();
	if (!Pusher) return;

	if (!Cart) return;

	// “즉시 체감”이 필요하면 목표 상태를 먼저 로컬에 적용 가능
	// 단, 최종 권한은 서버가 가진다.
	const bool bWillDrive = !bIsDriving;

	// 로컬 체감: AutonomousProxy면 즉시 반영(서버 확정은 OnRep로 수렴)
	if (Pusher->IsLocallyControlled())
	{
		ApplyOrientToMovement(!bWillDrive);
	}

	// 서버에 토글 요청
	if (Pusher->HasAuthority())
	{
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

void UBFPusherDriveComponent::SetOrientToMovement(const bool bEnable)
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

// -------------------- RPCs --------------------

void UBFPusherDriveComponent::ServerToggleDrivingMode_Implementation()
{
	ABFPusher* Pusher = GetPusher();
	if (!Pusher) return;

	// 서버에서 실제 부착/해제
	ApplyDrivingAttachment_Server(!bIsDriving);

	// 서버 자신도 즉시 로컬 처리(서버도 플레이어일 수 있음)
	HandleDrivingStateChanged(!bIsDriving);
	
	bIsDriving = !bIsDriving;
}

void UBFPusherDriveComponent::ServerSetCart_Implementation(ABFCartPawn* NewCart)
{
	Cart = NewCart;
	OnRep_Cart();
}

void UBFPusherDriveComponent::ServerSetOrientToMovement_Implementation(const bool bEnable)
{
	bOrientToMovement = bEnable;
	ApplyOrientToMovement(bEnable);
}

// -------------------- OnRep --------------------

void UBFPusherDriveComponent::OnRep_Cart()
{
	ResolveCartMovementComponent();

	if (CachedCartMovementComp)
	{
		CachedCartMovementComp->SetCart(Cart);
	}

	// UI/캐시 갱신 같은 로컬 처리도 여기서 하면 됨(필요 시)
}

void UBFPusherDriveComponent::OnRep_IsDriving()
{
	HandleDrivingStateChanged(bIsDriving);

	// 클라이언트는 부착을 서버가 복제해주는 방식으로 유지할 수도 있고,
	// “정확히 붙어있게”를 클라에서도 보장하려면 여기서도 부착을 시도할 수 있음.
	// 다만 네 구조는 서버에서만 Attach/Detach(ApplyDrivingAttachment_Server)라서 여기선 생략.
}

void UBFPusherDriveComponent::OnRep_OrientToMovement()
{
	ApplyOrientToMovement(bOrientToMovement);
}

// -------------------- Apply / State --------------------

void UBFPusherDriveComponent::HandleDrivingStateChanged(const bool bNowDriving)
{
	// 카트 무브먼트 컴포넌트에 드라이빙 상태 전달
	if (CachedCartMovementComp)
	{
		CachedCartMovementComp->SetDriving(bNowDriving);
	}

	// 드라이빙이면 OrientToMovement를 끄고, 아니면 켜는 정책(네 기존 코드 유지)
	const bool bEnableOrient = !bNowDriving;

	// 서버/클라 모두 “체감”을 위해 즉시 적용
	ApplyOrientToMovement(bEnableOrient);

	// 서버는 bOrientToMovement를 소스로 유지하고 싶으면 여기서도 갱신
	// (정책이 ‘운전 상태에 종속’이라면 bOrientToMovement를 굳이 별도 복제 안 하고 지워도 됨)
	bOrientToMovement = bEnableOrient;
}

void UBFPusherDriveComponent::ApplyOrientToMovement(const bool bEnable)
{
	ABFPusher* Pusher = GetPusher();
	if (!Pusher) return;
	
	if (!Cart) return;
	const float CartYaw = Cart->GetPivotComp()->GetComponentRotation().Yaw;

	FRotator NewRot = Pusher->GetActorRotation();
	NewRot.Yaw = CartYaw;

	Pusher->SetActorRotation(NewRot);
}

void UBFPusherDriveComponent::ApplyDrivingAttachment_Server(const bool bAttach)
{
	ABFPusher* Pusher = GetPusher();
	if (!Pusher) return;

	if (!Pusher->HasAuthority())
	{
		return;
	}

	if (bAttach)
	{
		if (!Cart) return;

		USceneComponent* StandAnker = Cart->GetPusherStandAnkerComponent();
		if (!StandAnker) return;
		
		Pusher->SetActorRotation(StandAnker->GetComponentRotation());
		
		Pusher->SetPhysicsEnabled(false);

		const FAttachmentTransformRules Rules(
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,
			EAttachmentRule::KeepWorld,
			true);

		// Pusher->GetMesh()->SetWorldTransform(Pusher->GetActorTransform());
		
		Pusher->AttachToComponent(StandAnker, Rules);

		// Pusher->AdjustActorLocationByZOffset();
	}
	else
	{
		Pusher->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Pusher->SetPhysicsEnabled(true);
	}
}
