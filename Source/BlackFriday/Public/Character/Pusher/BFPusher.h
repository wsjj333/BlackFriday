#pragma once

#include "CoreMinimal.h"
#include "Character/Common/BFPawnBase.h"
#include "BFPusher.generated.h"

class ABFCartPawn;
class UBFCartMovementComponent;
class UBFCartOverlapDetectorComponent;
class UBFCharacterAnimInstance;
class UBFCharacterAppearanceComponent;
class UBFNetworkPhysicsComponent;
class UBFPusherDriveComponent;
class UBFPusherInputComponent;
class UBFPhysicsMovementComponent;
class UBFTeamComponent;

/**
 * Pusher가 밀 대상 Cart를 참조합니다.
 * - 같은 팀 Cart 지정
 */
UCLASS()
class BLACKFRIDAY_API ABFPusher : public ABFPawnBase
{
	GENERATED_BODY()

public:
	// ----- Constructor -----
	ABFPusher();
	
	// ----- Gameplay API -----
	UFUNCTION(BlueprintPure)
	bool IsDriving() const;

	UFUNCTION(BlueprintPure)
	bool IsOverlappingCart() const;

	UFUNCTION(BlueprintPure)
	ABFCartPawn* GetCart() const;
	
	UFUNCTION()
	void SetCart(ABFCartPawn* NewCart);
	
	UBFCartMovementComponent* GetCartDrivingComp() const { return CartDrivingComp; }
	UBFPusherInputComponent* GetPusherInputComp() const { return PusherInputComp; }

	void SetPhysicsEnabled(bool bEnabled);
	void AdjustActorLocationByZOffset();

protected:
	// ----- UE Lifecycle -----
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// ----- Components -----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Appearance")
	TObjectPtr<UBFCharacterAppearanceComponent> AppearanceComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Drive")
	TObjectPtr<UBFCartMovementComponent> CartDrivingComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Overlap")
	TObjectPtr<UBFCartOverlapDetectorComponent> CartOverlapComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Physics")
	TObjectPtr<UBFNetworkPhysicsComponent> NetPhysicsComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Physics")
	TObjectPtr<UBFPhysicsMovementComponent> PhysicsMoveComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Drive")
	TObjectPtr<UBFPusherDriveComponent> PusherDriveComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Input")
	TObjectPtr<UBFPusherInputComponent> PusherInputComp;

	UPROPERTY(BlueprintReadOnly, Category="BF|Team")
	TObjectPtr<UBFTeamComponent> TeamComp;

protected:
	// ----- Internal Helpers -----
	void RefreshAnimInstanceCache();

private:
	// ----- Runtime Cache -----
	UPROPERTY(Transient)
	TObjectPtr<UBFCharacterAnimInstance> CachedAnimInstance;
};
