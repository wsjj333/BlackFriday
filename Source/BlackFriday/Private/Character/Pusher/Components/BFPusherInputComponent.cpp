// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Pusher/Components/BFPusherInputComponent.h"

// Sets default values for this component's properties
UBFPusherInputComponent::UBFPusherInputComponent()
{
}

void UBFPusherInputComponent::BindInput(UInputComponent* PlayerInputComponent)
{
}

ABFPusher* UBFPusherInputComponent::GetOwnerPusher() const
{
}

void UBFPusherInputComponent::HandleMoveInput(const FInputActionValue& Value)
{
}

void UBFPusherInputComponent::HandleLookInput(const FInputActionValue& Value)
{
}

void UBFPusherInputComponent::OnJumpPressed(const FInputActionValue& Value)
{
}

void UBFPusherInputComponent::OnJumpReleased(const FInputActionValue& Value)
{
}

void UBFPusherInputComponent::OnToggleDriveModePressed(const FInputActionValue& Value)
{
}

void UBFPusherInputComponent::OnAccelTriggered(const FInputActionValue& Value)
{
}

void UBFPusherInputComponent::OnAccelEnded(const FInputActionValue& Value)
{
}

void UBFPusherInputComponent::OnSteerTriggered(const FInputActionValue& Value)
{
}

void UBFPusherInputComponent::OnSteerEnded(const FInputActionValue& Value)
{
}

void UBFPusherInputComponent::OnDriftStarted(const FInputActionValue& Value)
{
}

void UBFPusherInputComponent::OnDriftEnded(const FInputActionValue& Value)
{
}

void UBFPusherInputComponent::AddMappingContextIfLocal()
{
}


// Called when the game starts
void UBFPusherInputComponent::BeginPlay()
{
	Super::BeginPlay();
}

