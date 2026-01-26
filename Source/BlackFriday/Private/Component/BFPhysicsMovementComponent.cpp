// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/BFPhysicsMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Component/BFNetworkPhysicsComponent.h"
#include "Components/PrimitiveComponent.h"

UBFPhysicsMovementComponent::UBFPhysicsMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	
	MoveForce = 180000.f;
	BrakingLinearDamping = 10.0f;
	MovingLinearDamping = 1.0f;
	MaxSpeed = 800.f;
	GroundTraceLength = 120.f;
}

void UBFPhysicsMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	CachePrimitive();

	if (AActor* Owner = GetOwner())
	{
		USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
		
		if (Mesh && Prim)
		{
			Mesh->SetSimulatePhysics(false);
			Mesh->SetAllBodiesSimulatePhysics(false);
			Mesh->SetCollisionProfileName(TEXT("NoCollision"));
			Mesh->AttachToComponent(Prim, FAttachmentTransformRules::SnapToTargetIncludingScale);
		}
	}

	if (Prim)
	{
		SetUpdatedComponent(Prim);
		Prim->OnComponentHit.AddDynamic(this, &UBFPhysicsMovementComponent::OnComponentHit);
	}

	if (auto* NetComp = GetOwner()->FindComponentByClass<UBFNetworkPhysicsComponent>())
	{
		AddTickPrerequisiteComponent(NetComp);
	}
	
	if (Prim && Prim->GetBodyInstance())
	{
		FBodyInstance* BI = Prim->GetBodyInstance();
		BI->SetUseCCD(true);
		
		BI->PositionSolverIterationCount = 8;
		BI->VelocitySolverIterationCount = 2; 
	}
}

void UBFPhysicsMovementComponent::CachePrimitive()
{
	if (PhysicsPrimitiveOverride)
	{
		Prim = PhysicsPrimitiveOverride;
	}
	else if (AActor* Owner = GetOwner())
	{
		Prim = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
	}
}

void UBFPhysicsMovementComponent::ApplyInputImmediately(const FBFMoveInputNet& Input)
{
	if (!Prim || !Prim->IsSimulatingPhysics()) return;
	
	bool bJump = (Input.Buttons & 0x01) != 0;
	if (bJump && JumpCooldownTime <= 0.f && bGrounded)
	{
		FVector V = Prim->GetPhysicsLinearVelocity();
		V.Z = 0.f;
		Prim->SetPhysicsLinearVelocity(V);
		Prim->SetLinearDamping(0.1f);
		Prim->AddImpulse(FVector(0.f, 0.f, JumpImpulse), NAME_None, true);
		
		bGrounded = false;
		JumpCooldownTime = 0.3f;
		if (Prim->GetBodyInstance()) Prim->GetBodyInstance()->WakeInstance();
		if (GetOwner()) GetOwner()->ForceNetUpdate();
	}
}

void UBFPhysicsMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!Prim) return;
	if (!UpdatedComponent) SetUpdatedComponent(Prim);

	const bool bSimulating = Prim->IsSimulatingPhysics();
	const FVector CurrentPhysVel = bSimulating ? Prim->GetPhysicsLinearVelocity() : FVector::ZeroVector;
	const FVector Loc = Prim->GetComponentLocation();

	{
		const FVector TraceStart = Loc;
		float CheckLength = GroundTraceLength * 1.2f; 

		const FVector TraceEnd = TraceStart - (FVector::UpVector * CheckLength);
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(GetOwner());

		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, GroundTraceChannel, Params);
		if (bHit && Hit.Normal.Z > GroundedDotThreshold)
		{
			bGrounded = true;
			GroundNormal = Hit.Normal;
		}
		else
		{
			bGrounded = false;
			GroundNormal = FVector::UpVector;
		}
	}

	if (JumpBufferTime > 0.f) JumpBufferTime -= DeltaTime;
	if (JumpCooldownTime > 0.f) JumpCooldownTime -= DeltaTime;
	
	bool bHasInput = (MoveX != 0.f || MoveY != 0.f);
	if (bGrounded)
		Prim->SetLinearDamping(bHasInput ? MovingLinearDamping : BrakingLinearDamping);
	else
		Prim->SetLinearDamping(0.1f);

	if (APawn* P = Cast<APawn>(GetOwner()))
	{

		if (P->IsLocallyControlled() || P->HasAuthority()) 
		{
			if (bGrounded && bSimulating && JumpBufferTime > 0.f && JumpCooldownTime <= 0.f)
			{
				FVector V = Prim->GetPhysicsLinearVelocity();
				V.Z = 0.f;
				Prim->SetPhysicsLinearVelocity(V);
				Prim->SetLinearDamping(0.1f);
				Prim->AddImpulse(FVector(0.f, 0.f, JumpImpulse), NAME_None, true);
                
				bGrounded = false;
				JumpBufferTime = 0.f;
				JumpCooldownTime = 0.3f;
				if (Prim->GetBodyInstance()) Prim->GetBodyInstance()->WakeInstance();

				if (P->HasAuthority())
				{
					P->ForceNetUpdate();
				}
			}
		}
	}
	
	if (bHasInput && bSimulating)
    {
        FVector LocalDir(MoveX, MoveY, 0.f);
        if (LocalDir.SizeSquared() > 1.f) LocalDir.Normalize();

        const FRotator YawRot(0.f, InputYawDeg, 0.f);
        FVector WorldDir = YawRot.RotateVector(LocalDir);

        if (bGrounded)
            WorldDir = FVector::VectorPlaneProject(WorldDir, GroundNormal).GetSafeNormal();
        else
        {
            WorldDir.Z = 0.f;
            WorldDir.Normalize();
            WorldDir *= AirControl;
        }

        float CurrentSpeed2D = Prim->GetPhysicsLinearVelocity().Size2D();
        float SpeedRatio = FMath::Clamp(CurrentSpeed2D / MaxSpeed, 0.f, 1.f);
        
        float DynamicMultiplier = FMath::Lerp(AccelMultiplier, 1.0f, SpeedRatio);
        
        Prim->AddForce(WorldDir * MoveForce * DynamicMultiplier);
        
        if (bGrounded) Prim->SetLinearDamping(MovingLinearDamping);

        FVector NewVel = Prim->GetPhysicsLinearVelocity();
        float NewSpeed2D = NewVel.Size2D();
        if (NewSpeed2D > MaxSpeed)
        {
            float Scale = MaxSpeed / NewSpeed2D;
            NewVel.X *= Scale;
            NewVel.Y *= Scale;
            Prim->SetPhysicsLinearVelocity(NewVel);
        }
    }
    else if (bGrounded && bSimulating)
    {
        Prim->SetLinearDamping(BrakingLinearDamping);
        
        FVector Vel = Prim->GetPhysicsLinearVelocity();
        if (Vel.SizeSquared2D() < 100.f) // 10cm/s 미만이면
        {
            Vel.X = 0.f;
            Vel.Y = 0.f;
            Prim->SetPhysicsLinearVelocity(Vel);
        }
    }
    else
    {
        Prim->SetLinearDamping(0.1f);
    }

    if (bSimulating) 
    {
        FRotator TargetRot(0.f, InputYawDeg, 0.f);
        FRotator CurrentRot = Prim->GetComponentRotation();
        FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 15.0f);
    	
        Prim->SetWorldRotation(NewRot, false, nullptr, ETeleportType::TeleportPhysics);
    }
	
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
	{
		FVector TargetVel = FVector::ZeroVector;
		if (auto* NetComp = GetOwner()->FindComponentByClass<UBFNetworkPhysicsComponent>())
		{
			TargetVel = NetComp->GetReplicatedVelocity();
		}
		SmoothAnimVelocity = FMath::VInterpTo(SmoothAnimVelocity, TargetVel, DeltaTime, 15.0f);
	}
	else
	{
		if (Prim) SmoothAnimVelocity = Prim->GetPhysicsLinearVelocity();
	}
	
	if (bDebugMove)
	{
		DebugAcc += DeltaTime;
		if (DebugAcc >= DebugInterval)
		{
			DebugAcc = 0.f;
			UE_LOG(LogTemp, Warning, TEXT("[BFMove] Loc=%s Vel=%s JBuf=%.2f Cool=%.2f Ground=%d"),
				*Loc.ToCompactString(),
				*CurrentPhysVel.ToCompactString(),
				JumpBufferTime,
				JumpCooldownTime,
				bGrounded ? 1 : 0
			);
		}
	}
}

void UBFPhysicsMovementComponent::SetCurrentInput(const FBFMoveInputNet& InInput)
{
	MoveX = (float)InInput.MoveX / 32767.f;
	MoveY = (float)InInput.MoveY / 32767.f;
	InputYawDeg = (float)InInput.ControlYaw100 / 100.f;

	const bool bNewJumpHeld = (InInput.Buttons & 0x01) != 0;
	if (JumpCooldownTime <= 0.f && bNewJumpHeld && !bPrevJumpHeld)
	{
		JumpBufferTime = 0.2f;
	}
	bPrevJumpHeld = bNewJumpHeld;
	bJumpHeld = bNewJumpHeld;
}

FVector UBFPhysicsMovementComponent::GetBFVelocity() const
{
	if (APawn* P = Cast<APawn>(GetOwner()))
	{
		if (P->GetLocalRole() == ROLE_AutonomousProxy)
		{
			if (FMath::Abs(MoveX) > 0.01f || FMath::Abs(MoveY) > 0.01f)
			{
				return P->GetActorForwardVector() * MaxSpeed;
			}
			return FVector::ZeroVector;
		}
	}
	return SmoothAnimVelocity;
}

void UBFPhysicsMovementComponent::OnComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (NormalImpulse.SizeSquared() > 1000000.f)
		{
			GetOwner()->ForceNetUpdate();
		}
	}
	
	if (OtherActor && OtherActor != GetOwner())
	{
		if (OtherActor->IsA<APawn>() || NormalImpulse.SizeSquared() > 1000000.f)
		{
			if (APawn* P = Cast<APawn>(GetOwner()))
			{
				if (P->IsLocallyControlled())
				{
					P->ForceNetUpdate();
				}
			}
		}
	}
}
