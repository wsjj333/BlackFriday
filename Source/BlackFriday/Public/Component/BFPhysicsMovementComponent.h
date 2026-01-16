// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
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

	// 외부에서 물리로 움직일 대상(UpdatedComponent)을 지정하지 않으면 RootPrimitive를 자동 사용 시도
	UPROPERTY(EditAnywhere, Category="BF|Move")
	TObjectPtr<UPrimitiveComponent> PhysicsPrimitiveOverride = nullptr;

	// 튜닝
	UPROPERTY(EditAnywhere, Category="BF|Move")
	float MoveForce = 180000.f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float AirControl = 0.35f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float MaxSpeed = 1200.f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float JumpImpulse = 420.f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float MovingLinearDamping = 1.0f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float BrakingLinearDamping = 6.0f;

	UPROPERTY(EditAnywhere, Category="BF|Ground")
	float GroundTraceLength = 110.f;

	UPROPERTY(EditAnywhere, Category="BF|Ground")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, Category="BF|Ground")
	float GroundedDotThreshold = 0.6f; // Up(0,0,1)과의 dot

	// 현재 바닥 상태(애님/상태머신용)
	UFUNCTION(BlueprintPure, Category="BF|Move")
	bool IsGrounded() const { return bGrounded; }

	UFUNCTION(BlueprintPure, Category="BF|Move")
	FVector GetGroundNormal() const { return GroundNormal; }

private:
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> Prim = nullptr;

	FBFMoveInputNet CurrentInput;
	uint8 PrevButtons = 0;

	bool bGrounded = false;
	FVector GroundNormal = FVector::UpVector;

private:
	void CachePrimitive();
	void UpdateGroundInfo();
	void ApplyForces(float DeltaTime);

	bool ShouldSimulatePhysicsMove() const;
};

