// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Smoothing.h"
#include "SyncSettings.generated.h"

namespace GMCReplication
{

struct FServerFlags
{
  uint8 bReplicateForSimulation:1;
  uint8 bReplicateForPrediction:1;
  uint8 bReplicateForCorrection:1;
  uint8 bValidate:1;
  uint8 bUseClientValue:1;
  uint8 bForceUpdateOnChange:1;

  FServerFlags(
    bool bReplicateForSimulation,
    bool bReplicateForPrediction,
    bool bReplicateForCorrection,
    bool bValidate,
    bool bUseClientValue,
    bool bForceUpdateOnChange
  ) :
    bReplicateForSimulation(bReplicateForSimulation),
    bReplicateForPrediction(bReplicateForPrediction),
    bReplicateForCorrection(bReplicateForCorrection),
    bValidate(bValidate),
    bUseClientValue(bUseClientValue),
    bForceUpdateOnChange(bForceUpdateOnChange)
  {}

  FServerFlags() :
    bReplicateForSimulation(false),
    bReplicateForPrediction(false),
    bReplicateForCorrection(false),
    bValidate(false),
    bUseClientValue(false),
    bForceUpdateOnChange(false)
  {}

  bool Compare(const FServerFlags& Rhs) const
  {
    return
      bReplicateForSimulation == Rhs.bReplicateForSimulation &&
      bReplicateForPrediction == Rhs.bReplicateForPrediction &&
      bReplicateForCorrection == Rhs.bReplicateForCorrection &&
      bValidate == Rhs.bValidate &&
      bUseClientValue == Rhs.bUseClientValue &&
      bForceUpdateOnChange == Rhs.bForceUpdateOnChange;
  }
};

struct FClientFlags
{
  uint8 CombineMode;
  uint8 bSendToServer:1;
  uint8 bValidate:1;

  FClientFlags(
    uint8 CombineMode,
    bool bSendToServer,
    bool bValidate
  ) :
    CombineMode(CombineMode),
    bSendToServer(bSendToServer),
    bValidate(bValidate)
  {}

  FClientFlags() :
    CombineMode(0),
    bSendToServer(false),
    bValidate(false)
  {}

  bool Compare(const FClientFlags& Rhs) const
  {
    return
      bSendToServer == Rhs.bSendToServer &&
      bValidate == Rhs.bValidate;
  }
};

struct FServerSettings
{
  FServerFlags InputState{};
  FServerFlags OutputState{};

  bool Compare(const FServerSettings& Rhs) const
  {
    return
      InputState.Compare(Rhs.InputState) &&
      OutputState.Compare(Rhs.OutputState);
  }
};

struct FClientSettings
{
  FClientFlags InputState{};
  FClientFlags OutputState{};

  bool Compare(const FClientSettings& Rhs) const
  {
    return
      InputState.Compare(Rhs.InputState) &&
      OutputState.Compare(Rhs.OutputState);
  }
};

struct FLocalSettings
{
  uint8 InterpFunc;
  uint8 bWildcard1:1;
  uint8 bWildcard2:1;
  uint8 bWildcard3:1;
  uint8 bWildcard4:1;

  FLocalSettings(
    uint8 InterpFunc,
    bool bWildcard1,
    bool bWildcard2,
    bool bWildcard3,
    bool bWildcard4
  ) :
    InterpFunc(InterpFunc),
    bWildcard1(bWildcard1),
    bWildcard2(bWildcard2),
    bWildcard3(bWildcard3),
    bWildcard4(bWildcard4)
  {}

  FLocalSettings() :
    InterpFunc(0),
    bWildcard1(false),
    bWildcard2(false),
    bWildcard3(false),
    bWildcard4(false)
  {}

  bool Compare(const FLocalSettings& Rhs) const
  {
    return
      InterpFunc == Rhs.InterpFunc &&
      bWildcard1 == Rhs.bWildcard1 &&
      bWildcard2 == Rhs.bWildcard2 &&
      bWildcard3 == Rhs.bWildcard3 &&
      bWildcard4 == Rhs.bWildcard4;
  }
};

struct FSyncSettings
{
  FServerSettings ServerSettings{};
  FClientSettings ClientSettings{};
  FLocalSettings LocalSettings{};

  bool Compare(const FSyncSettings& Rhs) const
  {
    return
      ServerSettings.Compare(Rhs.ServerSettings) &&
      ClientSettings.Compare(Rhs.ClientSettings) &&
      LocalSettings.Compare(Rhs.LocalSettings);
  }
};

}

UENUM()
enum class EGMC_PredictionMode : uint8
{
  ServerAuth_Output_ClientValidated UMETA(DisplayName = "ServerAuth Output ClientValidated", ToolTip = "Server authoritative output value validated client-side. Puts more pressure on the client's downstream."),
  ServerAuth_Input_ClientValidated UMETA(DisplayName = "ServerAuth Input ClientValidated", ToolTip = "Server authoritative input value validated client-side. Puts more pressure on the client's downstream."),
  ServerAuth_InputOutput_ClientValidated UMETA(DisplayName = "ServerAuth InputOutput ClientValidated", ToolTip = "Server authoritative input and output values validated client-side. Puts more pressure on the client's downstream."),
  ServerAuth_Output_ServerValidated UMETA(DisplayName = "ServerAuth Output ServerValidated", ToolTip = "Server authoritative output value validated server-side. Puts more pressure on the client's upstream."),
  ServerAuth_Input_ServerValidated UMETA(DisplayName = "ServerAuth Input ServerValidated", ToolTip = "Server authoritative input value validated server-side. Puts more pressure on the client's upstream."),
  ServerAuth_InputOutput_ServerValidated UMETA(DisplayName = "ServerAuth InputOutput ServerValidated", ToolTip = "Server authoritative input and output values validated server-side. Puts more pressure on the client's upstream."),
  ClientAuth_Output UMETA(DisplayName = "ClientAuth Output", ToolTip = "Client authoritative output value, the server will accept the client value without question."),
  ClientAuth_Input UMETA(DisplayName = "ClientAuth Input", ToolTip = "Client authoritative input value, the server will accept the client value without question."),
  ClientAuth_InputOutput UMETA(DisplayName = "ClientAuth InputOutput", ToolTip = "Client authoritative input and output values, the server will accept the client values without question."),
  Local UMETA(DisplayName = "Local", ToolTip = "No network synchronisation."),
};

UENUM()
enum class EGMC_CombineMode : uint8
{
  AlwaysCombine UMETA(DisplayName = "AlwaysCombine", ToolTip = "This value can always be combined."),
  CombineIfUnchanged UMETA(DisplayName = "CombineIfUnchanged", ToolTip = "This value can only be combined if it has not changed from the previous one."),
  Default UMETA(DisplayName = "Default", ToolTip = "Use the integrated C++ sync interface function to determine whether this value can be combined."),
  NeverCombine UMETA(DisplayName = "NeverCombine", ToolTip = "This value can never be combined.", Hidden), // This is here for completion's sake but has no real practical use.
};

UENUM()
enum class EGMC_SimulationMode : uint8
{
  Periodic_Output UMETA(DisplayName = "Periodic Output", ToolTip = "The output value is replicated periodically, clients may not receive every intermediate value."),
  Periodic_Input UMETA(DisplayName = "Periodic Input", ToolTip = "The input value is replicated periodically, clients may not receive every intermediate value."),
  Periodic_InputOutput UMETA(DisplayName = "Periodic InputOutput", ToolTip = "The input and output values are replicated periodically, clients may not receive every intermediate value."),
  PeriodicAndOnChange_Output UMETA(DisplayName = "PeriodicAndOnChange Output", ToolTip = "The output value is replicated periodically and on every change, use only for values that change infrequently."),
  PeriodicAndOnChange_Input UMETA(DisplayName = "PeriodicAndOnChange Input", ToolTip = "The input value is replicated periodically and on every change, use only for values that change infrequently."),
  PeriodicAndOnChange_InputOutput UMETA(DisplayName = "PeriodicAndOnChange InputOutput", ToolTip = "The input and output values are replicated periodically and on every change, use only for values that change infrequently."),
  None UMETA(DisplayName = "None", ToolTip = "Not replicated for simulation."),
};

enum class EGMC_SyncTag : uint8
{
  None,
};

USTRUCT(BlueprintType)
struct FGMC_DefaultPredictionSettings
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_PredictionMode ActorBase{EGMC_PredictionMode::ServerAuth_Output_ServerValidated};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_PredictionMode LinearVelocity{EGMC_PredictionMode::ServerAuth_Output_ClientValidated};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_PredictionMode AngularVelocity{EGMC_PredictionMode::ServerAuth_Output_ClientValidated};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_PredictionMode ActorLocation{EGMC_PredictionMode::ServerAuth_Output_ServerValidated};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_PredictionMode ActorRotation{EGMC_PredictionMode::ServerAuth_Output_ServerValidated};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_PredictionMode ActorScale{EGMC_PredictionMode::ServerAuth_Output_ServerValidated};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_PredictionMode ControlRotation{EGMC_PredictionMode::ClientAuth_Input};
};

USTRUCT(BlueprintType)
struct FGMC_DefaultCombineSettings
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_CombineMode ActorBase{EGMC_CombineMode::AlwaysCombine};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_CombineMode LinearVelocity{EGMC_CombineMode::AlwaysCombine};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_CombineMode AngularVelocity{EGMC_CombineMode::AlwaysCombine};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_CombineMode ActorLocation{EGMC_CombineMode::AlwaysCombine};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_CombineMode ActorRotation{EGMC_CombineMode::AlwaysCombine};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_CombineMode ActorScale{EGMC_CombineMode::AlwaysCombine};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_CombineMode ControlRotation{EGMC_CombineMode::AlwaysCombine};
};

USTRUCT(BlueprintType)
struct FGMC_DefaultSimulationSettings
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_SimulationMode ActorBase{EGMC_SimulationMode::Periodic_Output};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_SimulationMode LinearVelocity{EGMC_SimulationMode::Periodic_Output};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_SimulationMode AngularVelocity{EGMC_SimulationMode::Periodic_Output};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_SimulationMode ActorLocation{EGMC_SimulationMode::Periodic_Output};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_SimulationMode ActorRotation{EGMC_SimulationMode::Periodic_Output};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_SimulationMode ActorScale{EGMC_SimulationMode::Periodic_Output};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_SimulationMode ControlRotation{EGMC_SimulationMode::Periodic_Output};
};

USTRUCT(BlueprintType)
struct FGMC_DefaultInterpolationFunctions
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_InterpolationFunction ActorBase{EGMC_InterpolationFunction::NearestNeighbour};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_InterpolationFunction LinearVelocity{EGMC_InterpolationFunction::Linear};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_InterpolationFunction AngularVelocity{EGMC_InterpolationFunction::Linear};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_InterpolationFunction ActorLocation{EGMC_InterpolationFunction::Linear};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_InterpolationFunction ActorRotation{EGMC_InterpolationFunction::Linear};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_InterpolationFunction ActorScale{EGMC_InterpolationFunction::Linear};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_InterpolationFunction ControlRotation{EGMC_InterpolationFunction::Linear};
};

USTRUCT(BlueprintType)
struct FGMC_DefaultCompressionSettings
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_FloatPrecisionBlueprint LinearVelocity{EGMC_FloatPrecisionBlueprint::TwoDecimals};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_FloatPrecisionBlueprint AngularVelocity{EGMC_FloatPrecisionBlueprint::TwoDecimals};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_FloatPrecisionBlueprint ActorLocation{EGMC_FloatPrecisionBlueprint::TwoDecimals};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_FloatPrecisionBlueprint ActorRotation{EGMC_FloatPrecisionBlueprint::TwoDecimals};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_FloatPrecisionBlueprint ActorScale{EGMC_FloatPrecisionBlueprint::TwoDecimals};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  EGMC_FloatPrecisionBlueprint ControlRotation{EGMC_FloatPrecisionBlueprint::TwoDecimals};
};

USTRUCT(BlueprintType)
struct FGMC_DefaultErrorTolerances
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings", meta = (ClampMin = "0.000001", UIMin = "0.01", UIMax = "100"))
  float LinearVelocity{10.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings", meta = (ClampMin = "0.000001", UIMin = "0.01", UIMax = "100"))
  float AngularVelocity{10.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings", meta = (ClampMin = "0.000001", UIMin = "0.01", UIMax = "100"))
  float ActorLocation{2.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings", meta = (ClampMin = "0.000001", UIMin = "0.01", UIMax = "100"))
  float ActorRotation{2.f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings", meta = (ClampMin = "0.000001", UIMin = "0.01", UIMax = "100"))
  float ActorScale{0.01f};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings", meta = (ClampMin = "0.000001", UIMin = "0.01", UIMax = "100"))
  float ControlRotation{2.f};
};

struct FGMC_SyncTags
{
  EGMC_SyncTag ActorBase{EGMC_SyncTag::None};

  EGMC_SyncTag LinearVelocity{EGMC_SyncTag::None};

  EGMC_SyncTag AngularVelocity{EGMC_SyncTag::None};

  EGMC_SyncTag ActorLocation{EGMC_SyncTag::None};

  EGMC_SyncTag ActorRotation{EGMC_SyncTag::None};

  EGMC_SyncTag ActorScale{EGMC_SyncTag::None};

  EGMC_SyncTag ControlRotation{EGMC_SyncTag::None};
};

USTRUCT(BlueprintType)
struct FGMC_DefaultReplicationSettings
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  FGMC_DefaultPredictionSettings DefaultPredictionSettings{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  FGMC_DefaultCombineSettings DefaultCombineSettings{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  FGMC_DefaultSimulationSettings DefaultSimulationSettings{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  FGMC_DefaultInterpolationFunctions DefaultInterpolationFunctions{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  FGMC_DefaultCompressionSettings DefaultCompressionSettings{};

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Replication Settings")
  FGMC_DefaultErrorTolerances DefaultErrorTolerances{};

  FGMC_SyncTags SyncTags{};
};

inline void SetPredictionMode(EGMC_PredictionMode Mode, GMCReplication::FSyncSettings& Settings)
{
  switch (Mode)
  {
    case EGMC_PredictionMode::ServerAuth_Output_ClientValidated:
    {
      Settings.ServerSettings.OutputState.bReplicateForPrediction = true;
      Settings.ServerSettings.OutputState.bReplicateForCorrection = true;
      Settings.ClientSettings.OutputState.bValidate = true;
      break;
    }
    case EGMC_PredictionMode::ServerAuth_Input_ClientValidated:
    {
      Settings.ServerSettings.InputState.bReplicateForPrediction = true;
      Settings.ServerSettings.InputState.bReplicateForCorrection = true;
      Settings.ClientSettings.InputState.bValidate = true;
      break;
    }
    case EGMC_PredictionMode::ServerAuth_InputOutput_ClientValidated:
    {
      Settings.ServerSettings.InputState.bReplicateForPrediction = true;
      Settings.ServerSettings.InputState.bReplicateForCorrection = true;
      Settings.ClientSettings.InputState.bValidate = true;
      Settings.ServerSettings.OutputState.bReplicateForPrediction = true;
      Settings.ServerSettings.OutputState.bReplicateForCorrection = true;
      Settings.ClientSettings.OutputState.bValidate = true;
      break;
    }
    case EGMC_PredictionMode::ServerAuth_Output_ServerValidated:
    {
      Settings.ServerSettings.OutputState.bReplicateForCorrection = true;
      Settings.ServerSettings.OutputState.bValidate = true;
      Settings.ClientSettings.OutputState.bSendToServer = true;
      Settings.ClientSettings.OutputState.bValidate = true;
      break;
    }
    case EGMC_PredictionMode::ServerAuth_Input_ServerValidated:
    {
      Settings.ServerSettings.InputState.bReplicateForCorrection = true;
      Settings.ServerSettings.InputState.bValidate = true;
      Settings.ClientSettings.InputState.bSendToServer = true;
      Settings.ClientSettings.InputState.bValidate = true;
      break;
    }
    case EGMC_PredictionMode::ServerAuth_InputOutput_ServerValidated:
    {
      Settings.ServerSettings.InputState.bReplicateForCorrection = true;
      Settings.ServerSettings.InputState.bValidate = true;
      Settings.ClientSettings.InputState.bSendToServer = true;
      Settings.ClientSettings.InputState.bValidate = true;
      Settings.ServerSettings.OutputState.bReplicateForCorrection = true;
      Settings.ServerSettings.OutputState.bValidate = true;
      Settings.ClientSettings.OutputState.bSendToServer = true;
      Settings.ClientSettings.OutputState.bValidate = true;
      break;
    }
    case EGMC_PredictionMode::ClientAuth_Output:
    {
      Settings.ServerSettings.OutputState.bUseClientValue = true;
      Settings.ClientSettings.OutputState.bSendToServer = true;
      break;
    }
    case EGMC_PredictionMode::ClientAuth_Input:
    {
      Settings.ServerSettings.InputState.bUseClientValue = true;
      Settings.ClientSettings.InputState.bSendToServer = true;
      break;
    }
    case EGMC_PredictionMode::ClientAuth_InputOutput:
    {
      Settings.ServerSettings.InputState.bUseClientValue = true;
      Settings.ClientSettings.InputState.bSendToServer = true;
      Settings.ServerSettings.OutputState.bUseClientValue = true;
      Settings.ClientSettings.OutputState.bSendToServer = true;
      break;
    }
    case EGMC_PredictionMode::Local:
    {
      break;
    }
    default:
    {
      gmc_ckne();
    }
  }
}

inline void SetCombineMode(EGMC_CombineMode Mode, GMCReplication::FSyncSettings& Settings)
{
  Settings.ClientSettings.InputState.CombineMode = static_cast<uint8>(Mode);
  Settings.ClientSettings.OutputState.CombineMode = static_cast<uint8>(Mode); // Not relevant for the output state but set anyway for consistency.
}

inline void SetSimulationMode(EGMC_SimulationMode Mode, GMCReplication::FSyncSettings& Settings)
{
  switch (Mode)
  {
    case EGMC_SimulationMode::Periodic_Output:
    {
      Settings.ServerSettings.OutputState.bReplicateForSimulation = true;
      break;
    }
    case EGMC_SimulationMode::Periodic_Input:
    {
      Settings.ServerSettings.InputState.bReplicateForSimulation = true;
      break;
    }
    case EGMC_SimulationMode::Periodic_InputOutput:
    {
      Settings.ServerSettings.InputState.bReplicateForSimulation = true;
      Settings.ServerSettings.OutputState.bReplicateForSimulation = true;
      break;
    }
    case EGMC_SimulationMode::PeriodicAndOnChange_Output:
    {
      Settings.ServerSettings.OutputState.bReplicateForSimulation = true;
      Settings.ServerSettings.OutputState.bForceUpdateOnChange = true;
      break;
    }
    case EGMC_SimulationMode::PeriodicAndOnChange_Input:
    {
      Settings.ServerSettings.InputState.bReplicateForSimulation = true;
      Settings.ServerSettings.InputState.bForceUpdateOnChange = true;
      break;
    }
    case EGMC_SimulationMode::PeriodicAndOnChange_InputOutput:
    {
      Settings.ServerSettings.InputState.bReplicateForSimulation = true;
      Settings.ServerSettings.InputState.bForceUpdateOnChange = true;
      Settings.ServerSettings.OutputState.bReplicateForSimulation = true;
      Settings.ServerSettings.OutputState.bForceUpdateOnChange = true;
      break;
    }
    case EGMC_SimulationMode::None:
    {
      break;
    }
    default:
    {
      gmc_ckne();
    }
  }
}

inline void SetInterpolationFunction(EGMC_InterpolationFunction InterpFunc, GMCReplication::FSyncSettings& Settings)
{
  Settings.LocalSettings.InterpFunc = static_cast<uint8>(InterpFunc);
}

struct FGMC_PackedReplicationSettings
{
  EGMC_PredictionMode PredictionMode{};
  EGMC_CombineMode CombineMode{};
  EGMC_SimulationMode SimulationMode{};
  EGMC_InterpolationFunction InterpolationFunction{};
};

struct FGMC_SyncTagsData
{};

inline FGMC_PackedReplicationSettings GetNonDefaultReplicationSettings(EGMC_SyncTag Tag, const FGMC_SyncTagsData& Data)
{
  gmc_ckne()
  return FGMC_PackedReplicationSettings{};
}

inline GMCReplication::FSyncSettings TranslateToSyncSettings(
  EGMC_PredictionMode PredictionMode,
  EGMC_CombineMode CombineMode,
  EGMC_SimulationMode SimulationMode,
  EGMC_InterpolationFunction InterpolationFunction
)
{
  GMCReplication::FSyncSettings InternalSettings{};

  SetPredictionMode(PredictionMode, InternalSettings);
  SetCombineMode(CombineMode, InternalSettings);
  SetSimulationMode(SimulationMode, InternalSettings);
  SetInterpolationFunction(InterpolationFunction, InternalSettings);

  return InternalSettings;
}

inline GMCReplication::FSyncSettings TranslateToSyncSettings(const FGMC_PackedReplicationSettings& ReplicationSettings)
{
  return TranslateToSyncSettings(
    ReplicationSettings.PredictionMode,
    ReplicationSettings.CombineMode,
    ReplicationSettings.SimulationMode,
    ReplicationSettings.InterpolationFunction
  );
}

inline bool IsClientAuth(EGMC_PredictionMode Mode)
{
  return
    Mode == EGMC_PredictionMode::ClientAuth_Output ||
    Mode == EGMC_PredictionMode::ClientAuth_Input ||
    Mode == EGMC_PredictionMode::ClientAuth_InputOutput;
}

inline bool IsClientAuthInput(EGMC_PredictionMode Mode)
{
  return
    Mode == EGMC_PredictionMode::ClientAuth_Input ||
    Mode == EGMC_PredictionMode::ClientAuth_InputOutput;
}

inline bool IsClientAuthOutput(EGMC_PredictionMode Mode)
{
  return
    Mode == EGMC_PredictionMode::ClientAuth_Output ||
    Mode == EGMC_PredictionMode::ClientAuth_InputOutput;
}

inline bool IsServerAuthClientValidated(EGMC_PredictionMode Mode)
{
  return
    Mode == EGMC_PredictionMode::ServerAuth_Output_ClientValidated ||
    Mode == EGMC_PredictionMode::ServerAuth_Input_ClientValidated ||
    Mode == EGMC_PredictionMode::ServerAuth_InputOutput_ClientValidated;
}

inline bool IsServerAuthInputClientValidated(EGMC_PredictionMode Mode)
{
  return
    Mode == EGMC_PredictionMode::ServerAuth_Input_ClientValidated ||
    Mode == EGMC_PredictionMode::ServerAuth_InputOutput_ClientValidated;
}

inline bool IsServerAuthOutputClientValidated(EGMC_PredictionMode Mode)
{
  return
    Mode == EGMC_PredictionMode::ServerAuth_Output_ClientValidated ||
    Mode == EGMC_PredictionMode::ServerAuth_InputOutput_ClientValidated;
}

inline bool IsServerAuthServerValidated(EGMC_PredictionMode Mode)
{
  return
    Mode == EGMC_PredictionMode::ServerAuth_Output_ServerValidated ||
    Mode == EGMC_PredictionMode::ServerAuth_Input_ServerValidated ||
    Mode == EGMC_PredictionMode::ServerAuth_InputOutput_ServerValidated;
}

inline bool IsServerAuthInputServerValidated(EGMC_PredictionMode Mode)
{
  return
    Mode == EGMC_PredictionMode::ServerAuth_Input_ServerValidated ||
    Mode == EGMC_PredictionMode::ServerAuth_InputOutput_ServerValidated;
}

inline bool IsServerAuthOutputServerValidated(EGMC_PredictionMode Mode)
{
  return
    Mode == EGMC_PredictionMode::ServerAuth_Output_ServerValidated ||
    Mode == EGMC_PredictionMode::ServerAuth_InputOutput_ServerValidated;
}

inline bool IsServerAuth(EGMC_PredictionMode Mode)
{
  return IsServerAuthClientValidated(Mode) || IsServerAuthServerValidated(Mode);
}

inline bool IsServerAuthInput(EGMC_PredictionMode Mode)
{
  return IsServerAuthInputClientValidated(Mode) || IsServerAuthInputServerValidated(Mode);
}

inline bool IsServerAuthOutput(EGMC_PredictionMode Mode)
{
  return IsServerAuthOutputClientValidated(Mode) || IsServerAuthOutputServerValidated(Mode);
}

inline bool IsPeriodic(EGMC_SimulationMode Mode)
{
  return
    Mode == EGMC_SimulationMode::Periodic_Input ||
    Mode == EGMC_SimulationMode::Periodic_Output ||
    Mode == EGMC_SimulationMode::Periodic_InputOutput;
}

inline bool IsPeriodicInput(EGMC_SimulationMode Mode)
{
  return
    Mode == EGMC_SimulationMode::Periodic_Input ||
    Mode == EGMC_SimulationMode::Periodic_InputOutput;
}

inline bool IsPeriodicOutput(EGMC_SimulationMode Mode)
{
  return
    Mode == EGMC_SimulationMode::Periodic_Output ||
    Mode == EGMC_SimulationMode::Periodic_InputOutput;
}

inline bool IsPeriodicAndOnChange(EGMC_SimulationMode Mode)
{
  return
    Mode == EGMC_SimulationMode::PeriodicAndOnChange_Input ||
    Mode == EGMC_SimulationMode::PeriodicAndOnChange_Output ||
    Mode == EGMC_SimulationMode::PeriodicAndOnChange_InputOutput;
}

inline bool IsPeriodicAndOnChangeInput(EGMC_SimulationMode Mode)
{
  return
    Mode == EGMC_SimulationMode::PeriodicAndOnChange_Input ||
    Mode == EGMC_SimulationMode::PeriodicAndOnChange_InputOutput;
}

inline bool IsPeriodicAndOnChangeOutput(EGMC_SimulationMode Mode)
{
  return
    Mode == EGMC_SimulationMode::PeriodicAndOnChange_Output ||
    Mode == EGMC_SimulationMode::PeriodicAndOnChange_InputOutput;
}
