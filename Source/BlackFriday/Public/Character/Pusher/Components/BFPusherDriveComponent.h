#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BFPusherDriveComponent.generated.h"

class ABFPusher;
class ABFCartPawn;
class UCharacterMovementComponent;
class UBFCartMovementComponent;

UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFPusherDriveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBFPusherDriveComponent();

	// ---- External API (Input/BP에서 호출) ----
	UFUNCTION(BlueprintCallable, Category="BF|Drive")
	void ToggleDrivingMode();

	UFUNCTION(BlueprintCallable, Category="BF|Drive")
	bool IsDriving() const { return bIsDriving; }

	UFUNCTION(BlueprintCallable, Category="BF|Drive")
	ABFCartPawn* GetCart() const { return Cart; }

	/** 소유 클라(Autonomous) 또는 서버에서만 호출되도록 설계 */
	UFUNCTION(BlueprintCallable, Category="BF|Drive")
	void SetCart(ABFCartPawn* NewCart);

	/** (선택) 외부에서 명시적으로 회전 정책 설정하고 싶으면 */
	UFUNCTION(BlueprintCallable, Category="BF|Drive")
	void SetOrientToMovement(const bool bEnable);

	/** 입력 컴포넌트가 카트 입력을 전달할 때 편하게 쓰는 getter */
	UBFCartMovementComponent* GetCartMovementComponent() const;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// ---- Owner Cache ----
	UPROPERTY(Transient)
	TObjectPtr<ABFPusher> OwnerPusher;

	ABFPusher* GetPusher() const;

	// ---- Dependencies (Pusher에 이미 붙어있는 CartDrivingComp를 찾아 사용) ----
	UPROPERTY(Transient)
	TObjectPtr<UBFCartMovementComponent> CachedCartMovementComp;

	void ResolveCartMovementComponent();

	// ---- Replicated State ----
	UPROPERTY(ReplicatedUsing=OnRep_Cart)
	TObjectPtr<ABFCartPawn> Cart;

	UPROPERTY(ReplicatedUsing=OnRep_IsDriving)
	bool bIsDriving = false;

	UPROPERTY(ReplicatedUsing=OnRep_OrientToMovement)
	bool bOrientToMovement = true;

	// ---- Rep Notifies ----
	UFUNCTION()
	void OnRep_Cart();

	UFUNCTION()
	void OnRep_IsDriving();

	UFUNCTION()
	void OnRep_OrientToMovement();

	// ---- RPCs ----
	UFUNCTION(Server, Reliable)
	void ServerToggleDrivingMode();

	UFUNCTION(Server, Reliable)
	void ServerSetCart(ABFCartPawn* NewCart);

	UFUNCTION(Server, Reliable)
	void ServerSetOrientToMovement(const bool bEnable);

	// ---- Apply helpers ----
	void ApplyDrivingAttachment_Server(const bool bAttach);
	void ApplyOrientToMovement(const bool bEnable);

	/** 토글 후 파생 상태 적용(orient, cart movement driving flag 등) */
	void HandleDrivingStateChanged(const bool bNowDriving);
};
