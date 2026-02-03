#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "Data/Enums/BFCharacterType.h"
#include "BFPusherNet.generated.h"

class UBFPusherInputComponent;
class UBFPusherDriveComponent;
class UBFCharacterAppearanceComponent;
class UCapsuleComponent;
class USkeletalMeshComponent;
class UBFPhysicsMovementComponent;
class UBFNetworkPhysicsComponent;
class UBFCartMovementComponent;
class UBFCharacterAnimInstance;
class UInputMappingContext;
class UInputAction;
class ABFCartPawn;

UCLASS()
class BLACKFRIDAY_API ABFPusherNet : public APawn
{
	GENERATED_BODY()

public:
	ABFPusherNet();

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

	UFUNCTION(BlueprintPure, Category="BF|Components")
	USkeletalMeshComponent* GetMesh() const { return MeshComp; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ----- Components -----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Components")
	TObjectPtr<UCapsuleComponent> CapsuleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Components")
	TObjectPtr<USkeletalMeshComponent> MeshComp;

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

	void HardClampControlRotation();
	void RefreshAnimInstanceCache();
	void SetPhysicsEnabled(bool bEnabled);

	// ----- Server RPC -----
	UFUNCTION(Server, Reliable)
	void ServerToggleDrivingMode();

	UFUNCTION(Server, Reliable)
	void ServerSetCart(ABFCartPawn* NewCart);

	UFUNCTION(Server, Reliable)
	void ServerSetCharacterType(EBFCharacterType NewType);

	void ApplyDrivingAttachment_Server(bool bAttach);
};
