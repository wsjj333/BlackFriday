#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BFPusherDriveComponent.generated.h"

class ABFCartPawn;
class ABFPusher;
class UBFCartMovementComponent;
class UCharacterMovementComponent;

UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFPusherDriveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// ----- Constructor -----
	UBFPusherDriveComponent();
	
	// ----- Public API -----
	UFUNCTION(BlueprintCallable, Category="BF|Drive")
	void ToggleDrivingMode();

	UFUNCTION(BlueprintCallable, Category="BF|Drive")
	void SetCart(ABFCartPawn* NewCart);
	
	/** 외부에서 명시적으로 회전 정책 설정하고 싶을 때 사용 */
	UFUNCTION(BlueprintCallable, Category="BF|Drive")
	void SetOrientToMovement(bool bEnable);

	UFUNCTION(BlueprintCallable, Category="BF|Drive")
	bool IsDriving() const { return bIsDriving; }

	UFUNCTION(BlueprintCallable, Category="BF|Drive")
	ABFCartPawn* GetCart() const { return Cart; }
	
	UBFCartMovementComponent* GetCartMovementComponent() const;
	
	void ApplyOrientToMovement(bool bEnable);
	
protected:
	// ----- UE Lifecycle -----
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// ---- Owner Cache ----
	ABFPusher* GetPusher() const;

	void ResolveCartMovementComponent();
	
	UPROPERTY(Transient)
	TObjectPtr<ABFPusher> OwnerPusher;
	
	UPROPERTY(Transient)
	TObjectPtr<UBFCartMovementComponent> CachedCartMovementComp;

	// ---- Replicated State ----
	UPROPERTY(ReplicatedUsing=OnRep_Cart)
	TObjectPtr<ABFCartPawn> Cart = nullptr;

	UPROPERTY(ReplicatedUsing=OnRep_IsDriving)
	bool bIsDriving = false;

	UPROPERTY(ReplicatedUsing=OnRep_OrientToMovement)
	bool bOrientToMovement = true;

private:
	// ---- Rep Notifies ----
	UFUNCTION() void OnRep_Cart();
	UFUNCTION() void OnRep_IsDriving();
	UFUNCTION() void OnRep_OrientToMovement();

private:
	// ---- RPCs ----
	UFUNCTION(Server, Reliable) void ServerToggleDrivingMode();
	UFUNCTION(Server, Reliable) void ServerSetCart(ABFCartPawn* NewCart);
	UFUNCTION(Server, Reliable) void ServerSetOrientToMovement(bool bEnable);

private:
	// ---- Internal helpers ----
	void ApplyDrivingAttachment(bool bAttach);
	void HandleDrivingStateChanged(bool bNowDriving);
};
