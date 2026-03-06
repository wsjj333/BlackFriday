#include "Vehicle/Cart/BFCartMovementComponent.h"


// Engine
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"

// Vehicle
#include "Vehicle/Cart/BFCartPawn.h"

UBFCartMovementComponent::UBFCartMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBFCartMovementComponent::SetCart(ABFCartPawn* InCart)
{
	Cart = InCart;
}

void UBFCartMovementComponent::SetDriving(bool bInDriving)
{
	bIsDriving = bInDriving;

	// 로컬 카트에도 드라이빙 상태 업데이트
	if (Cart.IsValid())
	{
		Cart->SetDriving_Local(bIsDriving);
	}
	
	if (bIsDriving) return;
	
	if (CanSendInput())
	{
		Server_SetCartAccelerationAxis(0.f);
		Server_SetCartSteeringAxis(0.f);

		// 로컬 값 리셋
		if (Cart.IsValid())
		{
			Cart->SetAccelAxis_Local(0.f);
			Cart->SetSteerAxis_Local(0.f);
			
			// 로컬 코스매틱도 같이 리셋
			Cart->SetCosmeticAccelInput(0.f);
		}
	}
	
}

void UBFCartMovementComponent::Input_AccelTriggered(const FInputActionValue& Value)
{
	if (!CanSendInput()) return;

	const float Axis = Value.Get<float>();
	Server_SetCartAccelerationAxis(Axis);
	
	// 클라이언트 화면에서 즉각적으로 반응하도록 로컬에도 적용
	if (Cart.IsValid())
	{
		Cart->SetAccelAxis_Local(Axis);
		Cart->SetCosmeticAccelInput(Axis);
	}
}

void UBFCartMovementComponent::Input_AccelEnded(const FInputActionValue& Value)
{
	if (!CanSendInput()) return;
	
	Server_SetCartAccelerationAxis(0.f);
	
	if (Cart.IsValid())
	{
		Cart->SetAccelAxis_Local(0.f);
		Cart->SetCosmeticAccelInput(0.f);
	}
}

void UBFCartMovementComponent::Input_SteerTriggered(const FInputActionValue& Value)
{
	if (!CanSendInput()) return;

	const float Axis = Value.Get<float>();
	Server_SetCartSteeringAxis(Axis);
	if (Cart.IsValid()) Cart->SetSteerAxis_Local(Axis);
}

void UBFCartMovementComponent::Input_SteerEnded(const FInputActionValue& Value)
{
	if (!CanSendInput()) return;
	Server_SetCartSteeringAxis(0.f);
	if (Cart.IsValid()) Cart->SetSteerAxis_Local(0.f);
}

void UBFCartMovementComponent::Input_DriftStarted(const FInputActionValue& Value)
{
	if (!CanSendInput()) return;
	
	const float Angle = Value.Get<float>();
	
	Server_SetCartSteeringMultiplier(4.0f);
	bIsDrifting = true;
}

void UBFCartMovementComponent::Input_DriftEnded(const FInputActionValue& Value)
{
	if (!CanSendInput()) return;
	
	const float Angle = Value.Get<float>();
	
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
	if (!bIsDriving) return false;
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

	// 서버 권한에서 직접 서버 상태에 적용
	Cart->SetAccelAxis_Server(Axis);
}

void UBFCartMovementComponent::Server_SetCartSteeringAxis_Implementation(float Axis)
{
	if (!Cart.IsValid()) return;
	Cart->SetSteerAxis_Server(Axis);
}
