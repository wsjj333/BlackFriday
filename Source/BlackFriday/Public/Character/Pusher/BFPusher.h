#pragma once

#include "CoreMinimal.h"
#include "Character/Common/BFCharacterBase.h"
#include "Data/Enums/BFCharacterType.h"
#include "BFPusher.generated.h"

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
	void SetCurrentSkeletalMeshAsset(EBFCharacterType NewCharacterType);

	UFUNCTION(BlueprintCallable)
	void ToggleDrivingMode();

	UFUNCTION(BlueprintCallable)
	bool IsDriving() const { return bIsDriving; }

	UFUNCTION(BlueprintCallable)
	ABFCartPawn* GetCart() const { return Cart; }

	UFUNCTION(BlueprintCallable)
	void SetCart(ABFCartPawn* NewCart);

	UFUNCTION(BlueprintCallable)
	UBFCartMovementComponent* GetCartDrivingComp() const { return CartDrivingComp; }
	
	UFUNCTION(BlueprintCallable)
	UBFPusherInputComponent* GetPusherInputComp() const { return PusherInputComp; }

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PawnClientRestart() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Drive", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBFCartMovementComponent> CartDrivingComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BF|Input", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBFPusherInputComponent> PusherInputComp;

	// ----- Skeletal Mesh -----
	UPROPERTY(ReplicatedUsing=OnRep_CharacterType, EditDefaultsOnly, Category="BF|Character")
	EBFCharacterType CharacterType = EBFCharacterType::AfroHairMan;

	UPROPERTY(EditDefaultsOnly, Category="BF|Character")
	TMap<EBFCharacterType, TSoftObjectPtr<USkeletalMesh>> CharacterMeshMap;

	UFUNCTION()
	void OnRep_CharacterType();

	// ----- Drive Mode -----
	UPROPERTY(ReplicatedUsing=OnRep_Cart)
	TObjectPtr<ABFCartPawn> Cart;

	UPROPERTY(ReplicatedUsing=OnRep_OrientToMovement)
	bool bOrientToMovement = true;

	UFUNCTION(Server, Reliable)
	void ServerSetOrientToMovement(bool bEnable);

	UFUNCTION()
	void OnRep_OrientToMovement();

	void ApplyOrientToMovement(bool bEnable);

	UFUNCTION()
	void OnRep_Cart();

	void SetOrientToMovement(bool bEnable);

	UPROPERTY(ReplicatedUsing=OnRep_IsDriving)
	bool bIsDriving = false;

	UFUNCTION()
	void OnRep_IsDriving();

	UPROPERTY(Transient)
	TObjectPtr<UBFCharacterAnimInstance> CachedAnimInstance;

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
