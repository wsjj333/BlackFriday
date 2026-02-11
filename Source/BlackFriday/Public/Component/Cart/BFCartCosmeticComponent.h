// #pragma once
//
// #include "CoreMinimal.h"
// #include "Components/ActorComponent.h"
// #include "BFCartCosmeticComponent.generated.h"
//
// class ABFCartPawn;
// class UStaticMeshComponent;
// class USceneComponent;
// class UPrimitiveComponent;
//
// UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
// class BLACKFRIDAY_API UBFCartCosmeticComponent : public UActorComponent
// {
// 	GENERATED_BODY()
//
// public:
// 	UBFCartCosmeticComponent();
//
// protected:
// 	virtual void BeginPlay() override;
// 	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
//
// private:
// 	void CacheRefs();
//
// private:
// 	UPROPERTY(EditDefaultsOnly, Category="BF|Cosmetic")
// 	float RotationInterpSpeed = 3.f;
//
// 	UPROPERTY(EditDefaultsOnly, Category="BF|Cosmetic")
// 	float VisualSteerYawDeg = 25.f;
//
// 	UPROPERTY(EditDefaultsOnly, Category="BF|Cosmetic")
// 	float WheelSpinSpeedScale = 1.0f;
//
// 	float PrevSpeed2D = 0.f;
//
// 	TWeakObjectPtr<ABFCartPawn> Cart;
// 	TWeakObjectPtr<UPrimitiveComponent> RootPrim;
//
// 	TArray<TWeakObjectPtr<UStaticMeshComponent>> WheelMeshes;
// 	TWeakObjectPtr<USceneComponent> Pivot;
// 	TWeakObjectPtr<UStaticMeshComponent> CartBody;
// };