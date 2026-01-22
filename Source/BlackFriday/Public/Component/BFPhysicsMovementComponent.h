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

	void ApplyInputImmediately(const FBFMoveInputNet& InInput);
	
	// Debug
	UPROPERTY(EditAnywhere, Category="BF|Debug")
	bool bDebugMove = false;

	UPROPERTY(EditAnywhere, Category="BF|Debug", meta=(ClampMin="0.01"))
	float DebugInterval = 0.25f;

	// 메시 옵션
	UPROPERTY(EditAnywhere, Category="BF|Mesh")
	bool bEnableUpperBodyRagdoll = false;

	UPROPERTY(EditAnywhere, Category="BF|Mesh")
	FName UpperBodyStartBone = TEXT("spine_01");

	UPROPERTY(EditAnywhere, Category="BF|Mesh")
	float UpperBodyBlendWeight = 0.5f;

	// 외부에서 물리로 움직일 대상(UpdatedComponent)을 지정하지 않으면 RootPrimitive를 자동 사용 시도
	UPROPERTY(EditAnywhere, Category="BF|Move")
	TObjectPtr<UPrimitiveComponent> PhysicsPrimitiveOverride = nullptr;

	// 튜닝
	UPROPERTY(EditAnywhere, Category="BF|Move")
	float MoveForce = 180000.f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float AirControl = 0.35f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float MaxSpeed = 800.f; // 최대 속도 제한

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float JumpImpulse = 420.f;

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float MovingLinearDamping = 1.0f; // 움직일 때 마찰력

	UPROPERTY(EditAnywhere, Category="BF|Move")
	float BrakingLinearDamping = 10.0f; // 멈출 때 마찰력 (급정거용)

	UPROPERTY(EditAnywhere, Category="BF|Ground")
	float GroundTraceLength = 120.f;

	UPROPERTY(EditAnywhere, Category="BF|Ground")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, Category="BF|Ground")
	float GroundedDotThreshold = 0.6f; // Up(0,0,1)과의 dot

	// 현재 바닥 상태
	UPROPERTY(BlueprintReadOnly, Category="BF|Ground")
	bool bGrounded = false;

	UPROPERTY(BlueprintReadOnly, Category="BF|Ground")
	FVector GroundNormal = FVector::UpVector;

	UFUNCTION(BlueprintCallable, Category="BF|Motion")
	FVector GetBFVelocity() const;
	
protected:
	// 캐싱된 물리 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> Prim = nullptr;

	void CachePrimitive();

private:
	float MoveX = 0.f;
	float MoveY = 0.f;
	float InputYawDeg = 0.f;

	bool bJumpHeld = false;
	bool bPrevJumpHeld = false;
	bool bJumpJustPressed = false;

	float DebugAcc = 0.f;
	FVector SmoothAnimVelocity = FVector::ZeroVector;
	float JumpBufferTime = 0.f;
	float JumpCooldownTime = 0.f;
};