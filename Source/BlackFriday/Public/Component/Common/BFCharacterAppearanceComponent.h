#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Enums/BFCharacterType.h"
#include "BFCharacterAppearanceComponent.generated.h"

class ABFPawnBase;
class USkeletalMesh;
class USkeletalMeshComponent;
class UBFCharacterAppearanceData;

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

	// 매핑 데이터는 DataAsset에서만 관리
	UPROPERTY(EditDefaultsOnly, Category="BF|Visual")
	TObjectPtr<UBFCharacterAppearanceData> AppearanceData;

	UPROPERTY(BlueprintAssignable, Category="BF|Visual")
	FOnAppearanceApplied OnAppearanceApplied;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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
	// 기존 void ApplyCharacterType는 내부에서 TryApply를 호출하는 wrapper로 유지
	void ApplyCharacterType(EBFCharacterType TypeToApply);

	// ✅ 실제 적용 시도: 성공하면 true, 실패하면 false
	bool TryApplyCharacterType(EBFCharacterType TypeToApply);

	// ✅ 실패 시 다음 틱 재시도 (JIP/초기화 레이스 대응)
	void ApplyCharacterTypeDeferred();

	bool IsValidCharacterType(EBFCharacterType Type) const;

	USkeletalMeshComponent* ResolveMeshComponent() const;
	const TSoftObjectPtr<USkeletalMesh>* FindMeshSoftPtr(EBFCharacterType Type) const;

	// ✅ deferred 재시도 관리
	FTimerHandle DeferredApplyHandle;

	// ✅ 무한 재시도 방지용 (원하면 값 조절)
	int32 DeferredApplyAttempts = 0;
	static constexpr int32 MaxDeferredApplyAttempts = 10;
};