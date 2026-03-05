#pragma once

#include "CoreMinimal.h"
// #include "InputActionValue.h"
#include "GameFramework/Pawn.h"
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

	// 애니메이션 즉각 반응(로컬 예측)을 위한 코스메틱 입력 세터 추가
	void SetCosmeticAccelInput(float Axis);
	
	/** 로컬 입력에서 호출(클라는 Server RPC로 위임) */
	UFUNCTION(BlueprintCallable, Category="Cart|Reset")
	void RequestUpright();
	
	// ----- Getter/Setter -----
	USceneComponent* GetPusherStandAnchorComponent() const;
	
	UFUNCTION(BlueprintCallable)
	FTransform GetHandleLTransform() const;
	
	UFUNCTION(BlueprintCallable)
	FTransform GetHandleRTransform() const;

	UFUNCTION(BlueprintCallable)
	float GetAcceleration() const;
	
	UFUNCTION(BlueprintCallable)
	FVector GetCurrentVelocity() const;
	
	// 클라이언트 로컬 예측용 입력 세터
	void SetAccelAxis_Local(float Axis);
	void SetSteerAxis_Local(float Axis);
	void SetDriving_Local(bool bDriving);
	
	// 서버에서만 호출되는 입력축 세터(컴포넌트/서버 코드용)
	void SetAccelAxis_Server(float Axis);
	void SetSteerAxis_Server(float Axis);
	void SetSteeringMultiplier_Server(const float Multiplier);
	
	USceneComponent* GetPivotComp() const { return Pivot; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
	// 로컬 애니메이션용 코스메틱 가속 축
	float Cosmetic_AccelInput = 0.0f;
	
	// 애니메이션 속도 계산을 위해 이전 프레임 위치를 저장할 변수
	FVector LastTickLocation = FVector::ZeroVector;
	
	// 현재 이 클라이언트가 운전 중인지 여부 (로컬 전용 플래그)
	bool bIsLocallyDriven = false;

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

	// 물리 적용
	void ServerSimTick(float DeltaSeconds);
	void CalculateAcceleration(float DeltaSeconds);
	void AccelerateCart() const;

	// Cosmetic (클라에서 복제값 기반으로만)
	void RotateMeshes(float DeltaSeconds);

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

	// ===== 튜닝 가능한 카트 조작 관련 파라미터 =====
	
	/** 카트 이동 속도 계수(부스트 시 값을 높여줌) */
	UPROPERTY(EditAnywhere, Category="BF|Movement")
	float SpeedModifier = 1.0f;

	/** 카트가 공중에 떴을 때 긴 시간 떠 있는 것을 방지하기 위해 아래로 눌러주는 힘 */
	UPROPERTY(EditAnywhere, Category="BF|Movement")
	float DownForce = -4900000.0f;

	/** 회전 힘 */
	UPROPERTY(EditAnywhere, Category="BF|Movement")
	double SteeringTorque = 15000000.0f;
	
	/** 카트 최대 가속도 */
	UPROPERTY(EditAnywhere, Category="BF|Movement")
	float MaxAcceleration = 15000.0f;

	/** 카트 이동 속도 */
	UPROPERTY(EditAnywhere, Category="BF|Movement")
	float CartSpeed = 10000.0f;
	
	/** 드리프트 중일때 얼마나 더 큰 각도로 꺾을지 결정하는 계수 */
	UPROPERTY(Replicated, EditAnywhere, Category="BF|Movement")
	float Rep_SteeringMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, Category="BF|Cart")
	float SuspensionForceMultiplier = 10000000.0f;

	UPROPERTY(EditAnywhere, Category="BF|Cart")
	float WheelRadius = 18.0f;

	UPROPERTY(EditAnywhere, Category="BF|Cart")
	FVector GroundTraceEnd = FVector(0.0f, 0.0f, 150.0f);
	
	FVector CurrentVelocity = FVector(0.0, 0.0, 0.0);
	
	/** 클라이언트(AnimInstance/코스메틱)에서 사용할 가속도 캐시 */
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
	
	UPROPERTY(EditAnywhere, Category="BF|Cart")
	float UprightTorqueStrength = 500.0f; // 넘어질 때의 복원력

	/** 네트워크에서 요청자(주로 owner)만 요청 가능하게 제한 */
	bool CanRequestReset() const;

	// ----- Components -----
	
	UPROPERTY(BlueprintReadWrite, Category="BF|Team")
	TObjectPtr<UBFTeamComponent> TeamComp;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> Root;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> BasketLeftWallCollision;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> BasketRightWallCollision;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> BasketFrontWallCollision;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> BasketBackWallCollision;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UBoxComponent> BasketFloorCollision;

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
	TObjectPtr<USceneComponent> PusherStandAnchor;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> HandleL;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> HandleR;
};
