// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "BFPhysicsNetTypes.h"
#include "BFPhysicsMovementComponent.generated.h"

UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFPhysicsMovementComponent : public UPawnMovementComponent
{
	GENERATED_BODY()

public:
	UBFPhysicsMovementComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 네트워크/로컬에서 공통으로 현재 입력을 세팅
	void SetCurrentInput(const FBFMoveInputNet& InInput);

	void ApplyInputImmediately(const FBFMoveInputNet& InInput);
	
	// Debug
	UPROPERTY(EditAnywhere, Category="BF|Debug")
	bool bDebugMove = false;

	UPROPERTY(EditAnywhere, Category="BF|Debug", meta=(ClampMin="0.01"))
	float DebugInterval = 0.25f;
	
	UPROPERTY(EditAnywhere, Category="BF|Move")
	float RotationSpeed = 15.0f;
	
	UPROPERTY(EditAnywhere, Category="BF|Move")
	TObjectPtr<UPrimitiveComponent> PhysicsPrimitiveOverride = nullptr;

	// 튜닝
	UPROPERTY(EditAnywhere, Category="BF|Move")
	float MoveForce = 500000.f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float AirControl = 0.35f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float MaxSpeed = 700.f;
	
	UPROPERTY(EditAnywhere, Category="BF|Move")
	float AccelMultiplier = 2.5f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float MovingLinearDamping = 3.0f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float BrakingLinearDamping = 20.0f;
	
	UPROPERTY(EditAnywhere, Category="BF|Move")
	float JumpImpulse = 420.f;

	UPROPERTY(EditAnywhere, Category="BF|Ground")
	float GroundTraceLength = 120.f;

	UPROPERTY(EditAnywhere, Category="BF|Ground")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, Category="BF|Ground")
	float GroundedDotThreshold = 0.6f;

	// 상체 물리 (흐느적거림)
	UPROPERTY(EditAnywhere, Category="BF|PhysicalAnimation")
	bool bEnableUpperBodyPhysics = false;

	UPROPERTY(EditAnywhere, Category="BF|PhysicalAnimation")
	FName UpperBodyBoneName = TEXT("spine_02");

	UPROPERTY(EditAnywhere, Category="BF|PhysicalAnimation")
	float OrientationStrength = 1000.f;

	UPROPERTY(EditAnywhere, Category="BF|PhysicalAnimation")
	float AngularVelocityStrength = 100.f;

	UPROPERTY(EditAnywhere, Category="BF|PhysicalAnimation")
	float PositionStrength = 1000.f;

	UPROPERTY(EditAnywhere, Category="BF|PhysicalAnimation")
	float VelocityStrength = 100.f;

	// 현재 바닥 상태
	UPROPERTY(BlueprintReadOnly, Category="BF|Ground")
	bool bGrounded = false;

	UPROPERTY(BlueprintReadOnly, Category="BF|Ground")
	FVector GroundNormal = FVector::UpVector;

	UFUNCTION(BlueprintCallable, Category="BF|Motion")
	FVector GetBFVelocity() const;

	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:
	// 캐싱된 물리 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> Prim = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> CachedMesh = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPhysicalAnimationComponent> PhysicalAnimationComp = nullptr;

	void CachePrimitive();
	void SetupUpperBodyPhysics();

private:
	float MoveX = 0.f;
	float MoveY = 0.f;
	float InputYawDeg = 0.f;

	bool bJumpHeld = false;
	bool bPrevJumpHeld = false;

	float DebugAcc = 0.f;
	FVector SmoothAnimVelocity = FVector::ZeroVector;
	float JumpBufferTime = 0.f;
	float JumpCooldownTime = 0.f;
};