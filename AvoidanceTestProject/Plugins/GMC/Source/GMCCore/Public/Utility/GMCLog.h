// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GMCAssert.h"
#include "GMCLogCategory.h"
#include "GMCOptimize.h"
#include "FloatPrecision.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/EngineBaseTypes.h"

// Unreal Engine grid scale: 1uu = 1cm.
#define UU_METER      (100.)
#define UU_CENTIMETER (1.)
#define UU_MILLIMETER (1.e-1)
#define UU_MICROMETER (1.e-4)
#define UU_NANOMETER  (1.e-7)

// Values of the dot product between two normalized vectors at different angles (delta degrees).
#define DOT_PRODUCT_0   (1.)
#define DOT_PRODUCT_15  (0.966)
#define DOT_PRODUCT_30  (0.866)
#define DOT_PRODUCT_45  (0.707)
#define DOT_PRODUCT_60  (0.5)
#define DOT_PRODUCT_75  (0.259)
#define DOT_PRODUCT_90  (0.)
#define DOT_PRODUCT_105 (-0.259)
#define DOT_PRODUCT_120 (-0.5)
#define DOT_PRODUCT_135 (-0.707)
#define DOT_PRODUCT_150 (-0.866)
#define DOT_PRODUCT_165 (-0.966)
#define DOT_PRODUCT_180 (-1.)

#define TO_STR(X) (TEXT(#X))

#define BOOL_TO_STR(X) ((X) ? TEXT("true") : TEXT("false"))

#define GET_NAME(X) ((X) ? *(X)->GetName() : TEXT("nullptr"))

template<typename T> T& Unused(T&& Var) { return Var; }
#define UNUSED(Type) (Unused(Type{}))

inline FString GetNetRoleAsString(ENetRole Role)
{
  switch (Role)
  {
    case ROLE_None:
      return TEXT("None");
    case ROLE_SimulatedProxy:
      return TEXT("Simulated Proxy");
    case ROLE_AutonomousProxy:
      return TEXT("Autonomous Proxy");
    case ROLE_Authority:
      return TEXT("Authority");
    case ROLE_MAX:
      return TEXT("MAX");
    default:
      gmc_ckne();
  }
  gmc_ckne()
  return "";
}

inline FString GetNetModeAsString(ENetMode Mode)
{
  switch (Mode)
  {
    case NM_Standalone:
      return TEXT("Standalone");
    case NM_DedicatedServer:
      return TEXT("Dedicated Server");
    case NM_ListenServer:
      return TEXT("Listen Server");
    case NM_Client:
      return TEXT("Client");
    case NM_MAX:
      return TEXT("MAX");
    default:
      gmc_ckne();
  }
  gmc_ckne()
  return "";
}

inline FString GetVector2DAsString(const FVector2D& Value, EGMC_FloatPrecision Precision)
{
  switch (Precision)
  {
    case WholeNumber:
        return FString::Printf(TEXT("X=%.0f Y=%.0f"), Value.X, Value.Y);
    case OneDecimal:
        return FString::Printf(TEXT("X=%.1f Y=%.1f"), Value.X, Value.Y);
    case TwoDecimals:
        return FString::Printf(TEXT("X=%.2f Y=%.2f"), Value.X, Value.Y);
    case ThreeDecimals:
        return FString::Printf(TEXT("X=%.3f Y=%.3f"), Value.X, Value.Y);
    case FourDecimals:
        return FString::Printf(TEXT("X=%.4f Y=%.4f"), Value.X, Value.Y);
    case FiveDecimals:
        return FString::Printf(TEXT("X=%.5f Y=%.5f"), Value.X, Value.Y);
    case SixDecimals:
        return FString::Printf(TEXT("X=%.6f Y=%.6f"), Value.X, Value.Y);
    case FullPrecision:
        return FString::Printf(TEXT("X=%.14f Y=%.14f"), Value.X, Value.Y);
    default:
      gmc_ckne();
  }
  gmc_ckne()
  return "";
}

inline FString GetVectorAsString(const FVector& Value, EGMC_FloatPrecision Precision)
{
  switch (Precision)
  {
    case WholeNumber:
        return FString::Printf(TEXT("X=%.0f Y=%.0f Z=%.0f"), Value.X, Value.Y, Value.Z);
    case OneDecimal:
        return FString::Printf(TEXT("X=%.1f Y=%.1f Z=%.1f"), Value.X, Value.Y, Value.Z);
    case TwoDecimals:
        return FString::Printf(TEXT("X=%.2f Y=%.2f Z=%.2f"), Value.X, Value.Y, Value.Z);
    case ThreeDecimals:
        return FString::Printf(TEXT("X=%.3f Y=%.3f Z=%.3f"), Value.X, Value.Y, Value.Z);
    case FourDecimals:
        return FString::Printf(TEXT("X=%.4f Y=%.4f Z=%.4f"), Value.X, Value.Y, Value.Z);
    case FiveDecimals:
        return FString::Printf(TEXT("X=%.5f Y=%.5f Z=%.5f"), Value.X, Value.Y, Value.Z);
    case SixDecimals:
        return FString::Printf(TEXT("X=%.6f Y=%.6f Z=%.6f"), Value.X, Value.Y, Value.Z);
    case FullPrecision:
        return FString::Printf(TEXT("X=%.14f Y=%.14f Z=%.14f"), Value.X, Value.Y, Value.Z);
    default:
      gmc_ckne();
  }
  gmc_ckne()
  return "";
}

inline FString GetRotatorAsString(const FRotator& Value, EGMC_FloatPrecision Precision)
{
  switch (Precision)
  {
    case WholeNumber:
        return FString::Printf(TEXT("Roll=%.0f Pitch=%.0f Yaw=%.0f"), Value.Roll, Value.Pitch, Value.Yaw);
    case OneDecimal:
        return FString::Printf(TEXT("Roll=%.1f Pitch=%.1f Yaw=%.1f"), Value.Roll, Value.Pitch, Value.Yaw);
    case TwoDecimals:
        return FString::Printf(TEXT("Roll=%.2f Pitch=%.2f Yaw=%.2f"), Value.Roll, Value.Pitch, Value.Yaw);
    case ThreeDecimals:
        return FString::Printf(TEXT("Roll=%.3f Pitch=%.3f Yaw=%.3f"), Value.Roll, Value.Pitch, Value.Yaw);
    case FourDecimals:
        return FString::Printf(TEXT("Roll=%.4f Pitch=%.4f Yaw=%.4f"), Value.Roll, Value.Pitch, Value.Yaw);
    case FiveDecimals:
        return FString::Printf(TEXT("Roll=%.5f Pitch=%.5f Yaw=%.5f"), Value.Roll, Value.Pitch, Value.Yaw);
    case SixDecimals:
        return FString::Printf(TEXT("Roll=%.6f Pitch=%.6f Yaw=%.6f"), Value.Roll, Value.Pitch, Value.Yaw);
    case FullPrecision:
        return FString::Printf(TEXT("Roll=%.14f Pitch=%.14f Yaw=%.14f"), Value.Roll, Value.Pitch, Value.Yaw);
    default:
      gmc_ckne();
  }
  gmc_ckne()
  return "";
}

inline FString GetTransformAsString(const FTransform& Value, EGMC_FloatPrecision Precision)
{
  const auto& Translation = Value.GetTranslation();
  const auto& Rotation = Value.GetRotation().Rotator();
  const auto& Scale = Value.GetScale3D();

  switch (Precision)
  {
    case WholeNumber:
      return FString::Printf(
        TEXT("(Location .X=%.0f .Y=%.0f .Z=%.0f) (Rotation .Roll=%.0f .Pitch=%.0f .Yaw=%.0f) (Scale .X=%.0f .Y=%.0f .Z=%.0f)"),
        Translation.X, Translation.Y, Translation.Z,
        Rotation.Roll, Rotation.Pitch, Rotation.Yaw,
        Scale.X, Scale.Y, Scale.Z
      );
    case OneDecimal:
      return FString::Printf(
        TEXT("(Location .X=%.1f .Y=%.1f .Z=%.1f) (Rotation .Roll=%.1f .Pitch=%.1f .Yaw=%.1f) (Scale .X=%.1f .Y=%.1f .Z=%.1f)"),
        Translation.X, Translation.Y, Translation.Z,
        Rotation.Roll, Rotation.Pitch, Rotation.Yaw,
        Scale.X, Scale.Y, Scale.Z
      );
    case TwoDecimals:
      return FString::Printf(
        TEXT("(Location .X=%.2f .Y=%.2f .Z=%.2f) (Rotation .Roll=%.2f .Pitch=%.2f .Yaw=%.2f) (Scale .X=%.2f .Y=%.2f .Z=%.2f)"),
        Translation.X, Translation.Y, Translation.Z,
        Rotation.Roll, Rotation.Pitch, Rotation.Yaw,
        Scale.X, Scale.Y, Scale.Z
      );
    case ThreeDecimals:
      return FString::Printf(
        TEXT("(Location .X=%.3f .Y=%.3f .Z=%.3f) (Rotation .Roll=%.3f .Pitch=%.3f .Yaw=%.3f) (Scale .X=%.3f .Y=%.3f .Z=%.3f)"),
        Translation.X, Translation.Y, Translation.Z,
        Rotation.Roll, Rotation.Pitch, Rotation.Yaw,
        Scale.X, Scale.Y, Scale.Z
      );
    case FourDecimals:
      return FString::Printf(
        TEXT("(Location .X=%.4f .Y=%.4f .Z=%.4f) (Rotation .Roll=%.4f .Pitch=%.4f .Yaw=%.4f) (Scale .X=%.4f .Y=%.4f .Z=%.4f)"),
        Translation.X, Translation.Y, Translation.Z,
        Rotation.Roll, Rotation.Pitch, Rotation.Yaw,
        Scale.X, Scale.Y, Scale.Z
      );
    case FiveDecimals:
      return FString::Printf(
        TEXT("(Location .X=%.5f .Y=%.5f .Z=%.5f) (Rotation .Roll=%.5f .Pitch=%.5f .Yaw=%.5f) (Scale .X=%.5f .Y=%.5f .Z=%.5f)"),
        Translation.X, Translation.Y, Translation.Z,
        Rotation.Roll, Rotation.Pitch, Rotation.Yaw,
        Scale.X, Scale.Y, Scale.Z
      );
    case SixDecimals:
      return FString::Printf(
        TEXT("(Location .X=%.6f .Y=%.6f .Z=%.6f) (Rotation .Roll=%.6f .Pitch=%.6f .Yaw=%.6f) (Scale .X=%.6f .Y=%.6f .Z=%.6f)"),
        Translation.X, Translation.Y, Translation.Z,
        Rotation.Roll, Rotation.Pitch, Rotation.Yaw,
        Scale.X, Scale.Y, Scale.Z
      );
    case FullPrecision:
      return FString::Printf(
        TEXT("(Location .X=%.14f .Y=%.14f .Z=%.14f) (Rotation .Roll=%.14f .Pitch=%.14f .Yaw=%.14f) (Scale .X=%.14f .Y=%.14f .Z=%.14f)"),
        Translation.X, Translation.Y, Translation.Z,
        Rotation.Roll, Rotation.Pitch, Rotation.Yaw,
        Scale.X, Scale.Y, Scale.Z
      );
    default:
      gmc_ckne();
  }
  gmc_ckne()
  return "";
}

inline FString GetObjectNameWithOuter(const UObject* Object)
{
  if (IsValid(Object))
  {
    UObject* StopOuter = nullptr;
    const auto& Outer1 = Object->GetOuter();
    if (IsValid(Outer1))
    {
      StopOuter = Outer1;
      const auto& Outer2 = Outer1->GetOuter();
      if (IsValid(Outer2))
      {
        StopOuter = Outer2;
      }
    }

    if (IsValid(StopOuter))
    {
      return FString::Printf(TEXT("%s"), *Object->GetPathName(StopOuter));
    }

    return FString::Printf(TEXT("%s"), *Object->GetName());
  }

  return FString{TEXT("Null")};
}

#if !NO_LOGGING

#ifdef __COUNTER__

#define DEBUG_PRINT_MSG(Duration, Format, ...)\
  if (GEngine)\
  {\
    GEngine->AddOnScreenDebugMessage(\
      uint64(sizeof(__FILE__) + sizeof(__func__) + __LINE__ + __COUNTER__),\
      Duration,\
      FColor::Orange,\
      FString::Printf(TEXT(Format), ##__VA_ARGS__),\
      false,\
      FVector2D(0.9, 0.9)\
    );\
  }

#else

#define DEBUG_PRINT_MSG(Duration, FormatStr, ...)

#endif

#define GMC_LOG(Category, NetOwner, Verbosity, Format, ...)\
  {\
    if (NetOwner && Cast<AActor>(NetOwner))\
    {\
      const auto& GMCLocalPC = GEngine ? GEngine->GetFirstLocalPlayerController(NetOwner->GetWorld()) : nullptr;\
      FString\
      GMCTime = FDateTime::Now().GetTimeOfDay().ToString(),\
      GMCPlayerName = GMCLocalPC ? GMCLocalPC->GetHumanReadableName() : TEXT(""),\
      GMCActorName = NetOwner->GetName(),\
      GMCNetMode = GetNetModeAsString(NetOwner->GetNetMode()),\
      GMCNetworkRole = TEXT("");\
      ENetRole GMCRole = NetOwner->GetLocalRole();\
      const auto GMCPawnNetOwner = NetOwner->IsA<APawn>() ? (APawn*)NetOwner : nullptr;\
      if (GMCRole == ROLE_Authority && GMCPawnNetOwner)\
      {\
        FString GMCControl = GMCPawnNetOwner->IsLocallyControlled() ? TEXT("local  ") : TEXT("remote ");\
        GMCNetworkRole = GMCControl + GetNetRoleAsString(GMCRole);\
      }\
      else\
      {\
        GMCNetworkRole = GetNetRoleAsString(GMCRole);\
      }\
      LOG_INTERNAL(Category, Verbosity, OBJECT_INFO_TEXT(Format), OBJECT_INFO(GMCTime, GMCPlayerName, GMCActorName, GMCNetMode, GMCNetworkRole), ##__VA_ARGS__)\
    }\
    else\
    {\
      LOG_INTERNAL(Category, Verbosity, TEXT("| No actor debug info available | ") Format, ##__VA_ARGS__)\
    }\
  }
#define GMC_CLOG(Condition, Category, NetOwner, Verbosity, Format, ...) if (Condition) { GMC_LOG(Category, NetOwner, Verbosity, Format, ##__VA_ARGS__) }
#define OBJECT_INFO_TEXT(Text) TEXT("%s %-20s (%-s: %-16s | %-16s): " Text)
#define OBJECT_INFO(Time, PlayerName, ActorName, NetMode, NetworkRole) *Time, *PlayerName, *ActorName, *NetMode, *NetworkRole
#define LOG_INTERNAL(Category, Verbosity, Format, ...) UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__)

#else

#define DEBUG_PRINT_MSG(Duration, Format, ...)
#define GMC_LOG(Category, NetOwner, Verbosity, Format, ...)
#define GMC_CLOG(Condition, Category, NetOwner, Verbosity, Format, ...)
#define OBJECT_INFO_TEXT(Text)
#define OBJECT_INFO(Time, PlayerName, ActorName, NetMode, NetworkRole)
#define LOG_INTERNAL(Category, Verbosity, Format, ...)

#endif
