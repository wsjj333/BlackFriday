#pragma once

#include "CoreMinimal.h"
// #include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "BFCartPawn.generated.h"

class ABFPusher;
class USphereComponent;
class UInputAction;
class UInputMappingContext;
class UBoxComponent;
class UStaticMeshComponent;
class USceneComponent;
class UBFTeamComponent;

/**
 * 게임모드가 CartPawn과 Pusher에 부착된 TeamComp를 이용해 팀을 지정해야 합니다
 */
UCLASS()
class BLACKFRIDAY_API ABFCartPawn : public APawn
{
	GENERATED_BODY()

public:
	ABFCartPawn();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/** 로컬 입력에서 호출(클라는 Server RPC로 위임) */
	UFUNCTION(BlueprintCallable, Category="Cart|Reset")
	void RequestUpright();
	
	// ----- Getter/Setter -----
	USceneComponent* GetPusherStandAnkerComponent() const;
	
	UFUNCTION(BlueprintCallable)
	FTransform GetHandleLTransform() const;
	
	UFUNCTION(BlueprintCallable)
	FTransform GetHandleRTransform() const;

	UFUNCTION(BlueprintCallable)
	float GetAcceleration() const;
	
	UFUNCTION(BlueprintCallable)
	FVector GetCurrentVelocity() const;
	
	// 서버에서만 호출되는 입력축 세터(컴포넌트/서버 코드용)
	void SetAccelAxis_Server(float Axis);
	void SetSteerAxis_Server(float Axis);
	void SetSteeringMultiplier_Server(const float Multiplier);
	
	USceneComponent* GetPivotComp() const { return Pivot; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ----- Physics / Movement -----
	void SuspensionCast(USceneComponent* WheelComp) const;
	bool IsOnGround() const;
	
	/** 뒤집힘 판정 */
	bool IsFlipped() const;

	/** 서버 권위에서 실제 복구 수행 */
	void DoUprightReset_ServerAuth();

	/** 서버 RPC */
	UFUNCTION(Server, Reliable)
	void Server_RequestUpright();

	// 서버에서만 호출되는 물리 적용 루틴
	void ServerSimTick(float DeltaSeconds);
	void CalculateAcceleration(float DeltaSeconds);
	void AccelerateCart() const;

	// Cosmetic (클라에서 복제값 기반으로만)
	void RotateMeshes(float DeltaSeconds);

	// ----- RPCs -----
	// UFUNCTION(Server, Reliable, WithValidation)
	// void Server_SetAccelerationAxis(float Axis);
	//
	// UFUNCTION(Server, Reliable, WithValidation)
	// void Server_SetSteeringAxis(float Axis);

	// ----- Replicated State -----
	// “플레이어가 누르고 있는” 입력축 (서버 권한)
	UPROPERTY(Replicated)
	float Rep_AccelAxis = 0.0f;

	UPROPERTY(Replicated)
	float Rep_SteerAxis = 0.0f;

	// 서버가 만든 “물리/애니메이션용 스무딩 결과”
	UPROPERTY(Replicated)
	float Rep_AccelerationInput = 0.0f;

	UPROPERTY(Replicated)
	float Rep_Acceleration = 0.0f;

	UPROPERTY(Replicated)
	float Rep_DriftSteer = 0.0f;

	UPROPERTY(Replicated)
	FRotator Rep_DriftRotation = FRotator::ZeroRotator;
	
	UPROPERTY(Replicated, EditDefaultsOnly, Category="BF|Movement")
	float Rep_SteeringMultiplier = 2.0f;

	// ----- Tunables -----
	float SpeedModifier = 1.0f;

	UPROPERTY(EditAnywhere, Category="BF|Movement")
	float DownForce = -4900000.0f;

	UPROPERTY(EditDefaultsOnly, Category="BF|Movement")
	double SteeringTorque = 1500000.0f;

	// UPROPERTY(EditAnywhere, Category="BF|Movement")
	// float SteeringMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, Category="BF|Cart")
	float SuspensionForceMultiplier = 10000000.0f;

	UPROPERTY(EditAnywhere, Category="BF|Cart")
	float WheelRadius = 18.0f;

	UPROPERTY(EditAnywhere, Category="BF|Cart")
	FVector GroundTraceEnd = FVector(0.0f, 0.0f, 150.0f);

	UPROPERTY(EditDefaultsOnly, Category="BF|Movement")
	float MaxAcceleration = 15000.0f;

	UPROPERTY(EditAnywhere, Category="BF|Movement")
	float CartSpeed = 10000.0f;
	
	FVector CurrentVelocity = FVector(0.0, 0.0, 0.0);
	
	// 클라이언트(AnimInstance/코스메틱)에서 사용할 가속도 캐시
	UPROPERTY(BlueprintReadOnly, Category="Cart|Anim", Transient)
	float Acceleration = 0.0f;
	
	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float UprightDotThreshold = 0.85f; // 약 31.8도

	/** 너무 빠르게 움직이는 중엔 복구 금지 (공중 회전/드리프트 중 오작동 방지) */
	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float MaxSpeedToAllowReset = 200.f;

	/** 복구 후 바닥에서 띄울 높이(바운딩 박스 기반으로 추가) */
	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float ExtraLift = 5.f;

	/** 라인트레이스 거리(아래로) */
	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float TraceDownDistance = 5000.f;

	/** 라인트레이스 시작 높이(위로) */
	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float TraceUpDistance = 200.f;

	/** 복구 쿨다운(연타 방지) */
	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	float ResetCooldown = 1.0f;

	/** 마지막 복구 시간(서버 기준) */
	double LastResetTimeSeconds = -1.0;

	/** 바닥 노멀에 맞춰 세울지(경사면에서 자연스럽게) */
	UPROPERTY(EditAnywhere, Category="Cart|Reset")
	bool bAlignToGroundNormal = true;

	/** 네트워크에서 요청자(주로 owner)만 요청 가능하게 제한 */
	bool CanRequestReset() const;

	// ----- Components -----
	
	UPROPERTY(BlueprintREadWrite, Category="BF|Team")
	TObjectPtr<UBFTeamComponent> TeamComp;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Root;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> CartBody;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> Pivot;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> WheelFRComp;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> WheelFLComp;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> WheelBRComp;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USceneComponent> WheelBLComp;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> WheelFRMesh;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> WheelFLMesh;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> WheelBRMesh;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> WheelBLMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CasterForkFRMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CasterForkFLMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CasterForkBRMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> CasterForkBLMesh;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USceneComponent> PusherStandAnker;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> HandleL;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> HandleR;
};
