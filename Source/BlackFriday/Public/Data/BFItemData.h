// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BFItemData.generated.h"

USTRUCT(BlueprintType)
struct BLACKFRIDAY_API FBFItemData : public FTableRowBase
{
	GENERATED_BODY()

	// 아이템 이름 (한글 지원)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BF|Item")
	FText ItemName;

	// 아이템 가격
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BF|Item")
	float ItemPrice = 0.0f;

	// TODO: 아이콘, 카테고리 등 추가 예정
};
