#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BFCartResetComponent.generated.h"

class UPrimitiveComponent;

UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFCartResetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBFCartResetComponent();

	UFUNCTION(BlueprintCallable, Category="Cart|Reset")
	void RequestUpright();

protected:
	virtual void BeginPlay() override;

private:
	bool CanRequestReset() const;
	bool IsUprightEnough() const;
	void DoUprightReset_ServerAuth();

	UFUNCTION(Server, Reliable)
	void Server_RequestUpright();

	UPrimitiveComponent* GetRootPrim() const;

private:
	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float UprightDotThreshold = 0.85f;

	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float MaxSpeedToAllowReset = 200.f;

	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float ExtraLift = 5.f;

	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float TraceDownDistance = 5000.f;

	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float TraceUpDistance = 200.f;

	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float ResetCooldown = 1.0f;

	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	bool bAlignToGroundNormal = true;

	double LastResetTimeSeconds = -1.0;
};