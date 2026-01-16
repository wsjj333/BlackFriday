#include "Character/Common/BFTeamComponent.h"

UBFTeamComponent::UBFTeamComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	TeamId = 0;
}

uint8 UBFTeamComponent::GetTeamId() const
{
	return TeamId;
}

void UBFTeamComponent::SetTeamId(const uint8 InTeamId)
{
	TeamId = InTeamId;
}

