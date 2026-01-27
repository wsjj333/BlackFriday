// Fill out your copyright notice in the Description page of Project Settings.

#include "System/BFSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h" 

UBFSessionSubsystem::UBFSessionSubsystem()
{
}

void UBFSessionSubsystem::CreateSession(int32 InMaxPlayers, bool bInIsLAN)
{
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
    
    SessionSettings->Set(SEARCH_KEYWORDS, FString("BlackFriday"), EOnlineDataAdvertisementType::ViaOnlineService);

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
    OnFindSessionsComplete.Broadcast(bWasSuccessful);
}

void UBFSessionSubsystem::JoinSession(int32 InIndex)
{
    if (!SessionInterface.IsValid() || !LastSessionSearch.IsValid())
    {
        OnJoinSessionComplete.Broadcast(false);
        return;
    }

    if (LastSessionSearch->SearchResults.Num() <= InIndex)
    {
        OnJoinSessionComplete.Broadcast(false);
        return;
    }

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