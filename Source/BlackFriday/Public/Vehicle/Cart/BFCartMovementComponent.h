#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/PawnMovementComponent.h"
#include "BFCartMovementComponent.generated.h"

class ABFCartPawn;

UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFCartMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	// ----- Constructor -----
	UBFCartMovementComponent();

	// ----- Public API -----
	bool IsDrifting() const { return bIsDrifting; }
	
	void SetCart(ABFCartPawn* InCart);
	void SetDriving(bool bInDriving);

	// ----- Input Handler (delegated from Pusher input) -----
	void Input_AccelTriggered(const FInputActionValue& Value);
	void Input_AccelEnded(const FInputActionValue& Value);

	void Input_SteerTriggered(const FInputActionValue& Value);
	void Input_SteerEnded(const FInputActionValue& Value);

	void Input_DriftStarted(const FInputActionValue& Value);
	void Input_DriftEnded(const FInputActionValue& Value);
	
protected:
	bool CanSendInput() const;
	
	UFUNCTION(Server, Reliable)
	void Server_SetCartAccelerationAxis(float Axis);
	
	UFUNCTION(Server, Reliable)
	void Server_SetCartSteeringAxis(float Axis);
	
	UFUNCTION(Server, Reliable)
	void Server_SetCartSteeringMultiplier(float Multiplier);
	
private:
	TWeakObjectPtr<ABFCartPawn> Cart;

	bool bIsDriving = false;
	bool bIsDrifting = false;
};
