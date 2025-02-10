// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "SteamGameInstance.h"
#include "MainMenu.h"
#include "PauseMenu.h"
#include "ServerListEntry.h"

GMCSTEAM_API DEFINE_LOG_CATEGORY(LogGMCOnlineSubsystem);

static const FName UserServerNameKey = TEXT("ServerName");

void USteamGameInstance::Init()
{
  Super::Init();

  if (!MainMenu) UE_LOG(LogGMCOnlineSubsystem, Warning, TEXT("No main menu WBP set."))
  if (!PauseMenu) UE_LOG(LogGMCOnlineSubsystem, Warning, TEXT("No pause menu WBP set."))
  if (!ServerListEntry) UE_LOG(LogGMCOnlineSubsystem, Warning, TEXT("No server list entry WBP set."))

  OnlineSubsystem = IOnlineSubsystem::Get();
  if (OnlineSubsystem)
  {
    UE_LOG(LogGMCOnlineSubsystem, Log, TEXT("Online subsystem \"%s\" found."), *OnlineSubsystem->GetSubsystemName().ToString());
    SessionInterface = OnlineSubsystem->GetSessionInterface();
    if (SessionInterface.IsValid())
    {
      // Bind functions for session creation and destruction.
      SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &USteamGameInstance::OnCreateSessionComplete);
      SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &USteamGameInstance::OnDestroySessionComplete);
      SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &USteamGameInstance::OnFindSessionsComplete);
      SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &USteamGameInstance::OnJoinSessionComplete);

      // Session settings.
      SessionSettings.NumPublicConnections = MaxPublicConnections;
      if (OnlineSubsystem->GetSubsystemName() == "NULL")
      {
        SessionSettings.bIsLANMatch = true;
        UE_LOG(LogGMCOnlineSubsystem, Log, TEXT("Using online subsystem NULL, only LAN sessions will be available."))
      }
      else
      {
        SessionSettings.bIsLANMatch = false;
      }
      SessionSettings.bShouldAdvertise = true;
      SessionSettings.bAllowInvites = false;
      SessionSettings.bAllowJoinInProgress = true;
      SessionSettings.bUsesPresence = true; // Enables lobby sessions.

      // Workaround for a problem with Steam in 4.27 and higher. This must be true in order to be able to create game sessions.
      SessionSettings.bUseLobbiesIfAvailable = true;

      // Search settings.
      SessionSearch = MakeShareable(new FOnlineSessionSearch);
      SessionSearch->MaxSearchResults = 1000; // Must be a high value when using Steam dev app id 480 to filter the results for this game.
      SessionSearch->QuerySettings.Set(TEXT("PRESENCESEARCH"), true, EOnlineComparisonOp::Equals);
    }
    else
    {
      UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("No session interface found."));
    }
  }
  else
  {
    UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("No online subsystem found."));
  }

  // Bind the functions to catch network errors.
  if (GEngine)
  {
    GEngine->OnNetworkFailure().AddUObject(this, &USteamGameInstance::OnNetworkFailure);
    GEngine->OnTravelFailure().AddUObject(this, &USteamGameInstance::OnTravelFailure);
  }
}

bool USteamGameInstance::RefreshServerList()
{
  if (SessionSearch.IsValid())
  {
    if (SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
    {
      UE_LOG(LogGMCOnlineSubsystem, Log, TEXT("Searching for sessions..."));
      return true;
    }
  }
  UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("Search for sessions failed."))
  return false;
}

bool USteamGameInstance::HostGameSession(const FString& SessionName)
{
  UserDesiredSessionName = SessionName;
  const auto& LocalPlayerController = GetFirstLocalPlayerController();
  if (LocalPlayerController->HasAuthority())
  {
    if (SessionInterface.IsValid())
    {
      FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SessionLabel);
      if (ExistingSession)
      {
        // The session already exists, destroy it now.
        UE_LOG(LogGMCOnlineSubsystem, Warning,
          TEXT("Cannot create session as it already exists. Try again when the session was destroyed."))
        SessionInterface->DestroySession(ExistingSession->SessionName);
        return false;
      }
      SessionSettings.Set(UserServerNameKey, UserDesiredSessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
      // ServerTravel is called once the session was successfully created.
      SessionInterface->CreateSession(0, SessionLabel, SessionSettings);
      return true;
    }
    UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("Host failure: session interface not valid."))
    return false;
  }
  UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("Host failure: not a server."))
  return false;
}

void USteamGameInstance::OnTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString) const
{
  UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("A travel error occured (failure type %d: %s)."), FailureType, *ErrorString)
}

void USteamGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
  if (FailureType != ENetworkFailure::ConnectionTimeout)
  {
    UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("A network error occured (failure type %d: %s)."), FailureType, *ErrorString)
  }
  else
  {
    FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(SessionLabel);
    if (ExistingSession)
    {
      SessionInterface->DestroySession(ExistingSession->SessionName);
    }
    ReturnToMainMenu(); // Disconnect if we have a connection timeout (default after 60 seconds, specified in the config file).
    UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("Connection has timed out."))
  }
}

FString USteamGameInstance::GetCurrentMapName() const
{
  if (const auto& World = GetWorld())
  {
    FString CurrentMapName = World->GetMapName().Mid(World->StreamingLevelsPrefix.Len());
    return CurrentMapName;
  }
  return FString();
}

void USteamGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful) const
{
  if (bWasSuccessful)
  {
    UE_LOG(LogGMCOnlineSubsystem, Log, TEXT("Session \"%s\" was created successfully."), *SessionName.ToString())
    if (const auto& World = GetWorld())
    {
      // Start a listen server.
      World->ServerTravel(FString::Printf(TEXT("/Game/%s?listen"), *HostURL));
      return;
    }
    checkNoEntry()
  }
  else
  {
    UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("Session \"%s\" could not be created."), *SessionName.ToString())
  }
  OnCreateSessionCompleted.ExecuteIfBound();
}

void USteamGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful) const
{
  if (bWasSuccessful && SessionInterface.IsValid())
  {
    UE_LOG(LogGMCOnlineSubsystem, Log, TEXT("Session \"%s\" was destroyed."), *SessionName.ToString())
  }
  else
  {
    UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("Session \"%s\" could not be destroyed."), *SessionName.ToString())
  }
}

void USteamGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
  if (bWasSuccessful && SessionSearch)
  {
    UE_LOG(LogGMCOnlineSubsystem, Log, TEXT("Search for sessions completed."));
    ServerList.Empty(); // Clear server list before the refresh.
    for (const auto& SearchResult : SessionSearch->SearchResults)
    {
      UE_LOG(LogGMCOnlineSubsystem, VeryVerbose, TEXT("Found session \"%s\"."), *SearchResult.GetSessionIdStr());

      // Save the data of this search result that we want to display in the server list.
      FServerInfo ServerData{};
      ServerData.SessionSearchResult = SearchResult;
      ServerData.ServerName = UserDesiredSessionName;
      ServerData.HostName = SearchResult.Session.OwningUserName;
      ServerData.NumMaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
      ServerData.NumCurrentPlayers = ServerData.NumMaxPlayers - SearchResult.Session.NumOpenPublicConnections;
      ServerData.Ping = SearchResult.PingInMs;

      // Get custom session settings for server name.
      FString ServerName;
      if (SearchResult.Session.SessionSettings.Get(UserServerNameKey, ServerName))
      {
        UE_LOG(LogGMCOnlineSubsystem, VeryVerbose, TEXT("Found custom session settings for \"%s\""), *ServerName);
        ServerData.ServerName = ServerName;
      }
      else
      {
        UE_LOG(LogGMCOnlineSubsystem, VeryVerbose, TEXT("No custom session settings found for \"%s\"."), *ServerName);
        ServerData.ServerName = SearchResult.GetSessionIdStr();
      }
      ServerList.Add(ServerData);
    }
  }
  else
  {
    UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("Search for sessions was not successful."))
  }
  OnServerListUpdated.ExecuteIfBound();
}

void USteamGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type ResultType) const
{
  UE_LOG(LogGMCOnlineSubsystem, VeryVerbose, TEXT("Join session returned %d."), ResultType)

  if (ResultType == EOnJoinSessionCompleteResult::Success)
  {
    const auto& LocalPlayerController = GetFirstLocalPlayerController();
    if (LocalPlayerController && SessionInterface)
    {
      FString Address;
      if (SessionInterface->GetResolvedConnectString(SessionName, Address))
      {
        LocalPlayerController->ClientTravel(Address, TRAVEL_Absolute);
        UE_LOG(LogGMCOnlineSubsystem, Log, TEXT("Joined session \"%s\" hosted by \"%s\"."), *SessionName.ToString(), *Address)
      }
      else
      {
        UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("Connection information could not be retrieved."));
      }
    }
  }
  else
  {
    const auto Reason = [ResultType]() -> FString
    {
      switch (ResultType)
      {
        case EOnJoinSessionCompleteResult::Success: checkNoEntry() return "";
        case EOnJoinSessionCompleteResult::SessionIsFull: return "session is full.";
        case EOnJoinSessionCompleteResult::SessionDoesNotExist: return "session does not exist.";
        case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress: return "could not retrieve address.";
        case EOnJoinSessionCompleteResult::AlreadyInSession: return "already in session.";
        case EOnJoinSessionCompleteResult::UnknownError: return "an unknown error occurred.";
        default: checkNoEntry();
      }
      checkNoEntry()
      return "";
    };
    UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("Could not join session: %s"), *Reason());
  }
  OnJoinSessionCompleted.ExecuteIfBound();
}

bool USteamGameInstance::JoinGameSession(int32 ServerEntryIndex)
{
  if (ServerEntryIndex < 0 || ServerEntryIndex >= ServerList.Num())
  {
    UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("Server list index out of bounds."));
    return false;
  }
  if (SessionInterface && ServerList[ServerEntryIndex].SessionSearchResult.IsValid())
  {
    SessionInterface->JoinSession(0, SessionLabel, ServerList[ServerEntryIndex].SessionSearchResult);
    return true;
  }
  UE_LOG(LogGMCOnlineSubsystem, Error, TEXT("Attempt to join session failed."));
  return false;
}
