#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BFCartOverlapDetectorComponent.generated.h"

class ABFCartPawn;
class UPrimitiveComponent;
class USphereComponent;

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
	// ----- UE Lifecycle -----
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
private:
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
	
private:
	// Config
	UPROPERTY(EditAnywhere, Category="Overlap")
	float SphereRadius = 150.f;
	
	// Runtime state
	UPROPERTY(Replicated, VisibleInstanceOnly, Category="BF|Overlap")
	bool bIsOverlappingCart = false;
	
	// Internal component
	UPROPERTY(Transient)
	TObjectPtr<USphereComponent> OverlapSphere;
	
	// Server-only data 
	UPROPERTY(Replicated, VisibleInstanceOnly, Category="BF|Overlap")
	TArray<TObjectPtr<ABFCartPawn>> OverlappingCarts;
};