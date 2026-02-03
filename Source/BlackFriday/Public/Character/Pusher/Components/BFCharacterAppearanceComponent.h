#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Enums/BFCharacterType.h"
#include "BFCharacterAppearanceComponent.generated.h"

class ABFCharacterBase;
class USkeletalMesh;
class USkeletalMeshComponent;

UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFCharacterAppearanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBFCharacterAppearanceComponent();

	// ---- Public API ----
	UFUNCTION(BlueprintCallable, Category="BF|Appearance")
	void SetCharacterType(EBFCharacterType NewType);

	UFUNCTION(BlueprintCallable, Category="BF|Appearance")
	EBFCharacterType GetCharacterType() const { return CharacterType; }

	/** BP에서 데이터 세팅 편하게 하려고 열어둠(원하면 EditDefaultsOnly로 좁혀도 됨) */
	UPROPERTY(EditDefaultsOnly, Category="BF|Appearance")
	TMap<EBFCharacterType, TSoftObjectPtr<USkeletalMesh>> CharacterMeshMap;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// ---- Owner cache ----
	UPROPERTY(Transient)
	TObjectPtr<ABFCharacterBase> OwnerCharacter;

	ABFCharacterBase* GetOwnerCharacter() const;

	// ---- Replicated State ----
	UPROPERTY(ReplicatedUsing=OnRep_CharacterType, EditDefaultsOnly, Category="BF|Appearance")
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
