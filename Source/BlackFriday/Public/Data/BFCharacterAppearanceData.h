// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BFCharacterAppearanceData.generated.h"

enum class EBFCharacterType : uint8;
/**
 * 
 */
UCLASS()
class BLACKFRIDAY_API UBFCharacterAppearanceData : public UPrimaryDataAsset
{
	GENERATED_BODY()
																													
public:
	UPROPERTY(EditAnywhere)
	TMap<EBFCharacterType, TSoftObjectPtr<USkeletalMesh>> MeshMap;
	
	// 스캔할 폴더 (/Game 로 시작)
	UPROPERTY(EditAnywhere, Category="Appearance|Scan")
	FName ScanRootPath = TEXT("/Game/PublicAsset/Hyper-Casual_Character_Pack/Meshes");

	// 이름 규칙 예: Mesh_Alien -> Enum Alien, 혹은 AfroHairMan 등
	UPROPERTY(EditAnywhere, Category="Appearance|Scan")
	FString NamePrefixToStrip = TEXT("Mesh_");

#if WITH_EDITOR
	// 디테일 패널에서 버튼으로 실행
	UFUNCTION(CallInEditor, Category="Appearance|Scan")
	void RebuildFromEnum();
#endif
};
