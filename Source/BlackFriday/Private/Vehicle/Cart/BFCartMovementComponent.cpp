#include "Vehicle/Cart/BFCartMovementComponent.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Vehicle/Cart/BFCartPawn.h"

UBFCartMovementComponent::UBFCartMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true); // 컴포넌트 RPC 안정성(권장)
}

bool UBFCartMovementComponent::IsDrifting() const
{
	return bIsDrifting;
}

void UBFCartMovementComponent::SetCart(ABFCartPawn* InCart)
{
	Cart = InCart;
}

void UBFCartMovementComponent::SetDriving(bool bInDriving)
{
	bDriving = bInDriving;

	// 드라이빙을 끄는 순간 입력축을 0으로 리셋 (로컬/서버 정합성)
	if (!bDriving)
	{
		if (CanSendInput())
		{
			Server_SetCartAccelerationAxis(0.f);
			Server_SetCartSteeringAxis(0.f);
		}
	}
	
}

void UBFCartMovementComponent::Input_AccelTriggered(const FInputActionValue& Value)
{
	if (!CanSendInput()) return;

	const float Axis = Value.Get<float>();
	Server_SetCartAccelerationAxis(Axis);
}

void UBFCartMovementComponent::Input_AccelEnded(const FInputActionValue& Value)
{
	if (!CanSendInput()) return;
	Server_SetCartAccelerationAxis(0.f);
}

void UBFCartMovementComponent::Input_SteerTriggered(const FInputActionValue& Value)
{
	if (!CanSendInput()) return;

	const float Axis = Value.Get<float>();
	Server_SetCartSteeringAxis(Axis);
}

void UBFCartMovementComponent::Input_SteerEnded(const FInputActionValue& Value)
{
	if (!CanSendInput()) return;
	Server_SetCartSteeringAxis(0.f);
}

void UBFCartMovementComponent::Input_DriftStarted(const FInputActionValue& Value)
{
	if (!CanSendInput()) return;
	
	const float Angle = Value.Get<float>();
	
	// TODO: 매직넘버 수정(Rep_SteeringMultiplier 값의 2배를 의도함)
	Server_SetCartSteeringMultiplier(4.0f);
	bIsDrifting = true;
}

void UBFCartMovementComponent::Input_DriftEnded(const FInputActionValue& Value)
{
	if (!CanSendInput()) return;
	
	const float Angle = Value.Get<float>();
	
	// TODO: 매직넘버 수정(Rep_SteeringMultiplier 원래 값을 의도함)
	Server_SetCartSteeringMultiplier(2.0f);
	bIsDrifting = false;
}

bool UBFCartMovementComponent::CanSendInput() const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return false;

	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (!OwnerPawn) return false;

	// 로컬 소유자만 입력 전송
	if (!OwnerPawn->IsLocallyControlled()) return false;

	// 드라이빙 중 + 카트 유효
	if (!bDriving) return false;
	if (!Cart.IsValid()) return false;

	return true;
}

void UBFCartMovementComponent::Server_SetCartSteeringMultiplier_Implementation(float Multiplier)
{
	if (!Cart.IsValid()) return;
	
	Cart->SetSteeringMultiplier_Server(Multiplier);
}

void UBFCartMovementComponent::Server_SetCartAccelerationAxis_Implementation(float Axis)
{
	if (!Cart.IsValid()) return;

	Axis = FMath::Clamp(Axis, -1.f, 1.f);

	// ✅ 서버 권한에서 직접 서버 상태에 적용
	Cart->SetAccelAxis_Server(Axis);
}

void UBFCartMovementComponent::Server_SetCartSteeringAxis_Implementation(float Axis)
{
	if (!Cart.IsValid()) return;
	Cart->SetSteerAxis_Server(Axis);
}
