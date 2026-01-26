#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Character/Common/BFCharacterBase.h"
#include "Data/Enums/BFCharacterType.h"
#include "BFPusher.generated.h"

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
	void SetCurrentSkeletalMeshAsset(EBFCharacterType NewCharacterType);
	
	UFUNCTION(BLueprintCallable)
	void ToggleDrivingMode();
	
	UFUNCTION(BlueprintCallable)
	bool IsDriving() const;
	
	UFUNCTION(BlueprintCallable)
	ABFCartPawn* GetCart() const;
	
	UFUNCTION(BLueprintCallable)
	void SetCart(ABFCartPawn* NewCart);

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	// ----- Skeletal Mesh -----
	UPROPERTY(EditDefaultsOnly, Category="BF|Character")
	EBFCharacterType CharacterType;
	
	UPROPERTY(EditDefaultsOnly, Category="BF|Character")
	TMap<EBFCharacterType, TSoftObjectPtr<USkeletalMesh>> CharacterMeshMap;
	
	// ----- Bound Functions -----
	void HandleMoveInput(const FInputActionValue& Value);
	void HandleLookInput(const FInputActionValue& Value);
	void OnJumpPressed(const FInputActionValue& Value);
	void OnJumpReleased(const FInputActionValue& Value);
	void OnToggleDriveModePressed(const FInputActionValue& Value);
	void OnMoveEnded(const FInputActionValue& Value);
	void HandleSteeringInput(const FInputActionValue& Value);
	void OnSteeringEnded(const FInputActionValue& Value);
	
	// ----- etc -----
	void HardClampControlRotation();
	

	// ----- Input Actions -----
	UPROPERTY(EditDefaultsOnly, Category = "BF|Input")
	TObjectPtr<UInputMappingContext> PusherMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "BF|Input")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "BF|Input")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "BF|Input")
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "BF|Input")
	TObjectPtr<UInputAction> DriveModeAction;
	
	// ----- Drive Mode -----
	UPROPERTY()
	TObjectPtr<ABFCartPawn> Cart;
	
	UPROPERTY()
	TObjectPtr<UBFCharacterAnimInstance> CachedAnimInstance;
	
	bool bIsDriving = false;
	
private:
	
};
