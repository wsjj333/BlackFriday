#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Enums/BFCharacterType.h"
#include "BFCharacterAppearanceComponent.generated.h"

class ABFPawnBase;
class UBFCharacterAppearanceData;
class USkeletalMesh;
class USkeletalMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAppearanceApplied, EBFCharacterType, AppliedType);

UCLASS(ClassGroup=(BF), meta=(BlueprintSpawnableComponent))
class BLACKFRIDAY_API UBFCharacterAppearanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// ----- Constructor -----
	UBFCharacterAppearanceComponent();

	// ---- Public API ----
	UFUNCTION(BlueprintCallable, Category="BF|Visual")
	void SetCharacterType(EBFCharacterType NewType);

	UFUNCTION(BlueprintPure, Category="BF|Visual")
	EBFCharacterType GetCharacterType() const { return CharacterType; }
	
	ABFPawnBase* GetOwnerCharacter() const { return OwnerCharacter.Get(); }

	// ----- Events -----
	UPROPERTY(BlueprintAssignable, Category="BF|Visual")
	FOnAppearanceApplied OnAppearanceApplied;
	
	// ----- Config -----
	UPROPERTY(EditDefaultsOnly, Category="BF|Visual")
	TObjectPtr<UBFCharacterAppearanceData> AppearanceData;

protected:
	// ----- UE Lifecycle -----
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// ---- Replicated State ----
	UPROPERTY(ReplicatedUsing=OnRep_CharacterType, VisibleInstanceOnly, Category="BF|Visual")
	EBFCharacterType CharacterType = EBFCharacterType::AfroHairMan;

	// ---- Rep Notify ----
	UFUNCTION()
	void OnRep_CharacterType();

	// ---- RPC ----
	UFUNCTION(Server, Reliable)
	void ServerSetCharacterType(EBFCharacterType NewType);

	// ----- Apply Logic -----
	void ApplyCharacterTypeInternal(EBFCharacterType Type);
	// 실패 시 다음 틱 재시도 (JIP/초기화 레이스 대응)
	void ApplyCharacterType(EBFCharacterType TypeToApply);
	void ApplyCharacterTypeDeferred();
	// 실제 적용 시도: 성공하면 true, 실패하면 false
	bool TryApplyCharacterType(EBFCharacterType TypeToApply);
	
	// ----- Helpers -----
	USkeletalMeshComponent* ResolveMeshComponent() const;
	const TSoftObjectPtr<USkeletalMesh>* FindMeshSoftPtr(EBFCharacterType Type) const;
	
	static bool IsValidCharacterType(EBFCharacterType Type);
	
	// ----- Cached Owner -----
	UPROPERTY(Transient)
	TObjectPtr<ABFPawnBase> OwnerCharacter;
	
	// ----- Internal State -----
	bool bGiveUpDeferredApply = false;
	
	// 무한 재시도 방지용
	int32 DeferredApplyAttempts = 0;
	static constexpr int32 MaxDeferredApplyAttempts = 10;

	// ----- Timer -----
	FTimerHandle DeferredApplyHandle;
};
