// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Animation/TimeStretchCurve.h"
#include "GMCAnimMontage.generated.h"

enum class EGMC_MontageSubStepResult : uint8
{
  Moved,
  NotMoved,
  InvalidSection,
  InvalidMontage
};

struct GMCCORE_API FGMC_MontageSubStepper
{
  FGMC_MontageSubStepper(struct FGMC_AnimMontageInstance* InAnimInstance, float EvaluationTime);

  bool HasTimeRemaining() const;
  float GetRemainingTime() const;
  bool HasReachedEndOfSection() const;
  bool GetPlayingForward() const;
  int32 GetCurrentSectionIndex() const;
  EGMC_MontageSubStepResult Advance(float& InOutPosition_Original);

private:

  FGMC_AnimMontageInstance* MontageInstance{nullptr};

  float TimeRemaining{0.f};
  float PlayRate{0.f};
  float DeltaMove{0.f};
  bool bPlayingForward{true};

  int32 CurrentSectionIndex{INDEX_NONE};
  float CurrentSectionStartTime{0.f};
  float CurrentSectionLength{0.f};
  bool bReachedEndOfSection{false};

  FTimeStretchCurveInstance TimeStretchCurveInstance{};
  bool bHasValidTimeStretchCurveData{false};
  int32 TimeStretchMarkerIndex{INDEX_NONE};
  float CachedCombinedPlayRate{FLT_MAX};
  float CachedPosition_Target{FLT_MAX};
  float CachedPosition_Original{FLT_MAX};
  TArray<float> SectionStartPositions_Target{};
  TArray<float> SectionEndPositions_Target{};

  float FindMontagePosition_Target(float InPosition_Original);
  float FindMontagePosition_Original(float InPosition_Target);
  void ConditionallyUpdateTimeStretchCurveCachedData(class UAnimMontage* Montage);
};

struct GMCCORE_API FGMC_AnimMontageInstance
{
  FGMC_AnimMontageInstance(UAnimMontage* InMontage, float InPosition, float InPlayRate, bool bCalculateExtraData);

  UAnimMontage* GetMontage() const;
  bool IsValid() const;
  float GetPosition() const;
  float GetPlayRate() const;
  FName GetCurrentSection(float& OutPositionWithinSection) const;
  FName GetNextSection() const;
  FName GetPreviousSection() const;
  int32 GetCurrentSectionID(float& OutPositionWithinSection) const;
  int32 GetNextSectionID() const;
  int32 GetPreviousSectionID() const;
  FName GetSectionNameFromID(int32 SectionID) const;
  bool HasLoopingSections() const;
  float GetBlendInFinishedPosition() const;
  float GetBlendOutStartedPosition() const;
  float GetTimeToLastSectionEnd() const;
  int32 GetLastSectionIndex() const;
  int32 GetBlendInFinishedPositionIndex() const;
  int32 GetBlendOutStartedPositionIndex() const;
  const TArray<int32>& GetPreviousSections() const;
  const TArray<int32>& GetNextSections() const;
  bool Advance(
    float DeltaTime,
    FRootMotionMovementParams& OutRootMotionParams,
    bool& bOutFinishedBlendIn,
    bool& bOutStartedBlendOut,
    TArray<const FAnimNotifyEvent*>& OutMontageNotifyBeginEvents,
    TArray<const FAnimNotifyEvent*>& OutMontageNotifyEndEvents
  );

private:

  TObjectPtr<UAnimMontage> Montage{nullptr};

  float Position{0.f};
  float PlayRate{0.f};

  // Base data.
  TArray<int32> NextSections{};
  TArray<int32> PrevSections{};
  void SetSections();

  // Extra data.
  bool bHasLoopingSections{false};
  float BlendInFinishedPosition{0.f};
  float BlendOutStartedPosition{0.f};
  float TimeToLastSectionEnd{0.f};
  int32 LastSectionIndex{0};
  int32 BlendInFinishedPositionIndex{0};
  int32 BlendOutStartedPositionIndex{0};
  bool DetermineLoopingSections(int32& OutLastSectionIndex, float& OutTimeToLastSectionEnd) const;
  float GetPositionFromTime(float TimeIntoMontage) const;
};

USTRUCT(BlueprintType)
struct GMCCORE_API FGMC_MontageTracker
{
  GENERATED_BODY()

  // Whether the active montage was just started. This flag must be reset manually.
  bool bStartedNewMontage{false};

  // Whether the active montage has finished. This flag must be reset manually.
  bool bMontageEnded{false};

  // Whether the active montage interrupted a previous one. This flag must be reset manually.
  bool bInterruptedPreviousMontage{false};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  // The active montage.
  UAnimMontage* Montage{nullptr};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  // The position of the active montage.
  float MontagePosition{0.f};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  // The play rate of the active montage.
  float MontagePlayRate{1.f};

  UPROPERTY(BlueprintReadWrite, Category = "General Movement Component")
  // Whether the active montage is currently paused.
  bool bMontagePaused{false};

  UPROPERTY(Transient)
  // Prevents the callback proxy from being garbage collected until all GMC delegates have fired.
  UObject* CallbackProxyRef{nullptr};

  bool SetActiveMontage(UAnimMontage* InMontage, float InPosition, float InPlayRate);
  void ClearActiveMontage();
  bool HasActiveMontage() const;
  bool HasActiveRootMotionMontage() const;
  bool HasActiveNonRootMotionMontage() const;
};
