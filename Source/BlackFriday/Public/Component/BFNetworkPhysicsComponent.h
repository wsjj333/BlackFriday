// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "BFPhysicsNetTypes.h"
#include "BFNetworkPhysicsComponent.generated.h"

class UBFPhysicsMovementComponent;
class UPrimitiveComponent;

UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFNetworkPhysicsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBFNetworkPhysicsComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// [입력 관련 함수들]
	UFUNCTION(BlueprintCallable, Category="BF|NetInput")
	void SetMoveInput(FVector2D Move);

	UFUNCTION(BlueprintCallable, Category="BF|NetInput")
	void SetControlYawDegrees(float YawDegrees);

	UFUNCTION(BlueprintCallable, Category="BF|NetInput")
	void SetJumpHeld(bool bHeld);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Net|Mode")
	bool bUseClientPrediction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Net|Mode")
	bool bDisableCollisionWhenNotSimulating = true;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReceiveInput(FBFMoveInputNet Input);

	FBFPhysicsState GetLastServerState() const { return RepState; }

	UFUNCTION(BlueprintPure, Category="BF|Net")
	void GetLastServerStateBP(FVector& OutPos, FRotator& OutRot, FVector& OutLinVel, FVector& OutAngVel, float& OutTime) const;

	// 입력 전송: 120Hz (8ms) - 대역폭 신경 안 씀
	UPROPERTY(EditAnywhere, Category="BF|Net")
	float InputSendHz = 120.f;

	// 마우스 회전: 0.0 = 변화 즉시 무조건 전송
	UPROPERTY(EditAnywhere, Category="BF|Net")
	float YawSendThresholdDeg = 0.0f;

	// 서버 상태 전송: 90Hz (매우 부드러움)
	UPROPERTY(EditAnywhere, Category="BF|Net")
	float StateSendHz = 90.f;

	// 오너/프록시 차별 없이 전부 90Hz
	UPROPERTY(EditAnywhere, Category="BF|Net")
	float OwnerStateSendHz = 90.f;

	UPROPERTY(EditAnywhere, Category="BF|Net")
	float ProxyStateSendHz = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Net|Mode")
	bool bDriveOwnerAnimFromLocalInputWhenA = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Net|Mode", meta=(ClampMin="0.0"))
	float OwnerAnimMaxSpeed = 600.f;

	// 리컨실: 5미터까지는 텔레포트 안 함 (관대함)
	UPROPERTY(EditAnywhere, Category="BF|Net|Correction")
	float TeleportDist = 500.f;

	// 보정 강도: 1.0 (매우 약하게, 부드럽게)
	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerPosCorrectGain = 1.0f;

	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerVelCorrectGain = 6.f;

	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerMaxCorrectionAccel = 6000.f;

	UPROPERTY(EditAnywhere, Category="BF|Net|Correction")
	float VelCorrectGain = 6.f;

	UPROPERTY(EditAnywhere, Category="BF|Net|Correction")
	float VelLerpSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category="BF|Net|RemoteSmoothing")
	float RemoteInterpSpeed = 20.f; // 보간 빠르게

	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerTeleportDist = 500.f;

	UFUNCTION(BlueprintCallable, Category="BF|Net")
	FVector GetReplicatedVelocity() const;
	
private:
	UPROPERTY(Transient)
	TObjectPtr<UBFPhysicsMovementComponent> MoveComp = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> Prim = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<APawn> CachedPawn = nullptr;
	
	// 로컬 입력 저장
	FVector2D LocalMove = FVector2D::ZeroVector;
	float LocalYaw = 0.f;
	bool bLocalJumpHeld = false;
	bool bJumpHoldLatched = false;

	uint16 ClientFrameCounter = 0;

	FBFMoveInputNet LastSentInput;
	FBFMoveInputNet ServerInput;

	// 상태 동기화 변수
	UPROPERTY(ReplicatedUsing=OnRep_PhysicsState)
	FBFPhysicsState RepState;

	UPROPERTY(ReplicatedUsing=OnRep_PhysicsStateOwner)
	FBFPhysicsState RepStateOwner;

	UFUNCTION()
	void OnRep_PhysicsState();

	UFUNCTION()
	void OnRep_PhysicsStateOwner();

	bool bHasOwnerState = false;
	FBFPhysicsState LastOwnerState;

	// 보간용 변수
	FBFPhysicsState PrevState;
	FBFPhysicsState TargetState;
	float SmoothAlpha = 1.f;

	void CacheRefs();
	
	// 이 함수들은 분리된 CPP 파일에 구현될 수 있음
	FBFMoveInputNet BuildInputPacket() const;
	FBFPhysicsState BuildState() const;
	
	void ApplyRemoteSmoothing(float DeltaTime);
	void ApplyOwnerReconcile(float DeltaTime);

	float InputSendAccum = 0.f;
	float OwnerStateSendAccum = 0.f;
	float ProxyStateSendAccum = 0.f;
};