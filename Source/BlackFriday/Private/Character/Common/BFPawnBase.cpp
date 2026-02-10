// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Common/BFPawnBase.h"

#include "Components/CapsuleComponent.h"

ABFPawnBase::ABFPawnBase()
{
	bReplicates = true;

	// 캡슐 컴포넌트 (루트, 물리 시뮬레이션)
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(42.f, 110.f);
	CapsuleComp->SetCollisionProfileName(TEXT("PhysicsActor"));
	CapsuleComp->SetSimulatePhysics(true);
	CapsuleComp->SetEnableGravity(true);
	CapsuleComp->BodyInstance.bLockXRotation = true;
	CapsuleComp->BodyInstance.bLockYRotation = true;
	CapsuleComp->BodyInstance.SetMassOverride(80.f);
	SetRootComponent(CapsuleComp);
	
	// 스켈레탈 메시 컴포넌트
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	Mesh->SetupAttachment(CapsuleComp);
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, -110.f));
	Mesh->SetRelativeRotation(FRotator(0.f, -90.0f, 0.0f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->bEditableWhenInherited = true;
}

USkeletalMeshComponent* ABFPawnBase::GetMesh() const
{
	return Mesh;
}
