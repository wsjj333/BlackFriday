#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BFTeamComponent.generated.h"

/**
 * Team을 다루는 컴포넌트입니다.
 * 
 * 멀티플레이에서 같은 팀인지 구분하는 기준이 필요하므로 플레이어와 카트 등에서 이 컴포넌트를 부착해야 합니다.
 * 
 * Typical Usage:
 * - Pawn이나 Actor에 이 컴포넌트를 부착합니다.
 * - 서버에서 Setter를 호출해 해당 Pawn 혹은 Actor의 {@code TeamId}를 지정합니다.
 * - 같은 팀인지 판별해야 할 경우 Getter를 호출해 {@code TeamId}가 같은지 확인합니다.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLACKFRIDAY_API UBFTeamComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBFTeamComponent();
	
	uint8 GetTeamId() const;
	
	void SetTeamId(const uint8 InTeamId);

protected:
	uint8 TeamId;
};
