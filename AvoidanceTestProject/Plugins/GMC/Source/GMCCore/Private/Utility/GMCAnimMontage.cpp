// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCAnimMontage.h"
#include "GMCLog.h"

FGMC_MontageSubStepper::FGMC_MontageSubStepper(FGMC_AnimMontageInstance* InAnimInstance, float EvaluationTime)
{
  if (InAnimInstance && InAnimInstance->GetMontage())
  {
    MontageInstance = InAnimInstance;
    TimeRemaining = EvaluationTime;
    return;
  }

  gmc_ckne()
}

bool FGMC_MontageSubStepper::HasTimeRemaining() const
{
  return TimeRemaining > UE_SMALL_NUMBER;
}

float FGMC_MontageSubStepper::GetRemainingTime() const
{
  return TimeRemaining;
}

bool FGMC_MontageSubStepper::HasReachedEndOfSection() const
{
  return bReachedEndOfSection;
}

bool FGMC_MontageSubStepper::GetPlayingForward() const
{
  return bPlayingForward;
}

int32 FGMC_MontageSubStepper::GetCurrentSectionIndex() const
{
  return CurrentSectionIndex;
}

EGMC_MontageSubStepResult FGMC_MontageSubStepper::Advance(float& InOutPosition_Original)
{
  if (!MontageInstance)
  {
    gmc_ckne()
    return EGMC_MontageSubStepResult::InvalidMontage;
  }

  const auto& Montage = MontageInstance->GetMontage();
  if (!Montage)
  {
    gmc_ckne()
    return EGMC_MontageSubStepResult::InvalidMontage;
  }

  const float MontagePlayLength = Montage->GetPlayLength();

  gmc_ck(InOutPosition_Original >= 0.)
  gmc_ck(InOutPosition_Original <= MontagePlayLength)

  DeltaMove = 0.f;
  bReachedEndOfSection = false;

  float PositionWithinSection{0.f};
  CurrentSectionIndex = Montage->GetAnimCompositeSectionIndexFromPos(InOutPosition_Original, PositionWithinSection);

  if (!Montage->IsValidSectionIndex(CurrentSectionIndex))
  {
    gmc_ckne()
    return EGMC_MontageSubStepResult::InvalidSection;
  }

  const FCompositeSection& CurrentSection = Montage->GetAnimCompositeSection(CurrentSectionIndex);
  CurrentSectionStartTime = CurrentSection.GetTime();
  CurrentSectionLength = Montage->GetSectionLength(CurrentSectionIndex);

  if (FMath::IsNearlyZero(TimeRemaining))
  {
    return EGMC_MontageSubStepResult::NotMoved;
  }

  PlayRate = MontageInstance->GetPlayRate() * Montage->RateScale;

  if (FMath::IsNearlyZero(PlayRate))
  {
    return EGMC_MontageSubStepResult::NotMoved;
  }

  const bool bAttemptTimeStretchCurve = Montage->TimeStretchCurve.IsValid() && !FMath::IsNearlyEqual(PlayRate, 1.f);
  if (bAttemptTimeStretchCurve)
  {
    ConditionallyUpdateTimeStretchCurveCachedData(Montage);
  }

  if (!bAttemptTimeStretchCurve || !bHasValidTimeStretchCurveData)
  {
    bPlayingForward = PlayRate > 0.f;
    DeltaMove = TimeRemaining * PlayRate;
    TimeStretchMarkerIndex = INDEX_NONE;
  }
  else
  {
    float Position_Target = FindMontagePosition_Target(InOutPosition_Original);
    Position_Target += bPlayingForward ? TimeRemaining : -TimeRemaining;
    Position_Target = TimeStretchCurveInstance.Clamp_P_Target(Position_Target);
    const float NewPosition_Original = FindMontagePosition_Original(Position_Target);
    DeltaMove = NewPosition_Original - InOutPosition_Original;
    PlayRate = DeltaMove / TimeRemaining;
  }

  if (bPlayingForward)
  {
    const float MaxSectionMove = CurrentSectionLength - PositionWithinSection;
    if (DeltaMove >= MaxSectionMove)
    {
      DeltaMove = MaxSectionMove;
      bReachedEndOfSection = true;
    }
  }
  else
  {
    const float MinSectionMove = -PositionWithinSection;
    if (DeltaMove <= MinSectionMove)
    {
      DeltaMove = MinSectionMove;
      bReachedEndOfSection = true;
    }
  }

  if (FMath::Abs(DeltaMove) > 0.f)
  {
    InOutPosition_Original += DeltaMove;
    InOutPosition_Original = FMath::Clamp(InOutPosition_Original, 0.f, MontagePlayLength);

    const float TimeStep = DeltaMove / PlayRate;
    gmc_ck(TimeStep >= 0.f)

    TimeRemaining = FMath::Max(TimeRemaining - TimeStep, 0.f);

    return EGMC_MontageSubStepResult::Moved;
  }

  return EGMC_MontageSubStepResult::NotMoved;
}

float FGMC_MontageSubStepper::FindMontagePosition_Target(float InPosition_Original)
{
  gmc_ck(bHasValidTimeStretchCurveData)

  if (InPosition_Original != CachedPosition_Original)
  {
    CachedPosition_Original = InPosition_Original;
    TimeStretchCurveInstance.UpdateMarkerIndexForPosition(TimeStretchMarkerIndex, CachedPosition_Original, TimeStretchCurveInstance.GetMarkers_Original());
    CachedPosition_Target = TimeStretchCurveInstance.Convert_P_Original_To_Target(TimeStretchMarkerIndex, CachedPosition_Original);
  }

  return CachedPosition_Target;
}

float FGMC_MontageSubStepper::FindMontagePosition_Original(float InPosition_Target)
{
  gmc_ck(bHasValidTimeStretchCurveData)

  if (InPosition_Target != CachedPosition_Target)
  {
    CachedPosition_Target = InPosition_Target;
    TimeStretchCurveInstance.UpdateMarkerIndexForPosition(TimeStretchMarkerIndex, CachedPosition_Target, TimeStretchCurveInstance.GetMarkers_Target());
    CachedPosition_Original = TimeStretchCurveInstance.Convert_P_Target_To_Original(TimeStretchMarkerIndex, CachedPosition_Target);
  }

  return CachedPosition_Original;
}

void FGMC_MontageSubStepper::ConditionallyUpdateTimeStretchCurveCachedData(UAnimMontage* Montage)
{
  gmc_ck(Montage)

  const float CombinedPlayRate = MontageInstance->GetPlayRate() * Montage->RateScale;

  if (CombinedPlayRate == CachedCombinedPlayRate)
  {
    return;
  }

  CachedCombinedPlayRate = CombinedPlayRate;

  bHasValidTimeStretchCurveData = false;

  gmc_ck(!FMath::IsNearlyZero(CombinedPlayRate))
  gmc_ck(!FMath::IsNearlyEqual(CombinedPlayRate, 1.f))

  bPlayingForward = CombinedPlayRate > 0.f;
  TimeStretchCurveInstance.InitializeFromPlayRate(CombinedPlayRate, Montage->TimeStretchCurve);

  const int32 NumSections = Montage->CompositeSections.Num();
  SectionStartPositions_Target.Reset(NumSections);
  SectionStartPositions_Target.Init(-1.f, NumSections);
  SectionEndPositions_Target.Reset(NumSections);
  SectionEndPositions_Target.Init(-1.f, NumSections);

  bHasValidTimeStretchCurveData = TimeStretchCurveInstance.HasValidData();
}

FGMC_AnimMontageInstance::FGMC_AnimMontageInstance(UAnimMontage* InMontage, float InPosition, float InPlayRate, bool bCalculateExtraData)
{
  if (::IsValid(InMontage) && InMontage->GetPlayLength() > 0.f && InPosition >= 0.f && InPosition <= InMontage->GetPlayLength())
  {
    Montage = InMontage;
    Position = InPosition;
    PlayRate = InPlayRate;

    SetSections();

    if (bCalculateExtraData)
    {
      bHasLoopingSections = DetermineLoopingSections(LastSectionIndex, TimeToLastSectionEnd);
      BlendInFinishedPosition = GetPositionFromTime(Montage->GetDefaultBlendInTime());
      BlendOutStartedPosition = GetPositionFromTime(TimeToLastSectionEnd - Montage->GetDefaultBlendOutTime());
      BlendInFinishedPositionIndex = Montage->GetAnimCompositeSectionIndexFromPos(BlendInFinishedPosition, UNUSED(float));
      BlendOutStartedPositionIndex = Montage->GetAnimCompositeSectionIndexFromPos(BlendOutStartedPosition, UNUSED(float));
    }
  }
}

UAnimMontage* FGMC_AnimMontageInstance::GetMontage() const
{
  return Montage;
}

bool FGMC_AnimMontageInstance::IsValid() const
{
  return static_cast<bool>(Montage);
}

float FGMC_AnimMontageInstance::GetPosition() const
{
  return Position;
}

float FGMC_AnimMontageInstance::GetPlayRate() const
{
  return PlayRate;
}

void FGMC_AnimMontageInstance::SetSections()
{
  const int32 CompositeSectionsNum = Montage->CompositeSections.Num();

  if (CompositeSectionsNum > 0)
  {
    NextSections.Empty(CompositeSectionsNum);
    NextSections.AddUninitialized(CompositeSectionsNum);
    PrevSections.Empty(CompositeSectionsNum);
    PrevSections.AddUninitialized(CompositeSectionsNum);

    for (int32 Index = 0; Index < CompositeSectionsNum; ++Index)
    {
      PrevSections[Index] = INDEX_NONE;
    }

    for (int32 Index = 0; Index < CompositeSectionsNum; ++Index)
    {
      const FCompositeSection& Section = Montage->CompositeSections[Index];
      const int32 NextSectionIdx = Montage->GetSectionIndex(Section.NextSectionName);
      NextSections[Index] = NextSectionIdx;
      if (NextSections.IsValidIndex(NextSectionIdx))
      {
        PrevSections[NextSectionIdx] = Index;
      }
    }
  }
  else
  {
    gmc_ckne()
  }
}

bool FGMC_AnimMontageInstance::DetermineLoopingSections(int32& OutLastSectionIndex, float& OutTimeToLastSectionEnd) const
{
  gmc_ck(Montage)

  OutLastSectionIndex = 0;
  OutTimeToLastSectionEnd = 0.f;

  int32 SectionIndex = 0;
  gmc_ck(Montage->IsValidSectionIndex(SectionIndex))

  TArray<int32> VisitedSections{};
  while (Montage->IsValidSectionIndex(SectionIndex))
  {
    if (VisitedSections.Contains(SectionIndex))
    {
      return true;
    }
    VisitedSections.Add(SectionIndex);

    OutLastSectionIndex = SectionIndex;
    OutTimeToLastSectionEnd += Montage->GetSectionLength(SectionIndex);

    SectionIndex = NextSections[SectionIndex];
  }

  return false;
}

float FGMC_AnimMontageInstance::GetPositionFromTime(float TimeIntoMontage) const
{
  gmc_ck(Montage)

  if (TimeIntoMontage <= 0.f)
  {
    return UE_KINDA_SMALL_NUMBER;
  }

  float RemainingTime = TimeIntoMontage;

  float SectionStartTime{0.f};
  float SectionEndTime{0.f};

  int32 SectionIndex{0};
  gmc_ck(Montage->IsValidSectionIndex(SectionIndex))

  while (Montage->IsValidSectionIndex(SectionIndex))
  {
    Montage->GetSectionStartAndEndTime(SectionIndex, SectionStartTime, SectionEndTime);

    gmc_ck(SectionEndTime > SectionStartTime)
    const float SectionLength = SectionEndTime - SectionStartTime;

    if (RemainingTime - SectionLength <= 0.f)
    {
      return FMath::Clamp(SectionStartTime + RemainingTime, UE_KINDA_SMALL_NUMBER, SectionEndTime - UE_KINDA_SMALL_NUMBER);
    }

    RemainingTime -= SectionLength;
    gmc_ck(RemainingTime > 0.f)

    if (SectionIndex == LastSectionIndex)
    {
      // Don't go into the looping part of the montage.
      break;
    }

    SectionIndex = NextSections[SectionIndex];
  }

  // The passed time value is outside of the non-looping range of the montage, return the end position of the last section.
  Montage->GetSectionStartAndEndTime(LastSectionIndex, UNUSED(float), SectionEndTime);
  return SectionEndTime - UE_KINDA_SMALL_NUMBER;
}

FName FGMC_AnimMontageInstance::GetCurrentSection(float& OutPositionWithinSection) const
{
  OutPositionWithinSection = 0.f;

  if (Montage)
  {
    const int32 CurrentSectionIndex = GetCurrentSectionID(OutPositionWithinSection);
    if (Montage->IsValidSectionIndex(CurrentSectionIndex))
    {
      const FCompositeSection& CurrentSection = Montage->GetAnimCompositeSection(CurrentSectionIndex);
      return CurrentSection.SectionName;
    }
  }

  return NAME_None;
}

FName FGMC_AnimMontageInstance::GetNextSection() const
{
  return GetSectionNameFromID(GetNextSectionID());
}

FName FGMC_AnimMontageInstance::GetPreviousSection() const
{
  return GetSectionNameFromID(GetPreviousSectionID());
}

int32 FGMC_AnimMontageInstance::GetCurrentSectionID(float& OutPositionWithinSection) const
{
  OutPositionWithinSection = 0.f;

  if (Montage)
  {
    const int32 CurrentSectionIndex = Montage->GetAnimCompositeSectionIndexFromPos(Position, OutPositionWithinSection);
    if (Montage->IsValidSectionIndex(CurrentSectionIndex))
    {
      return CurrentSectionIndex;
    }
  }

  return INDEX_NONE;
}

int32 FGMC_AnimMontageInstance::GetNextSectionID() const
{
  int32 CurrentSectionID = GetCurrentSectionID(UNUSED(float));
  return NextSections.IsValidIndex(CurrentSectionID) ? NextSections[CurrentSectionID] : INDEX_NONE;
}

int32 FGMC_AnimMontageInstance::GetPreviousSectionID() const
{
  int32 CurrentSectionID = GetCurrentSectionID(UNUSED(float));
  return NextSections.IsValidIndex(CurrentSectionID) ? PrevSections[CurrentSectionID] : INDEX_NONE;
}

FName FGMC_AnimMontageInstance::GetSectionNameFromID(int32 SectionID) const
{
  if (Montage && Montage->IsValidSectionIndex(SectionID))
  {
    const FCompositeSection& CurrentSection = Montage->GetAnimCompositeSection(SectionID);
    return CurrentSection.SectionName;
  }

  return NAME_None;
}

bool FGMC_AnimMontageInstance::HasLoopingSections() const
{
  return bHasLoopingSections;
}

float FGMC_AnimMontageInstance::GetBlendInFinishedPosition() const
{
  return BlendInFinishedPosition;
}

float FGMC_AnimMontageInstance::GetBlendOutStartedPosition() const
{
  return BlendOutStartedPosition;
}

float FGMC_AnimMontageInstance::GetTimeToLastSectionEnd() const
{
  return TimeToLastSectionEnd;
}

int32 FGMC_AnimMontageInstance::GetLastSectionIndex() const
{
  return LastSectionIndex;
}

int32 FGMC_AnimMontageInstance::GetBlendInFinishedPositionIndex() const
{
  return BlendInFinishedPositionIndex;
}

int32 FGMC_AnimMontageInstance::GetBlendOutStartedPositionIndex() const
{
  return BlendOutStartedPositionIndex;
}

const TArray<int32>& FGMC_AnimMontageInstance::GetPreviousSections() const
{
  return PrevSections;
}

const TArray<int32>& FGMC_AnimMontageInstance::GetNextSections() const
{
  return NextSections;
}

bool FGMC_AnimMontageInstance::Advance(
  float DeltaTime,
  FRootMotionMovementParams& OutRootMotionParams,
  bool& bOutFinishedBlendIn,
  bool& bOutStartedBlendOut,
  TArray<const FAnimNotifyEvent*>& OutMontageNotifyBeginEvents,
  TArray<const FAnimNotifyEvent*>& OutMontageNotifyEndEvents
)
{
  gmc_ck(IsValid())

  OutRootMotionParams = {};
  bOutFinishedBlendIn = false;
  bOutStartedBlendOut = false;

  const bool bExtractRootMotion = Montage->HasRootMotion();

  FGMC_MontageSubStepper MontageSubStepper{this, DeltaTime};

  const auto GetNextSectionIndex = [this](const FGMC_MontageSubStepper& MontageSubStepper)
  {
    const int32 CurrentSectionIndex = MontageSubStepper.GetCurrentSectionIndex();
    const bool bPlayingForward = MontageSubStepper.GetPlayingForward();
    return bPlayingForward ? NextSections[CurrentSectionIndex] : PrevSections[CurrentSectionIndex];
  };

  while (MontageSubStepper.HasTimeRemaining())
  {
    const float PreviousSubStepPosition = Position;

    EGMC_MontageSubStepResult SubStepResult = MontageSubStepper.Advance(Position);
    gmc_ck(Position >= 0.)
    gmc_ck(Position <= Montage->GetPlayLength())

    if (SubStepResult != EGMC_MontageSubStepResult::Moved)
    {
      return MontageSubStepper.HasReachedEndOfSection() && GetNextSectionIndex(MontageSubStepper) == INDEX_NONE;
    }

    if (bExtractRootMotion)
    {
      OutRootMotionParams.Accumulate(Montage->ExtractRootMotionFromTrackRange(PreviousSubStepPosition, Position));
    }

    const float MinTriggerPosition = UE_KINDA_SMALL_NUMBER;
    const float MaxTriggerPosition = Montage->GetPlayLength() - UE_KINDA_SMALL_NUMBER;

    if (MontageSubStepper.GetCurrentSectionIndex() == GetBlendInFinishedPositionIndex())
    {
      const float BlendInPosition = FMath::Clamp(GetBlendInFinishedPosition(), MinTriggerPosition, MaxTriggerPosition);
      if (Position == BlendInPosition || (PreviousSubStepPosition < BlendInPosition && Position > BlendInPosition))
      {
        bOutFinishedBlendIn = true;
      }
    }

    if (MontageSubStepper.GetCurrentSectionIndex() == GetBlendOutStartedPositionIndex())
    {
      const float BlendOutPosition = FMath::Clamp(GetBlendOutStartedPosition(), MinTriggerPosition, MaxTriggerPosition);
      if (Position == BlendOutPosition || (PreviousSubStepPosition < BlendOutPosition && Position > BlendOutPosition))
      {
        bOutStartedBlendOut = true;
      }
    }

    FAnimTickRecord TickRecord{};
    FAnimNotifyContext NotifyContext(TickRecord);
    Montage->GetAnimNotifiesFromDeltaPositions(PreviousSubStepPosition, Position, NotifyContext);
    for (const auto& NotifyEvent : NotifyContext.ActiveNotifies)
    {
      const auto& Notify = NotifyEvent.GetNotify();
      gmc_ck(Notify)

      const auto& GMCMontageNotify = Cast<UAnimNotify_PlayMontageNotify>(Notify->Notify);
      const auto& GMCMontageNotifyState = Cast<UAnimNotify_PlayMontageNotifyWindow>(Notify->NotifyStateClass);

      if (GMCMontageNotify || GMCMontageNotifyState)
      {
        const float NotifyStartTime = FMath::Clamp(Notify->GetTriggerTime(), MinTriggerPosition, MaxTriggerPosition);
        const bool bTriggered = Position == NotifyStartTime || (PreviousSubStepPosition < NotifyStartTime && Position > NotifyStartTime);
        if (bTriggered)
        {
          OutMontageNotifyBeginEvents.Add(Notify);
        }

        if (GMCMontageNotifyState)
        {
          const float NotifyEndTime = FMath::Clamp(Notify->GetEndTriggerTime(), MinTriggerPosition, MaxTriggerPosition);
          const bool bEnded = Position == NotifyEndTime || (PreviousSubStepPosition < NotifyEndTime && Position > NotifyEndTime);
          if (bEnded)
          {
            OutMontageNotifyEndEvents.Add(Notify);
          }
        }
      }
    }

    if (MontageSubStepper.HasReachedEndOfSection())
    {
      const int32 NextSectionIndex = GetNextSectionIndex(MontageSubStepper);

      if (NextSectionIndex == INDEX_NONE)
      {
        float CurrentSectionStartTime{0.f};
        float CurrentSectionEndTime{0.f};
        Montage->GetSectionStartAndEndTime(MontageSubStepper.GetCurrentSectionIndex(), CurrentSectionStartTime, CurrentSectionEndTime);
        Position = MontageSubStepper.GetPlayingForward() ? CurrentSectionEndTime - UE_KINDA_SMALL_NUMBER : CurrentSectionStartTime;
        return true;
      }

      float NextSectionStartTime{0.f};
      float NextSectionEndTime{0.f};
      Montage->GetSectionStartAndEndTime(NextSectionIndex, NextSectionStartTime, NextSectionEndTime);
      Position = MontageSubStepper.GetPlayingForward() ? NextSectionStartTime : NextSectionEndTime - UE_KINDA_SMALL_NUMBER;

      gmc_ck(Position >= 0.)
      gmc_ck(Position <= Montage->GetPlayLength())
    }
  }

  return MontageSubStepper.HasReachedEndOfSection() && GetNextSectionIndex(MontageSubStepper) == INDEX_NONE;
}

bool FGMC_MontageTracker::SetActiveMontage(UAnimMontage* InMontage, float InPosition, float InPlayRate)
{
  if (IsValid(InMontage) && InMontage->GetPlayLength() > 0.f && InPosition >= 0.f && InPosition <= InMontage->GetPlayLength())
  {
    bStartedNewMontage = true;
    bMontageEnded = false;
    bInterruptedPreviousMontage |= IsValid(Montage);
    Montage = InMontage;
    MontagePosition = InPosition;
    MontagePlayRate = InPlayRate;
    bMontagePaused = false;

    return true;
  }

  return false;
}

void FGMC_MontageTracker::ClearActiveMontage()
{
  bStartedNewMontage = false;
  bMontageEnded = false;
  bInterruptedPreviousMontage = false;
  Montage = nullptr;
  MontagePosition = 0.f;
  MontagePlayRate = 1.f;
  bMontagePaused = false;
  CallbackProxyRef = nullptr;
}

bool FGMC_MontageTracker::HasActiveMontage() const
{
  if (IsValid(Montage))
  {
    gmc_ck(Montage->GetPlayLength() > 0.f)
    gmc_ck(MontagePosition >= 0.f)
    gmc_ck(MontagePosition <= Montage->GetPlayLength())

    return true;
  }

  gmc_ck(!bStartedNewMontage)
  gmc_ck(!bMontageEnded)
  gmc_ck(!bInterruptedPreviousMontage)
  gmc_ck(!Montage)
  gmc_ck(MontagePosition == 0.f)
  //gmc_ck(MontagePlayRate == 1.f)
  gmc_ck(!bMontagePaused)

  return false;
}

bool FGMC_MontageTracker::HasActiveRootMotionMontage() const
{
  if (HasActiveMontage() && Montage->HasRootMotion())
  {
    return true;
  }

  return false;
}

bool FGMC_MontageTracker::HasActiveNonRootMotionMontage() const
{
  if (HasActiveMontage() && !Montage->HasRootMotion())
  {
    return true;
  }

  return false;
}
