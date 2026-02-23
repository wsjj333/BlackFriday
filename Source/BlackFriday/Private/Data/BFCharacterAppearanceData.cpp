#include "Data/BFCharacterAppearanceData.h"
#include "Data/Enums/BFCharacterType.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SoftObjectPath.h"
#include "Editor.h"
#endif

static FString ConvertEnumToUnderscoreName(const FString& In)
{
	FString Out;
	Out.Reserve(In.Len() * 2);

	auto IsUpper = [](TCHAR C) { return FChar::IsUpper(C); };
	auto IsLower = [](TCHAR C) { return FChar::IsLower(C); };
	auto IsDigit = [](TCHAR C) { return FChar::IsDigit(C); };
	auto IsAlpha = [](TCHAR C) { return FChar::IsAlpha(C); };

	for (int32 i = 0; i < In.Len(); ++i)
	{
		const TCHAR C = In[i];
		const TCHAR Prev = (i > 0) ? In[i - 1] : 0;
		const TCHAR Next = (i + 1 < In.Len()) ? In[i + 1] : 0;

		// 이미 '_'가 있으면 그대로
		if (C == '_')
		{
			Out.AppendChar(C);
			continue;
		}

		const bool bIsUpper = IsUpper(C);
		const bool bIsDigit = IsDigit(C);

		const bool bPrevIsLower = (i > 0 && IsLower(Prev));
		const bool bPrevIsUpper = (i > 0 && IsUpper(Prev));
		const bool bPrevIsDigit = (i > 0 && IsDigit(Prev));
		const bool bPrevIsAlpha = (i > 0 && IsAlpha(Prev));

		const bool bNextIsLower = (i + 1 < In.Len() && IsLower(Next));

		// 언더스코어 삽입 규칙
		const bool bNeedUnderscore =
			// 소문자 다음 대문자: AfroHair -> Afro_Hair
			(bIsUpper && bPrevIsLower) ||
			// 대문자 연속에서 끊김: XMLParser -> XML_Parser (P 앞)
			(bIsUpper && bPrevIsUpper && bNextIsLower) ||
			// 문자 다음 숫자: Woman1 -> Woman_1
			(bIsDigit && bPrevIsAlpha) ||
			// 숫자 다음 문자: 1Woman -> 1_Woman (원하면 유지)
			(!bIsDigit && bPrevIsDigit);

		if (i > 0 && bNeedUnderscore)
		{
			// 중복 '_' 방지
			if (Out.Len() > 0 && Out[Out.Len() - 1] != '_')
			{
				Out.AppendChar('_');
			}
		}

		Out.AppendChar(C);
	}

	return Out;
}


#if WITH_EDITOR
void UBFCharacterAppearanceData::RebuildFromEnum()
{
	MeshMap.Reset();

	const UEnum* Enum = StaticEnum<EBFCharacterType>();
	if (!Enum)
	{
		return;
	}

	const FString Root =
		TEXT("/Game/PublicAsset/Hyper-Casual_Character_Pack/Meshes/");
	const FString Prefix = TEXT("Mesh_");

	for (int32 i = 0; i < Enum->NumEnums(); ++i)
	{
		const int64 Value = Enum->GetValueByIndex(i);
		const EBFCharacterType Type =
			static_cast<EBFCharacterType>(Value);

		if (Type == EBFCharacterType::None)
		{
			continue;
		}

		FString EnumName =
			Enum->GetNameStringByValue(Value);

		// 🔥 핵심 부분
		FString Converted =
			ConvertEnumToUnderscoreName(EnumName);

		FString AssetName =
			Prefix + Converted;

		FString ObjectPath =
			Root + AssetName + TEXT(".") + AssetName;

		MeshMap.Add(
			Type,
			TSoftObjectPtr<USkeletalMesh>(
				FSoftObjectPath(ObjectPath)
			)
		);

		UE_LOG(LogTemp, Log,
			TEXT("[Appearance] AutoMapped %s -> %s"),
			*EnumName,
			*ObjectPath);
	}

#if WITH_EDITOR
	Modify();
	(void)MarkPackageDirty();
#endif
}
// void UBFCharacterAppearanceData::RebuildFromFolder()
// {
// 	MeshMap.Reset();
//
// 	FAssetRegistryModule& AssetRegistryModule =
// 		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
//
// 	FARFilter Filter;
// 	Filter.bRecursivePaths = true;
// 	Filter.PackagePaths.Add(ScanRootPath);
//
// 	// UE5에서는 ClassPaths 권장
// 	Filter.ClassPaths.Add(USkeletalMesh::StaticClass()->GetClassPathName());
//
// 	TArray<FAssetData> Assets;
// 	AssetRegistryModule.Get().GetAssets(Filter, Assets);
//
// 	const UEnum* Enum = StaticEnum<EBFCharacterType>();
// 	if (!Enum)
// 	{
// 		return;
// 	}
//
// 	for (const FAssetData& Asset : Assets)
// 	{
// 		// 에셋 이름 (예: Mesh_Alien)
// 		FString AssetName = Asset.AssetName.ToString();
//
// 		// Prefix 제거 (Mesh_ -> Alien)
// 		FString Token = AssetName;
// 		if (!NamePrefixToStrip.IsEmpty() && Token.StartsWith(NamePrefixToStrip))
// 		{
// 			Token.RightChopInline(NamePrefixToStrip.Len());
// 		}
//
// 		// Token을 Enum 이름으로 매칭 (Enum 항목이 Alien 이거나 AfroHairMan 등)
// 		const int64 EnumValue = Enum->GetValueByNameString(Token);
// 		if (EnumValue == INDEX_NONE)
// 		{
// 			// 매칭 실패한 건 로그로 남겨두는 게 좋음
// 			UE_LOG(LogTemp, Warning, TEXT("[AppearanceScan] Enum match failed: %s (asset: %s)"),
// 				*Token, *Asset.GetObjectPathString());
// 			continue;
// 		}
//
// 		const EBFCharacterType Type = static_cast<EBFCharacterType>(EnumValue);
//
// 		// Soft 참조로 저장 (패키징/리다이렉트에 안전)
// 		const FSoftObjectPath SoftPath(Asset.GetObjectPathString());
// 		MeshMap.Add(Type, TSoftObjectPtr<USkeletalMesh>(SoftPath));
// 	}
//
// 	// 변경사항 저장 가능 상태로 표시
// 	if (!MarkPackageDirty())
// 	{
// 		UE_LOG(LogTemp, Warning, TEXT("Failed to mark package dirty"));
// 	}
//
// 	UE_LOG(LogTemp, Log, TEXT("[AppearanceScan] Rebuilt %d meshes from %s"),
// 		MeshMap.Num(), *ScanRootPath.ToString());
// }
#endif