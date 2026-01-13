#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BFCartPawn.generated.h"

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
	virtual void Tick(float DeltaSeconds) override;
	
	void SuspensionCast(USceneComponent* WheelComp) const;
	
	float AccelerationInput;
	float Acceleration;
	float SpeedModifier;
	float DownForce;
	float SteeringTorque;
	float SteeringMultiplier;
	float MaxAcceleration;
	bool bIsDrifting;
	FRotator DriftRotation;
	float DriftSteer;
	float SuspensionForceMultiplier;
	FVector GroundTraceEnd;
	float Speed;
	
	// UPROPERTY()
	// TObjectPtr<ABFCartDriverCharacter> CartDriver;

private:
};
