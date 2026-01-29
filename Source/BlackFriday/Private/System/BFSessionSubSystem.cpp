// Fill out your copyright notice in the Description page of Project Settings.

#include "System/BFSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"

UBFSessionSubsystem::UBFSessionSubsystem()
{
}

// ===== 플레이어 이름 관리 =====

void UBFSessionSubsystem::SetPlayerName(const FString& InPlayerName)
{
    PlayerName = InPlayerName;
}

FString UBFSessionSubsystem::GetPlayerName() const
{
    return PlayerName;
}

void UBFSessionSubsystem::SavePlayerName()
{
    if (GConfig)
    {
        GConfig->SetString(TEXT("BlackFriday"), TEXT("PlayerName"), *PlayerName, GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
    }
}

void UBFSessionSubsystem::LoadPlayerName()
{
    if (GConfig)
    {
        GConfig->GetString(TEXT("BlackFriday"), TEXT("PlayerName"), PlayerName, GGameUserSettingsIni);
    }
}

// ===== 세션 리스트 조회 =====

TArray<FBFSessionInfo> UBFSessionSubsystem::GetSessionList() const
{
    return CachedSessionList;
}

TArray<UBFSessionListItem*> UBFSessionSubsystem::GetSessionListItems()
{
    TArray<UBFSessionListItem*> Items;

    for (const FBFSessionInfo& Info : CachedSessionList)
    {
        UBFSessionListItem* Item = NewObject<UBFSessionListItem>(this);
        Item->SessionInfo = Info;
        Items.Add(Item);
    }

    return Items;
}

int32 UBFSessionSubsystem::GetSessionCount() const
{
    return CachedSessionList.Num();
}

void UBFSessionSubsystem::CreateSession(int32 InMaxPlayers, bool bInIsLAN)
{
    // 기존 함수 유지 - 세션 이름 없이 생성
    CreateSessionWithName(TEXT(""), InMaxPlayers, bInIsLAN);
}

void UBFSessionSubsystem::CreateSessionWithName(const FString& InSessionName, int32 InMaxPlayers, bool bInIsLAN)
{
    CurrentSessionName = InSessionName;

    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
    if (Subsystem)
    {
        SessionInterface = Subsystem->GetSessionInterface();
    }

    if (!SessionInterface.IsValid())
    {
        OnCreateSessionComplete.Broadcast(false);
        return;
    }

    FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
    if (ExistingSession != nullptr)
    {
        SessionInterface->DestroySession(NAME_GameSession);
    }

    SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &UBFSessionSubsystem::OnCreateSessionCompleteInternal));

    TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());

    SessionSettings->bIsLANMatch = bInIsLAN;
    SessionSettings->NumPublicConnections = InMaxPlayers;
    SessionSettings->bAllowJoinInProgress = true;
    SessionSettings->bAllowInvites = true;
    SessionSettings->bShouldAdvertise = true;
    SessionSettings->bUsesPresence = true;

    // 기존 SEARCH_KEYWORDS 유지 (세션 필터링용)
    SessionSettings->Set(SEARCH_KEYWORDS, FString("BlackFriday"), EOnlineDataAdvertisementType::ViaOnlineService);

    // 세션 이름을 커스텀 키로 저장 (UI 표시용)
    if (!InSessionName.IsEmpty())
    {
        SessionSettings->Set(FName("SESSION_NAME"), InSessionName, EOnlineDataAdvertisementType::ViaOnlineService);
    }

    // 호스트 플레이어 이름도 저장
    if (!PlayerName.IsEmpty())
    {
        SessionSettings->Set(FName("HOST_NAME"), PlayerName, EOnlineDataAdvertisementType::ViaOnlineService);
    }

    const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
    if (LocalPlayer)
    {
        SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings);
    }
}

void UBFSessionSubsystem::OnCreateSessionCompleteInternal(FName SessionName, bool bWasSuccessful)
{
    if (bWasSuccessful)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            //맵 경로가 실제 파일 위치와 정확히
            World->ServerTravel(TEXT("/Game/Colab/JSW/Maps/Maps_Test1Arrival?listen"));
        }
    }

    OnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void UBFSessionSubsystem::FindSessions(int32 InMaxResults, bool bInIsLAN)
{
    IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
    if (Subsystem) SessionInterface = Subsystem->GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        OnFindSessionsComplete.Broadcast(false);
        return;
    }

    SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &UBFSessionSubsystem::OnFindSessionsCompleteInternal));

    LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
    LastSessionSearch->MaxSearchResults = InMaxResults;
    LastSessionSearch->bIsLanQuery = bInIsLAN;
    LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

    const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
    if (LocalPlayer)
    {
        SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef());
    }
}

void UBFSessionSubsystem::OnFindSessionsCompleteInternal(bool bWasSuccessful)
{
    // 세션 리스트 캐시 초기화
    CachedSessionList.Empty();

    if (bWasSuccessful && LastSessionSearch.IsValid())
    {
        for (int32 i = 0; i < LastSessionSearch->SearchResults.Num(); i++)
        {
            const FOnlineSessionSearchResult& SearchResult = LastSessionSearch->SearchResults[i];

            FBFSessionInfo SessionInfo;
            SessionInfo.SessionIndex = i;

            // 세션 이름 가져오기
            FString SessionName;
            if (SearchResult.Session.SessionSettings.Get(FName("SESSION_NAME"), SessionName))
            {
                SessionInfo.SessionName = SessionName;
            }
            else
            {
                // 세션 이름이 없으면 기본값
                SessionInfo.SessionName = FString::Printf(TEXT("Session %d"), i + 1);
            }

            // 인원 정보
            SessionInfo.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
            SessionInfo.CurrentPlayers = SessionInfo.MaxPlayers - SearchResult.Session.NumOpenPublicConnections;

            // 핑 정보
            SessionInfo.PingInMs = SearchResult.PingInMs;

            CachedSessionList.Add(SessionInfo);
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("검색 결과: %d개 찾음 (성공여부: %d)"), CachedSessionList.Num(), bWasSuccessful);
    OnFindSessionsComplete.Broadcast(bWasSuccessful);
}

void UBFSessionSubsystem::JoinSession(int32 InIndex)
{
    // [경보 1] 함수 진입 확인
    UE_LOG(LogTemp, Warning, TEXT("[JoinSession] 요청 들어옴! Index: %d"), InIndex);

    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[JoinSession] 실패: SessionInterface가 NULL입니다."));
        OnJoinSessionComplete.Broadcast(false);
        return;
    }

    if (!LastSessionSearch.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[JoinSession] 실패: 검색 결과(LastSessionSearch)가 만료되었습니다. 다시 검색하세요."));
        OnJoinSessionComplete.Broadcast(false);
        return;
    }

    // [경보 2] 인덱스 검사
    int32 SearchCount = LastSessionSearch->SearchResults.Num();
    UE_LOG(LogTemp, Warning, TEXT("[JoinSession] 현재 검색된 세션 수: %d, 요청한 인덱스: %d"), SearchCount, InIndex);

    if (SearchCount <= InIndex)
    {
        UE_LOG(LogTemp, Error, TEXT("[JoinSession] 실패: 인덱스 초과! (방은 %d개인데 %d번을 요청함)"), SearchCount, InIndex);
        OnJoinSessionComplete.Broadcast(false);
        return;
    }

    // 통과! 접속 시도
    UE_LOG(LogTemp, Warning, TEXT("[JoinSession] 검증 통과! 접속 시도 중..."));
    
    SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
        FOnJoinSessionCompleteDelegate::CreateUObject(this, &UBFSessionSubsystem::OnJoinSessionCompleteInternal));

    const FOnlineSessionSearchResult& Result = LastSessionSearch->SearchResults[InIndex];
    const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
    
    if (LocalPlayer)
    {
        SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, Result);
    }
}

void UBFSessionSubsystem::OnJoinSessionCompleteInternal(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    UE_LOG(LogTemp, Warning, TEXT("접속 시도 결과: %s"), 
        Result == EOnJoinSessionCompleteResult::Success ? TEXT("성공") : TEXT("실패"));
    
    bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);

    if (bSuccess)
    {
        FString ConnectInfo;
        if (SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectInfo))
        {
            if (ConnectInfo.EndsWith(":0"))
            {
                UE_LOG(LogTemp, Warning, TEXT("[BFSession] Port was 0. Fixing to 7777. Original: %s"), *ConnectInfo);
                ConnectInfo = ConnectInfo.LeftChop(2); // 끝에 :0 자르기
                ConnectInfo.Append(":7777");           // :7777 붙이기
            }

            APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
            if (PlayerController)
            {
                // 수정된 주소로 접속 시도
                PlayerController->ClientTravel(ConnectInfo, ETravelType::TRAVEL_Absolute);
            }
        }
    }

    OnJoinSessionComplete.Broadcast(bSuccess);
}