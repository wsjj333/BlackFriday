#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "BFCartPawn.generated.h"

class UInputAction;
class UInputMappingContext;
class UBoxComponent;
class UCapsuleComponent;
class UStaticMeshComponent;
class USceneComponent;
class UBFCartMovementComponent;
class ABFCartDriverCharacter;

UCLASS()
class BLACKFRIDAY_API ABFCartPawn : public APawn
{
	GENERATED_BODY()

public:
	ABFCartPawn();

protected:
	virtual void BeginPlay() override;
	
	virtual void Tick(float DeltaSeconds) override;
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void SuspensionCast(USceneComponent* WheelComp) const;
	
	void SetAccelerationInput(const FInputActionValue& Value);
	
	void SteerCart(const FInputActionValue& Value);
	
	void RotateMeshes();
	
	void CalculateAcceleration();
	
	void AccelerateCart() const;
	
	void SetCartCenterOfMass() const;
	
	bool IsOnGround() const;
	
	void StartDrift();
	
	void StopDrift();
	
	float AccelerationInput = 0.0f;
	float Acceleration = 0.0f;
	float SpeedModifier = 1.0f;
	float DownForce = -4900000.0f;
	
	UPROPERTY(EditDefaultsOnly, Category="BF|Movement")
	double SteeringTorque = 900000000.0f;
	
	float SteeringMultiplier = 2.0f;
	
	bool bIsDrifting = false;
	FRotator DriftRotation = FRotator(0.0f, 0.0f, 0.0f);
	float DriftSteer = 0.0f;
	float SuspensionForceMultiplier = 10000000.0f;
	
	UPROPERTY(EditDefaultsOnly)
	FVector CartCenterOfMess = FVector(0.0f, 0.0f, -10.0f);
	
	/** 차체가 땅에 닿아있는지 판단하는 벡터 */
	UPROPERTY(EditAnywhere)
	FVector GroundTraceEnd = FVector(0.0f, 0.0f, 1500.0f);
	
	UPROPERTY(EditDefaultsOnly, Category = "BF|Movement")
	float MaxAcceleration = 15000.0f;
	
	UPROPERTY(EditAnywhere, Category="BF|Movement")
	float CartSpeed = 1000.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "BF|Input")
	TObjectPtr<UInputMappingContext> CartMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "BF|Input")
	TObjectPtr<UInputAction> AccelerationAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "BF|Input")
	TObjectPtr<UInputAction> SteeringAction;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Root;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CartBody;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CartHandle;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> Pivot;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> WheelFRComp;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> WheelFLComp;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> WheelBRComp;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> WheelBLComp;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CasterForkFR;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CasterForkFL;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CasterForkBR;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CasterForkBL;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> WheelFRMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> WheelFLMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> WheelBRMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> WheelBLMesh;
	
	// UPROPERTY()
	// TObjectPtr<ABFCartDriverCharacter> CartDriver;

private:
};
