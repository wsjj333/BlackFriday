#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Enums/BFCharacterType.h"
#include "BFCharacterAppearanceComponent.generated.h"

class ABFPawnBase;
class USkeletalMesh;
class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAppearanceApplied, EBFCharacterType, AppliedType);

UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFCharacterAppearanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBFCharacterAppearanceComponent();

	// ---- Public API ----
	UFUNCTION(BlueprintCallable, Category="BF|Visual")
	void SetCharacterType(EBFCharacterType NewType);

	UFUNCTION(BlueprintCallable, Category="BF|Visual")
	EBFCharacterType GetCharacterType() const { return CharacterType; }

	/** BP에서 데이터 세팅 편하게 하려고 열어둠(원하면 EditDefaultsOnly로 좁혀도 됨) */
	UPROPERTY(EditDefaultsOnly, Category="BF|Visual")
	TMap<EBFCharacterType, TSoftObjectPtr<USkeletalMesh>> CharacterMeshMap;
	
	/** 외형(메시) 적용이 끝났을 때(서버/클라 모두) 호출되는 이벤트 */
	UPROPERTY(BlueprintAssignable, Category="BF|Visual")
	FOnAppearanceApplied OnAppearanceApplied;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// ---- Owner cache ----
	UPROPERTY(Transient)
	TObjectPtr<ABFPawnBase> OwnerCharacter;

	ABFPawnBase* GetOwnerCharacter() const;

	// ---- Replicated State ----
	UPROPERTY(ReplicatedUsing=OnRep_CharacterType, EditDefaultsOnly, Category="BF|Visual")
	EBFCharacterType CharacterType = EBFCharacterType::AfroHairMan;

	// ---- Rep Notify ----
	UFUNCTION()
	void OnRep_CharacterType();

	// ---- RPC ----
	UFUNCTION(Server, Reliable)
	void ServerSetCharacterType(EBFCharacterType NewType);

	// ---- Apply ----
	void ApplyCharacterType(EBFCharacterType TypeToApply);
	bool IsValidCharacterType(EBFCharacterType Type) const;

	USkeletalMeshComponent* ResolveMeshComponent() const;
};
