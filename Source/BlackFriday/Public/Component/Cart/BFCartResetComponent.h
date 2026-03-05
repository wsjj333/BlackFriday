#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BFCartResetComponent.generated.h"

class APlayerController;
class UBoxComponent;

UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFCartResetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBFCartResetComponent();

	UFUNCTION(BlueprintCallable, Category="Cart|Reset")
	void RequestUpright();

protected:
	UFUNCTION(Server, Reliable)
	void Server_RequestUpright(APlayerController* RequestingPC);
	
private:
	bool CanRequestReset() const;
	bool IsUprightEnough() const;
	
	void DoUprightReset_ServerAuth();

	UBoxComponent* GetRootComp() const;

private:
	// ----- Reset Condition -----
	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float UprightDotThreshold = 0.85f;

	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float MaxSpeedToAllowReset = 200.f;

	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float ResetCooldown = 1.0f;
	
	// ----- Trace -----
	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float TraceDownDistance = 5000.f;

	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float TraceUpDistance = 200.f;
	
	// ----- Placement -----
	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float ExtraLift = 5.f;

	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	bool bAlignToGroundNormal = true;

	// ----- Cooldown -----
	double LastResetTimeSeconds = -1.0;
};