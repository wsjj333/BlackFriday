#pragma once

#include "CoreMinimal.h"
#include "Character/Common/BFCharacterBase.h"
#include "Data/Enums/BFCharacterType.h"
#include "BFPusher.generated.h"

class UBFCharacterAppearanceComponent;
class UBFPusherDriveComponent;
class UBFPusherInputComponent;
class UBFCartMovementComponent;
class UBFCharacterAnimInstance;
class UInputMappingContext;
class UInputAction;
class ABFCartPawn;

UCLASS()
class BLACKFRIDAY_API ABFPusher : public ABFCharacterBase
{
	GENERATED_BODY()

public:
	ABFPusher();

	UFUNCTION(BlueprintCallable)
	void ToggleDrivingMode();

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

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Drive", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBFCartMovementComponent> CartDrivingComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Input", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBFPusherInputComponent> PusherInputComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Drive", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBFPusherDriveComponent> PusherDriveComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Appearance", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBFCharacterAppearanceComponent> AppearanceComp;
	
	// ----- Drive Mode -----
	UPROPERTY(Transient)
	TObjectPtr<UBFCharacterAnimInstance> CachedAnimInstance;

	void RefreshAnimInstanceCache();
	
private:
	UFUNCTION()
	void HandleAppearanceApplied(EBFCharacterType AppliedType);

};
