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

	// BP에서 EnhancedInput 값을 꽂아주면 됨
	UFUNCTION(BlueprintCallable, Category="BF|NetInput")
	void SetMoveInput(FVector2D Move);

	UFUNCTION(BlueprintCallable, Category="BF|NetInput")
	void SetControlYawDegrees(float YawDegrees);

	UFUNCTION(BlueprintCallable, Category="BF|NetInput")
	void SetJumpHeld(bool bHeld);

	// 디버그
	FBFPhysicsState GetLastServerState() const { return RepState; }

	UFUNCTION(BlueprintPure)
	void GetLastServerStateBP(FVector& Loc, FRotator& OutRot, FVector& OutLinVel, FVector& OutAngVelDeg, float& OutServerTime) const;
	
	// 움직일 대상(물리 Primitive)을 지정(없으면 RootPrimitive 자동)
	UPROPERTY(EditAnywhere, Category="BF|Net")
	TObjectPtr<UPrimitiveComponent> PhysicsPrimitiveOverride = nullptr;

	// 튜닝
	UPROPERTY(EditAnywhere, Category="BF|Net")
	float InputSendHz = 30.f;

	UPROPERTY(EditAnywhere, Category="BF|Net")
	float StateSendHz = 15.f;

	UPROPERTY(EditAnywhere, Category="BF|Net|Correction")
	float TeleportDist = 200.f;

	UPROPERTY(EditAnywhere, Category="BF|Net|Correction")
	float VelCorrectGain = 6.f;

	UPROPERTY(EditAnywhere, Category="BF|Net|Correction")
	float VelLerpSpeed = 10.f;

	UPROPERTY(EditAnywhere, Category="BF|Net|RemoteSmoothing")
	float RemoteInterpSpeed = 12.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBFPhysicsMovementComponent> MoveComp = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> Prim = nullptr;

	// 로컬 입력
	FVector2D LocalMove = FVector2D::ZeroVector;
	float LocalYaw = 0.f;
	bool bLocalJumpHeld = false;

	uint16 ClientFrameCounter = 0;

	// 서버에 마지막으로 보낸 입력(최적화용)
	FBFMoveInputNet LastSentInput;

	// 서버가 받은 입력(서버에서만 의미)
	FBFMoveInputNet ServerInput;

	// 상태 복제
	UPROPERTY(ReplicatedUsing=OnRep_PhysicsState)
	FBFPhysicsState RepState;

	UFUNCTION()
	void OnRep_PhysicsState();

	// 원격 보간용
	FBFPhysicsState PrevState;
	FBFPhysicsState TargetState;
	float SmoothAlpha = 1.f;

	// 타이밍
	float InputSendAccum = 0.f;
	float StateSendAccum = 0.f;

	// 현재 프록시 모드 관리
	bool bAppliedProxyPhysicsOff = false;

private:
	void CacheRefs();
	void UpdateProxyPhysicsMode();

	FBFMoveInputNet BuildInputPacket() const;
	FBFPhysicsState BuildState() const;

	void ApplyLocalCorrection(const FBFPhysicsState& S);
	void ApplyRemoteSmoothing(float DeltaTime);

	// RPC
	UFUNCTION(Server, Unreliable)
	void ServerReceiveInput(FBFMoveInputNet InInput);

	void ServerReceiveInput_Implementation(FBFMoveInputNet InInput);
};
