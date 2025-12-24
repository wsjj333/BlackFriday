#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BFCartPawn.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class USceneComponent;
class UBFCartMovementComponent;

UCLASS()
class BLACKFRIDAY_API ABFCartPawn : public APawn
{
	GENERATED_BODY()

public:
	ABFCartPawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual UPawnMovementComponent* GetMovementComponent() const override;

	// Character가 잡을 핸들 위치 제공
	UFUNCTION(BlueprintCallable, Category="Cart|Pull")
	USceneComponent* GetHandleComponent() const { return Handle; }
	
	// Character를 위치 시킬 방향 제공
	UFUNCTION(BlueprintCallable, Category="Cart|Pull")
	USceneComponent* GetHandleFacingComponent() const { return HandleArrow; }

	// 안전하게 입력 라우팅 (Character/Controller가 호출)
	void SetThrottle(float Value);
	void SetSteer(float Value);
	void SetDriftHeld(bool bHeld);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	UCapsuleComponent* Capsule;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CartMesh;

	// 캐릭터가 붙는 기준점(핸들)
	UPROPERTY(VisibleAnywhere)
	USceneComponent* Handle;
	
	// 캐릭터가 붙는 기준점의 방향(핸들)
	UPROPERTY(VisibleAnywhere)
	USceneComponent* HandleArrow;

	UPROPERTY(VisibleAnywhere)
	UBFCartMovementComponent* CartMovement;
};
