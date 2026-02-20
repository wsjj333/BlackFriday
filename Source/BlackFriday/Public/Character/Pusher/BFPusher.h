#pragma once

#include "CoreMinimal.h"
#include "Character/Common/BFPawnBase.h"
#include "BFPusher.generated.h"

class UBFTeamComponent;
class UBFPusherInputComponent;
class UBFPusherDriveComponent;
class UBFCharacterAppearanceComponent;
class UCapsuleComponent;
class UBFPhysicsMovementComponent;
class UBFNetworkPhysicsComponent;
class UBFCartMovementComponent;
class UBFCharacterAnimInstance;
class ABFCartPawn;

UCLASS()
class BLACKFRIDAY_API ABFPusher : public ABFPawnBase
{
	GENERATED_BODY()

public:
	ABFPusher();

	UFUNCTION(BlueprintCallable)
	bool IsDriving() const;

	UFUNCTION(BlueprintCallable)
	ABFCartPawn* GetCart() const;

	UFUNCTION(BlueprintCallable)
	void SetCart(ABFCartPawn* NewCart) const;
	
	UFUNCTION(BlueprintCallable)
	UBFCartMovementComponent* GetCartDrivingComp() const { return CartDrivingComp; }
	
	UFUNCTION(BlueprintCallable)
	UBFPusherInputComponent* GetPusherInputComp() const { return PusherInputComp; }
	
	void SetPhysicsEnabled(bool bEnabled) const;
	void AdjustActorLocationByZOffset();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// ----- Components -----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Components")
	TObjectPtr<UBFPhysicsMovementComponent> PhysicsMoveComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Components")
	TObjectPtr<UBFNetworkPhysicsComponent> NetPhysicsComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Components")
	TObjectPtr<UBFCartMovementComponent> CartDrivingComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Input", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBFPusherInputComponent> PusherInputComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Drive", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBFPusherDriveComponent> PusherDriveComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Appearance", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBFCharacterAppearanceComponent> AppearanceComp;
	
	UPROPERTY()
	TObjectPtr<UBFTeamComponent> TeamComp;

	UPROPERTY(Transient)
	TObjectPtr<UBFCharacterAnimInstance> CachedAnimInstance;

	void RefreshAnimInstanceCache();
};
