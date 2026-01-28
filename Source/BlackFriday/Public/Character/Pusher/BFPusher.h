#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Character/Common/BFCharacterBase.h"
#include "Data/Enums/BFCharacterType.h"
#include "BFPusher.generated.h"

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
	void SetCurrentSkeletalMeshAsset(EBFCharacterType NewCharacterType);

	UFUNCTION(BlueprintCallable)
	void ToggleDrivingMode();

	UFUNCTION(BlueprintCallable)
	bool IsDriving() const { return bIsDriving; }

	UFUNCTION(BlueprintCallable)
	ABFCartPawn* GetCart() const { return Cart; }

	UFUNCTION(BlueprintCallable)
	void SetCart(ABFCartPawn* NewCart);

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Drive", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBFCartMovementComponent> CartDrivingComp;

	// ----- Skeletal Mesh -----
	UPROPERTY(ReplicatedUsing=OnRep_CharacterType, EditDefaultsOnly, Category="BF|Character")
	EBFCharacterType CharacterType = EBFCharacterType::AfroHairMan;

	UPROPERTY(EditDefaultsOnly, Category="BF|Character")
	TMap<EBFCharacterType, TSoftObjectPtr<USkeletalMesh>> CharacterMeshMap;

	UFUNCTION()
	void OnRep_CharacterType();

	// ----- Input Actions -----
	UPROPERTY(EditDefaultsOnly, Category="BF|Input")
	TObjectPtr<UInputMappingContext> PusherMappingContext;

	UPROPERTY(EditDefaultsOnly, Category="BF|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category="BF|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category="BF|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category="BF|Input")
	TObjectPtr<UInputAction> DriveModeAction;
	
	UPROPERTY(EditDefaultsOnly, Category="BF|Input")
	TObjectPtr<UInputAction> AccelerationAction;
	
	UPROPERTY(EditDefaultsOnly, Category="BF|Input")
	TObjectPtr<UInputAction> SteerAction;

	// ----- Drive Mode -----
	UPROPERTY(ReplicatedUsing=OnRep_Cart)
	TObjectPtr<ABFCartPawn> Cart;

	UFUNCTION()
	void OnRep_Cart();

	UPROPERTY(ReplicatedUsing=OnRep_IsDriving)
	bool bIsDriving = false;

	UFUNCTION()
	void OnRep_IsDriving();

	UPROPERTY(Transient)
	TObjectPtr<UBFCharacterAnimInstance> CachedAnimInstance;

	// ----- Bound Functions -----
	void HandleMoveInput(const FInputActionValue& Value);
	void HandleLookInput(const FInputActionValue& Value);
	void OnJumpPressed(const FInputActionValue& Value);
	void OnJumpReleased(const FInputActionValue& Value);
	void OnToggleDriveModePressed(const FInputActionValue& Value);

	//void ApplyDrivingState_Local(bool bDriving);
	void HardClampControlRotation();
	void RefreshAnimInstanceCache();


	// -------- 서버 권한 RPC --------
	UFUNCTION(Server, Reliable)
	void ServerToggleDrivingMode();

	UFUNCTION(Server, Reliable)
	void ServerSetCart(ABFCartPawn* NewCart);

	UFUNCTION(Server, Reliable)
	void ServerSetCharacterType(EBFCharacterType NewType);

	// 서버에서만 실행되는 실제 부착/해제
	void ApplyDrivingAttachment_Server(bool bAttach);
};
