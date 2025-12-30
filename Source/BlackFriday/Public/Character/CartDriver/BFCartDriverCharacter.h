#pragma once

#include "CoreMinimal.h"
#include "Character/Common/BFCharacterBase.h"
#include "BFCartDriverCharacter.generated.h"

class ABFCartPawn;

UCLASS()
class BLACKFRIDAY_API ABFCartDriverCharacter : public ABFCharacterBase
{
	GENERATED_BODY()

public:
	ABFCartDriverCharacter();

	virtual void Tick(float DeltaSeconds) override;

	// Controller에서 호출
	void InputMove(const FVector2D& MoveAxis);
	void InputLook(const FVector2D& LookAxis);
	void InputDriftPressed();
	void InputDriftReleased();
	void InputInteract();

	UFUNCTION(BlueprintCallable, Category="Cart|Pull")
	bool IsPullingCart() const { return bIsPullingCart; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleInstanceOnly, Category="Cart|Pull")
	ABFCartPawn* CurrentCart = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category="Cart|Pull")
	bool bIsPullingCart = false;

	// 카트 탐색
	UPROPERTY(EditAnywhere, Category="Cart|Pull")
	float FindCartRadius = 200.f;

	// 캐릭터가 핸들에 붙을 상대 오프셋(캐릭터 캡슐/애니에 맞춰 조정)
	UPROPERTY(EditAnywhere, Category="Cart|Pull")
	FVector PullAttachOffset = FVector(-40.f, 0.f, -90.f);

private:
	void StartPulling(ABFCartPawn* Cart);
	void StopPulling();

	ABFCartPawn* FindNearestCart() const;

	void RouteMoveToCharacter(const FVector2D& MoveAxis);
	void RouteMoveToCart(const FVector2D& MoveAxis);
};
