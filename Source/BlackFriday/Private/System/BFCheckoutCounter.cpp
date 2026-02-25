// Fill out your copyright notice in the Description page of Project Settings.

#include "System/BFCheckoutCounter.h"
#include "Character/Common/BFTeamComponent.h"
#include "Component/Cart/BFCartInventoryComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

ABFCheckoutCounter::ABFCheckoutCounter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	TriggerZone = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerZone"));
	TriggerZone->SetupAttachment(RootComponent);
	TriggerZone->SetCollisionProfileName(TEXT("OverlapAll"));
	TriggerZone->SetGenerateOverlapEvents(true);

	EntryBarrier = CreateDefaultSubobject<UBoxComponent>(TEXT("EntryBarrier"));
	EntryBarrier->SetupAttachment(RootComponent);
	EntryBarrier->SetCollisionProfileName(TEXT("BlockAll"));
	EntryBarrier->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABFCheckoutCounter::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 Overlap 이벤트 처리
	if (HasAuthority())
	{
		TriggerZone->OnComponentBeginOverlap.AddDynamic(this, &ABFCheckoutCounter::OnTriggerBeginOverlap);
		TriggerZone->OnComponentEndOverlap.AddDynamic(this, &ABFCheckoutCounter::OnTriggerEndOverlap);
	}
}

void ABFCheckoutCounter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABFCheckoutCounter, OccupyingTeamId);
	DOREPLIFETIME(ABFCheckoutCounter, bIsActive);
}

void ABFCheckoutCounter::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !OtherActor || !bIsActive) return;

	UBFTeamComponent* TC = OtherActor->FindComponentByClass<UBFTeamComponent>();
	if (!TC) return;

	uint8 IncomingTeam = TC->GetTeamId();

	if (OccupyingTeamId == 255)
	{
		// 빈 카운터 - 점유 획득
		OccupyingTeamId = IncomingTeam;
		EntryBarrier->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ActorsInZone.Add(OtherActor);
		OnRep_OccupyingTeamId();

		UE_LOG(LogTemp, Log, TEXT("[BFCheckoutCounter] %s - Team %d occupied"), *GetName(), IncomingTeam);
	}
	else if (OccupyingTeamId == IncomingTeam)
	{
		// 같은 팀 - 추가 진입 허용
		ActorsInZone.Add(OtherActor);
	}
	else
	{
		// 다른 팀 - 밀어내기
		FVector PushDir = OtherActor->GetActorLocation() - GetActorLocation();
		PushDir.Z = 0.0f;
		PushDir = PushDir.GetSafeNormal();

		if (ACharacter* Char = Cast<ACharacter>(OtherActor))
		{
			Char->LaunchCharacter(PushDir * 800.0f + FVector(0.0f, 0.0f, 300.0f), true, true);
		}
		else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(OtherActor->GetRootComponent()))
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				PrimComp->AddImpulse(PushDir * 600000.0f);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[BFCheckoutCounter] %s - Team %d rejected (occupied by Team %d)"),
			*GetName(), IncomingTeam, OccupyingTeamId);
	}
}

void ABFCheckoutCounter::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority() || !OtherActor) return;

	ActorsInZone.Remove(OtherActor);

	// 점유 팀의 모든 액터가 나갔으면 점유 해제
	if (OccupyingTeamId != 255 && IsOccupyingTeamGone())
	{
		ResetOccupation();
	}
}

bool ABFCheckoutCounter::IsOccupyingTeamGone() const
{
	for (const TObjectPtr<AActor>& Actor : ActorsInZone)
	{
		if (!Actor) continue;
		UBFTeamComponent* TC = Actor->FindComponentByClass<UBFTeamComponent>();
		if (TC && TC->GetTeamId() == OccupyingTeamId)
		{
			return false;
		}
	}
	return true;
}

float ABFCheckoutCounter::ProcessCheckout()
{
	if (!HasAuthority()) return 0.0f;
	if (OccupyingTeamId == 255 || !bIsActive) return 0.0f;

	float Amount = GetTeamCartTotalPrice(OccupyingTeamId);
	OnCheckoutProcessed.Broadcast(OccupyingTeamId, Amount);

	UE_LOG(LogTemp, Log, TEXT("[BFCheckoutCounter] %s - Checkout Team %d: %.0f원"), *GetName(), OccupyingTeamId, Amount);
	return Amount;
}

float ABFCheckoutCounter::GetTeamCartTotalPrice_Implementation(uint8 TeamId)
{
	// TriggerZone 안의 액터 중 해당 팀의 BFCartInventoryComponent 탐색
	// TODO: 인벤토리 시스템 확정 후 BP_CheckoutCounter에서 override 예정
	for (AActor* Actor : ActorsInZone)
	{
		if (!IsValid(Actor)) continue;

		UBFTeamComponent* TC = Actor->FindComponentByClass<UBFTeamComponent>();
		if (!TC || TC->GetTeamId() != TeamId) continue;

		UBFCartInventoryComponent* InvComp = Actor->FindComponentByClass<UBFCartInventoryComponent>();
		if (InvComp)
		{
			return InvComp->GetTotalPrice();
		}
	}
	return 0.0f;
}

void ABFCheckoutCounter::ResetOccupation()
{
	if (!HasAuthority()) return;

	OccupyingTeamId = 255;
	EntryBarrier->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OnRep_OccupyingTeamId(); // OnCounterReleased 브로드캐스트

	UE_LOG(LogTemp, Log, TEXT("[BFCheckoutCounter] %s - Released"), *GetName());
}

void ABFCheckoutCounter::Deactivate()
{
	if (!HasAuthority()) return;

	bIsActive = false;
	TriggerZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EntryBarrier->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ActorsInZone.Empty();
	OccupyingTeamId = 255;
	OnRep_bIsActive();

	UE_LOG(LogTemp, Log, TEXT("[BFCheckoutCounter] %s - Deactivated"), *GetName());
}

void ABFCheckoutCounter::OnRep_OccupyingTeamId()
{
	if (OccupyingTeamId != 255)
	{
		OnCounterOccupied.Broadcast(this, OccupyingTeamId);
	}
	else
	{
		OnCounterReleased.Broadcast(this);
	}
}

void ABFCheckoutCounter::OnRep_bIsActive()
{
	// BP에서 바인딩: 셔터 닫기 애니메이션, 비활성 표시 등
}
