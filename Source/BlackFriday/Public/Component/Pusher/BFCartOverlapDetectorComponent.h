#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BFCartOverlapDetectorComponent.generated.h"

class USphereComponent;
class ABFCartPawn;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFCartOverlapDetectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBFCartOverlapDetectorComponent();

	UFUNCTION(BlueprintCallable, Category="BF|Overlap")
	bool IsOverlappingCart() const { return bIsOverlappingCart; }

	UFUNCTION(BlueprintCallable, Category="BF|Overlap")
	void ForceRecheckOverlap_ServerOnly();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category="BF|Overlap")
	TObjectPtr<USphereComponent> OverlapSphere;

	UPROPERTY(EditAnywhere, Category="Overlap")
	float SphereRadius = 150.f;

	/** 서버 확정 결과(복제) */
	UPROPERTY(Replicated, VisibleInstanceOnly, Category="BF|Overlap")
	bool bIsOverlappingCart = false;

	/** 서버에서만 추적: 현재 겹치는 Cart들 */
	UPROPERTY(Replicated, VisibleInstanceOnly, Category="BF|Overlap")
	TArray<TObjectPtr<ABFCartPawn>> OverlappingCarts;

	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void HandleEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	void RecalculateOverlapState_ServerOnly();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};