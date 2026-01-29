// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "BFSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBFCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBFFindSessionsComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBFJoinSessionComplete, bool, bWasSuccessful);

// 세션 리스트 UI에서 사용할 정보 구조체
USTRUCT(BlueprintType)
struct FBFSessionInfo
{
    GENERATED_BODY()

    // 세션 이름 (호스트가 설정한 이름)
    UPROPERTY(BlueprintReadOnly, Category = "BF|Session")
    FString SessionName;

    // 현재 접속 인원
    UPROPERTY(BlueprintReadOnly, Category = "BF|Session")
    int32 CurrentPlayers;

    // 최대 인원
    UPROPERTY(BlueprintReadOnly, Category = "BF|Session")
    int32 MaxPlayers;

    // 핑 (밀리초)
    UPROPERTY(BlueprintReadOnly, Category = "BF|Session")
    int32 PingInMs;

    // 세션 인덱스 (JoinSession 호출 시 사용)
    UPROPERTY(BlueprintReadOnly, Category = "BF|Session")
    int32 SessionIndex;

    FBFSessionInfo()
        : SessionName(TEXT(""))
        , CurrentPlayers(0)
        , MaxPlayers(0)
        , PingInMs(0)
        , SessionIndex(-1)
    {}
};

UCLASS(BlueprintType)
class BLACKFRIDAY_API UBFSessionListItem : public UObject
{
    GENERATED_BODY()

public:
    // 세션 정보 데이터
    UPROPERTY(BlueprintReadWrite, Category = "BF|Session")
    FBFSessionInfo SessionInfo;

    // 인원수 포맷된 문자열 (예: "2/8")
    UFUNCTION(BlueprintPure, Category = "BF|Session")
    FString GetPlayerCountText() const
    {
        return FString::Printf(TEXT("%d/%d"), SessionInfo.CurrentPlayers, SessionInfo.MaxPlayers);
    }

    // 핑 포맷된 문자열 (예: "32ms")
    UFUNCTION(BlueprintPure, Category = "BF|Session")
    FString GetPingText() const
    {
        return FString::Printf(TEXT("%dms"), SessionInfo.PingInMs);
    }
};

UCLASS()
class BLACKFRIDAY_API UBFSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UBFSessionSubsystem();

    // ===== 플레이어 이름 관리 =====

    // 플레이어 이름 설정 (게임 시작 시 이름 입력 UI에서 호출)
    UFUNCTION(BlueprintCallable, Category = "BF|Session")
    void SetPlayerName(const FString& InPlayerName);

    // 플레이어 이름 가져오기
    UFUNCTION(BlueprintPure, Category = "BF|Session")
    FString GetPlayerName() const;

    // 플레이어 이름 저장 (INI 파일에 저장)
    UFUNCTION(BlueprintCallable, Category = "BF|Session")
    void SavePlayerName();

    // 플레이어 이름 불러오기 (INI 파일에서 불러오기)
    UFUNCTION(BlueprintCallable, Category = "BF|Session")
    void LoadPlayerName();

    // ===== 세션 관리 =====

    UFUNCTION(BlueprintCallable, Category = "BF|Session")
    void CreateSession(int32 InMaxPlayers, bool bInIsLAN);

    // 세션 이름과 함께 세션 생성
    UFUNCTION(BlueprintCallable, Category = "BF|Session")
    void CreateSessionWithName(const FString& InSessionName, int32 InMaxPlayers = 8, bool bInIsLAN = false);

    UFUNCTION(BlueprintCallable, Category = "BF|Session")
    void FindSessions(int32 InMaxResults, bool bInIsLAN);

    UFUNCTION(BlueprintCallable, Category = "BF|Session")
    void JoinSession(int32 InIndex);

    // ===== 세션 리스트 조회 =====

    // 검색된 세션 리스트 가져오기 (FindSessions 완료 후 호출)
    UFUNCTION(BlueprintCallable, Category = "BF|Session")
    TArray<FBFSessionInfo> GetSessionList() const;

    // ListView용 세션 리스트 가져오기 (UObject 배열 반환)
    UFUNCTION(BlueprintCallable, Category = "BF|Session")
    TArray<UBFSessionListItem*> GetSessionListItems();

    // 세션 리스트 개수 가져오기
    UFUNCTION(BlueprintPure, Category = "BF|Session")
    int32 GetSessionCount() const;

    // ===== 델리게이트 =====

    UPROPERTY(BlueprintAssignable, Category = "BF|Session")
    FOnBFCreateSessionComplete OnCreateSessionComplete;

    UPROPERTY(BlueprintAssignable, Category = "BF|Session")
    FOnBFFindSessionsComplete OnFindSessionsComplete;

    UPROPERTY(BlueprintAssignable, Category = "BF|Session")
    FOnBFJoinSessionComplete OnJoinSessionComplete;

protected:
    void OnCreateSessionCompleteInternal(FName SessionName, bool bWasSuccessful);
    void OnFindSessionsCompleteInternal(bool bWasSuccessful);
    void OnJoinSessionCompleteInternal(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

private:
    IOnlineSessionPtr SessionInterface;

    TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

    // 플레이어 이름
    FString PlayerName;

    // 세션 이름 (세션 생성 시 사용)
    FString CurrentSessionName;

    // 검색된 세션 리스트 캐시
    TArray<FBFSessionInfo> CachedSessionList;
};