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
    UBFCartMovementComponent();
    
    bool IsDrifting() const;

    // Pusher에서 RepNotify 타이밍에 전달
    void SetCart(ABFCartPawn* InCart);
    void SetDriving(bool bInDriving);

    // Pusher 입력 바인딩이 여기로 위임
    void Input_AccelTriggered(const FInputActionValue& Value);
    void Input_AccelEnded(const FInputActionValue& Value);

    void Input_SteerTriggered(const FInputActionValue& Value);
    void Input_SteerEnded(const FInputActionValue& Value);
    
    void Input_DriftStarted(const FInputActionValue& Value);
    void Input_DriftEnded(const FInputActionValue& Value);
    
private:
    TWeakObjectPtr<ABFCartPawn> Cart;
    
    bool bDriving = false;
    
    bool bIsDrifting = false;

    bool CanSendInput() const;

    // ---- Server RPC (Owner는 Pusher이므로 OwningConnection OK) ----
    UFUNCTION(Server, Reliable)
    void Server_SetCartAccelerationAxis(float Axis);

    UFUNCTION(Server, Reliable)
    void Server_SetCartSteeringAxis(float Axis);
    
    UFUNCTION(Server, Reliable)
    void Server_SetCartSteeringMultiplier(float Multiplier);
};
