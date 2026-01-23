// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Gunner/PredictedMovementPawn.h"

// Sets default values
APredictedMovementPawn::APredictedMovementPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APredictedMovementPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APredictedMovementPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APredictedMovementPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

