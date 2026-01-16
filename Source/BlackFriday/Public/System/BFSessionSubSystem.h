// Fill out your copyright notice in the Description page of Project Settings.
// Source/BlackFriday/Public/Systems/BFSessionSubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "BFSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBFCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBFFindSessionsComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBFJoinSessionComplete, bool, bWasSuccessful);

UCLASS()
class BLACKFRIDAY_API UBFSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UBFSessionSubsystem();
    
    UFUNCTION(BlueprintCallable, Category = "BF|Session")
    void CreateSession(int32 InMaxPlayers, bool bInIsLAN);
    
    UFUNCTION(BlueprintCallable, Category = "BF|Session")
    void FindSessions(int32 InMaxResults, bool bInIsLAN);
    
    UFUNCTION(BlueprintCallable, Category = "BF|Session")
    void JoinSession(int32 InIndex);
    
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
};