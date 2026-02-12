// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "BFPhysicsNetTypes.h"
#include "Interfaces/BFInputSink.h"
#include "BFNetworkPhysicsComponent.generated.h"

class UBFPhysicsMovementComponent;
class UPrimitiveComponent;

UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFNetworkPhysicsComponent : public UActorComponent, public IBFInputSink
{
	GENERATED_BODY()

public:
	UBFNetworkPhysicsComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 입력
	UFUNCTION(BlueprintCallable, Category="BF|NetInput")
	virtual void SetMoveInput(FVector2D Move) override;

	UFUNCTION(BlueprintCallable, Category="BF|NetInput")
	virtual void SetControlYawDegrees(float YawDegrees) override;

	UFUNCTION(BlueprintCallable, Category="BF|NetInput")
	virtual void SetJumpHeld(bool bHeld) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Net|Mode")
	bool bUseClientPrediction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Net|Mode")
	bool bDisableCollisionWhenNotSimulating = true;

	UFUNCTION(Server, Unreliable, WithValidation)
	virtual void ServerReceiveInput(FBFMoveInputNet Input) override;

	FBFPhysicsState GetLastServerState() const { return RepState; }

	UFUNCTION(BlueprintPure, Category="BF|Net")
	void GetLastServerStateBP(FVector& OutPos, FRotator& OutRot, FVector& OutLinVel, FVector& OutAngVel, float& OutTime) const;

	// 입력 전송
	UPROPERTY(EditAnywhere, Category="BF|Net")
	float InputSendHz = 120.f;

	// 서버 상태 전송
	UPROPERTY(EditAnywhere, Category="BF|Net")
	float OwnerStateSendHz = 120.f;

	UPROPERTY(EditAnywhere, Category="BF|Net")
	float ProxyStateSendHz = 120.f;
	
	// 리컨실
	UPROPERTY(EditAnywhere, Category="BF|Net|Correction")
	float TeleportDist = 500.f;

	// 보정 강도
	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerPosCorrectGain = 6.0f;

	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerVelCorrectGain = 6.f;

	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerMaxCorrectionAccel = 6000.f;

	UPROPERTY(EditAnywhere, Category="BF|Net|Correction")
	float VelLerpSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category="BF|Net|RemoteSmoothing")
	float RemoteInterpSpeed = 40.f;

	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerTeleportDist = 500.f;
	
	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerDeadZone = 10.0f;
	
	UFUNCTION(BlueprintCallable, Category="BF|Net")
	FVector GetReplicatedVelocity() const;

	UFUNCTION(BlueprintCallable, Category="BF|Net")
	void SetHighPriorityMode(bool bEnable);
	
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

	uint16 LastRecvClientFrame = 0;
	bool bHasRecvClientFrame = false;
	
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
	
	FBFMoveInputNet BuildInputPacket() const;
	FBFPhysicsState BuildState() const;
	
	void ApplyRemoteSmoothing(float DeltaTime);
	void ApplyOwnerReconcile(float DeltaTime);
	void ApplyStateWithTeleportCheck(const FBFPhysicsState& NewState);

	float InputSendAccum = 0.f;
	float OwnerStateSendAccum = 0.f;
	float ProxyStateSendAccum = 0.f;
};