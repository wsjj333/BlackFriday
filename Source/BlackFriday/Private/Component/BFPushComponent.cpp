// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/BFPushComponent.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UBFPushComponent::UBFPushComponent()
{
	// Tick 사용
	PrimaryComponentTick.bCanEverTick = true;

	// 기본 푸시 파라미터
	PushStrength = 800.0f;
	PushRange = 120.0f;
	bFlattenZ = true;

	// 현재 무시 중인 액터 없음
	CurrentIgnoredActor = nullptr;

	// 푸시 쿨타임 설정
    PushInterval = 0.05f;
    LastPushTime = 0.0f;
    ServerLastPushTime = 0.0f;
}

void UBFPushComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UBFPushComponent::TickComponent(
	float DeltaTime,
	enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // ----- 로컬 컨트롤 Pawn만 처리 -----
    AActor* Owner = GetOwner();
    APawn* OwnerPawn = Cast<APawn>(Owner);
    if (!OwnerPawn) return;
    if (!OwnerPawn->IsLocallyControlled()) return;
    
    // RootComponent는 물리/충돌 처리를 위해 PrimitiveComponent여야 함
    UPrimitiveComponent* OwnerRoot = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
    if (!OwnerRoot) return;

    // ----- 전방 Sweep 설정 -----
    FVector Start = Owner->GetActorLocation();
    FVector Forward = Owner->GetActorForwardVector();
    FVector End = Start + (Forward * PushRange);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner); // 자기 자신 제외

    // 박스 형태의 Sweep (전방 밀기 영역)
    FVector BoxHalfSize = FVector(10.0f, 40.0f, 80.0f); 
    FCollisionShape BoxShape = FCollisionShape::MakeBox(BoxHalfSize);

    bool bHit = GetWorld()->SweepSingleByChannel(
        Hit,
        Start,
        End,
        Owner->GetActorQuat(),
        ECC_Visibility,
        BoxShape,
        Params
    );

    // 디버그용 박스 시각화
    DrawDebugBox(
		GetWorld(),
		Start + (Forward * (PushRange * 0.5f)),
		BoxHalfSize,
		Owner->GetActorQuat(),
		bHit ? FColor::Green : FColor::Red,
		false,
		-1.0f,
		0,
		2.0f
	);

    // ----- 물리 오브젝트 판별 -----
    UPrimitiveComponent* HitComp = Hit.GetComponent();
    bool bIsPhysicsObject = bHit && HitComp && HitComp->IsSimulatingPhysics();

    if (bIsPhysicsObject)
    {
        AActor* HitActor = Hit.GetActor();
        
        // 이전에 밀던 액터와 다르면 IgnoreActor 설정 갱신
        if (CurrentIgnoredActor != HitActor)
        {
            if (CurrentIgnoredActor && IsValid(CurrentIgnoredActor))
            {
                OwnerRoot->IgnoreActorWhenMoving(CurrentIgnoredActor, false);
            }

            CurrentIgnoredActor = HitActor;
            OwnerRoot->IgnoreActorWhenMoving(CurrentIgnoredActor, true);
        }
    
        // ----- 로컬 푸시 쿨타임 체크 -----
        float CurrentTime = GetWorld()->GetTimeSeconds();
        if (CurrentTime - LastPushTime < PushInterval) return;
        LastPushTime = CurrentTime;
        
        // 푸시 방향 계산
        FVector PushDir = Forward;
        if (bFlattenZ)
        {
			// 수직 성분 제거 (위로 뜨는 현상 방지)
			PushDir.Z = 0.0f;
		}
        PushDir.Normalize();

        // 속도 비교 (느린 물체만 밀 수 있도록 제한)
        float MySpeed = Owner->GetVelocity().Size();
        float BoxSpeed = HitComp->GetPhysicsLinearVelocity().Size();

        if (MySpeed > 10.0f && BoxSpeed < (MySpeed * 1.3f))
        {
            FVector FinalImpulse = PushDir * PushStrength * HitComp->GetMass();

            // ----- 서버 권한 처리 -----
            if (Owner->HasAuthority())
            {
                // 서버라면 즉시 물리 적용
                HitComp->AddImpulseAtLocation(FinalImpulse, Hit.Location);
            }
            else if (Owner->GetLocalRole() == ROLE_AutonomousProxy)
            {
                // 클라이언트라면 서버 RPC로 요청
                Server_ApplyPush(HitComp, FinalImpulse, Hit.Location);
            }
        }
    }
    else
    {
        // 더 이상 밀 대상이 없으면 IgnoreActor 해제
        if (CurrentIgnoredActor)
        {
            if (IsValid(CurrentIgnoredActor))
            {
                OwnerRoot->IgnoreActorWhenMoving(CurrentIgnoredActor, false);
            }
            CurrentIgnoredActor = nullptr;
        }
    }
}

void UBFPushComponent::Server_ApplyPush_Implementation(
	UPrimitiveComponent* HitComp,
	FVector PushForce,
	FVector Location
)
{
    // 서버에서도 푸시 쿨타임 체크 (RPC 스팸 방지)
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - ServerLastPushTime < PushInterval) return;
    ServerLastPushTime = CurrentTime;

    // 최종 물리 Impulse 적용
    if (HitComp && HitComp->IsSimulatingPhysics())
    {
        HitComp->AddImpulseAtLocation(PushForce, Location);
    }
}
