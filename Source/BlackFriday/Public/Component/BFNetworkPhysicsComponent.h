#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BFPhysicsNetTypes.h"
#include "Interfaces/BFInputSink.h"
#include "BFNetworkPhysicsComponent.generated.h"

class UBFPhysicsMovementComponent;
class UPrimitiveComponent;

/**
 * 물리 기반 Pawn을 위한 네트워크 동기화 컴포넌트
 *
 * 역할 요약:
 * - 로컬 입력 수집 및 서버 전송
 * - 서버 물리 상태를 Owner / Proxy에 맞게 복제
 * - Owner: 리컨실(Reconcile)
 * - Proxy: 보간(Smoothing)
 */
UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFNetworkPhysicsComponent : public UActorComponent, public IBFInputSink
{
	GENERATED_BODY()

public:
	UBFNetworkPhysicsComponent();

	/** 컴포넌트 초기화 */
	virtual void BeginPlay() override;

	/** PrePhysics 단계에서 네트워크/물리 동기화 처리 */
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	/** Replication 변수 등록 */
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps
	) const override;

	// =====================
	// 입력 (로컬 → 서버)
	// =====================

	/** 이동 입력 (XY 평면) */
	UFUNCTION(BlueprintCallable, Category="BF|NetInput")
	virtual void SetMoveInput(FVector2D Move) override;

	/** 컨트롤 기준 Yaw */
	UFUNCTION(BlueprintCallable, Category="BF|NetInput")
	virtual void SetControlYawDegrees(float YawDegrees) override;

	/** 점프 버튼 홀드 여부 */
	UFUNCTION(BlueprintCallable, Category="BF|NetInput")
	virtual void SetJumpHeld(bool bHeld) override;

	/** 로컬 클라이언트 예측 사용 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Net|Mode")
	bool bUseClientPrediction = true;

	/** 시뮬레이션 안 할 때 충돌 제거 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BF|Net|Mode")
	bool bDisableCollisionWhenNotSimulating = true;

	/**
	 * 클라이언트 → 서버 입력 전송
	 * - Unreliable: 매 프레임 보내므로 신뢰성 불필요
	 */
	UFUNCTION(Server, Unreliable, WithValidation)
	virtual void ServerReceiveInput(FBFMoveInputNet Input) override;

	// =====================
	// 상태 조회
	// =====================
	
	// CMC와 동일한 의미의 "월드 기준 이동 입력 벡터"
	// - 길이: 0~1
	// - 방향: 이동 의도
	FVector GetMoveInputWorldSpace() const;

	/** 마지막 서버 상태 (C++용) */
	FBFPhysicsState GetLastServerState() const { return RepState; }

	/** 마지막 서버 상태 (BP용) */
	UFUNCTION(BlueprintPure, Category="BF|Net")
	void GetLastServerStateBP(
		FVector& OutPos,
		FRotator& OutRot,
		FVector& OutLinVel,
		FVector& OutAngVel,
		float& OutTime
	) const;

	// =====================
	// 네트워크 주기 설정
	// =====================

	/** 입력 전송 빈도 (Hz) */
	UPROPERTY(EditAnywhere, Category="BF|Net")
	float InputSendHz = 120.f;

	/** Owner에게 상태 전송 빈도 */
	UPROPERTY(EditAnywhere, Category="BF|Net")
	float OwnerStateSendHz = 120.f;

	/** Proxy에게 상태 전송 빈도 */
	UPROPERTY(EditAnywhere, Category="BF|Net")
	float ProxyStateSendHz = 120.f;
	
	// =====================
	// 보정 / 리컨실 설정
	// =====================

	/** 이 거리 이상이면 순간이동 */
	UPROPERTY(EditAnywhere, Category="BF|Net|Correction")
	float TeleportDist = 500.f;

	/** 위치 보정 강도 */
	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerPosCorrectGain = 6.0f;

	/** 속도 보정 강도 */
	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerVelCorrectGain = 6.f;

	/** 최대 보정 가속도 */
	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerMaxCorrectionAccel = 6000.f;

	/** 서버 속도 → 현재 속도 보간 속도 */
	UPROPERTY(EditAnywhere, Category="BF|Net|Correction")
	float VelLerpSpeed = 10.f;

	/** 원격 Pawn 보간 속도 */
	UPROPERTY(EditAnywhere, Category="BF|Net|RemoteSmoothing")
	float RemoteInterpSpeed = 40.f;

	/** 오너 순간이동 임계값 */
	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerTeleportDist = 500.f;
	
	/** 보정 무시 데드존 */
	UPROPERTY(EditAnywhere, Category="BF|Net|OwnerReconcile")
	float OwnerDeadZone = 10.0f;
	
	// =====================
	// 유틸
	// =====================

	/** 서버에서 복제된 속도 */
	UFUNCTION(BlueprintCallable, Category="BF|Net")
	FVector GetReplicatedVelocity() const;

	/** 네트워크 중요 상황 토글 */
	UFUNCTION(BlueprintCallable, Category="BF|Net")
	void SetHighPriorityMode(bool bEnable);

private:
	// =====================
	// 캐시된 참조
	// =====================

	UPROPERTY(Transient)
	TObjectPtr<UBFPhysicsMovementComponent> MoveComp = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> Prim = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<APawn> CachedPawn = nullptr;
	
	// =====================
	// 로컬 입력 상태
	// =====================

	FVector2D LocalMove = FVector2D::ZeroVector;
	float LocalYaw = 0.f;
	bool bLocalJumpHeld = false;
	bool bJumpHoldLatched = false;

	/** 클라이언트 프레임 카운터 */
	uint16 ClientFrameCounter = 0;

	uint16 LastRecvClientFrame = 0;
	bool bHasRecvClientFrame = false;
	
	/** 서버에서 사용하는 최신 입력 */
	FBFMoveInputNet ServerInput;

	// =====================
	// 상태 복제
	// =====================

	/** Owner 제외 모든 클라이언트 */
	UPROPERTY(ReplicatedUsing=OnRep_PhysicsState)
	FBFPhysicsState RepState;

	/** Owner 전용 */
	UPROPERTY(ReplicatedUsing=OnRep_PhysicsStateOwner)
	FBFPhysicsState RepStateOwner;

	UFUNCTION()
	void OnRep_PhysicsState();

	UFUNCTION()
	void OnRep_PhysicsStateOwner();

	bool bHasOwnerState = false;
	FBFPhysicsState LastOwnerState;

	// =====================
	// 보간용 상태
	// =====================

	FBFPhysicsState PrevState;
	FBFPhysicsState TargetState;
	float SmoothAlpha = 1.f;

	// =====================
	// 내부 로직
	// =====================

	void CacheRefs();
	
	FBFMoveInputNet BuildInputPacket() const;
	FBFPhysicsState BuildState() const;
	
	void ApplyRemoteSmoothing(float DeltaTime);
	void ApplyOwnerReconcile(float DeltaTime);
	void ApplyStateWithTeleportCheck(const FBFPhysicsState& NewState);

	/** 주기 누적용 타이머 */
	float InputSendAccum = 0.f;
	float OwnerStateSendAccum = 0.f;
	float ProxyStateSendAccum = 0.f;
};
