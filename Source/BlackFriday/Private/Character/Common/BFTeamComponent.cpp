#include "Character/Common/BFTeamComponent.h"
#include "System/BFGameState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

UBFTeamComponent::UBFTeamComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	TeamId = 0;
}

void UBFTeamComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (ABFGameState* GS = World->GetGameState<ABFGameState>())
		{
			GS->OnPlayerTeamChanged.AddDynamic(this, &UBFTeamComponent::OnPlayerTeamChanged);
		}
	}

	TryInitFromGameState();
}

void UBFTeamComponent::TryInitFromGameState()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	const APlayerState* PS = OwnerPawn->GetPlayerState();
	if (!PS) return;

	const ABFGameState* GS = GetWorld()->GetGameState<ABFGameState>();
	if (!GS) return;

	const int32 PlayerId = PS->GetPlayerId();
	if (!GS->HasPlayerInfo(PlayerId)) return;

	TeamId = GS->GetPlayerTeam(PlayerId);
}

void UBFTeamComponent::OnPlayerTeamChanged(int32 ChangedPlayerId, uint8 NewTeamId)
{
	// OnRep_PlayerTeamInfos는 (-1, 255)로 전체 갱신 신호를 보냄 → GameState에서 다시 조회
	if (ChangedPlayerId == -1)
	{
		TryInitFromGameState();
		return;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return;

	const APlayerState* PS = OwnerPawn->GetPlayerState();
	if (!PS) return;

	if (PS->GetPlayerId() == ChangedPlayerId)
	{
		TeamId = NewTeamId;
	}
}

uint8 UBFTeamComponent::GetTeamId() const
{
	return TeamId;
}

void UBFTeamComponent::SetTeamId(const uint8 InTeamId)
{
	TeamId = InTeamId;
}

