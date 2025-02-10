// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCRollbackPlatform.h"
#include "GMCMovementUtilityComponent.h"
#include "GMCAssert.h"

AGMC_RollbackPlatform::AGMC_RollbackPlatform(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  LinearVelocity = {200., 0., 0.};
  AngularVelocity = {0., 0., 20.};
}

void AGMC_RollbackPlatform::UpdateState_Implementation(double Time, float DeltaTime, const FGMC_Move& Move, EGMC_NetContext Context)
{
  const auto& Root = GetRootComponent();
  if (!IsValid(Root) || Root->IsSimulatingPhysics() || DeltaTime < UGMC_ReplicationCmp::MIN_DELTA_TIME)
  {
    return;
  }

  gmc_ck(Time >= 0.)

  if (LinearMoveDuration > 0. && !BaseLinearVelocity.IsZero())
  {
    const auto GetLocationFromTime = [this](double Time, double CycleDuration)
    {
      const double CycleDurationRemainder = fmod(Time, CycleDuration);
      const bool bIsReturning = CycleDurationRemainder > LinearMoveDuration;
      const double MoveDurationRemainder = bIsReturning ? CycleDuration - CycleDurationRemainder : CycleDurationRemainder;
      return BaseTransform.GetLocation() + BaseLinearVelocity * MoveDurationRemainder;
    };

    const double CycleDuration = 2. * LinearMoveDuration;
    const auto& NewLocation = GetLocationFromTime(Time, CycleDuration);
    const auto& PreviousLocation = GetLocationFromTime(Time - DeltaTime, CycleDuration);
    const auto& NewLinearVelocity = (NewLocation - PreviousLocation) / DeltaTime;
    SetActorLocation(NewLocation);
    SetLinearVelocity(NewLinearVelocity);
  }

  if (!BaseAngularVelocity.IsZero())
  {
    const double DegreesRemainder = fmod(Time, 360.);
    const double PreviousDegreesRemainder = fmod(Time - DeltaTime, 360.);
    const auto& NewRotation = UGMC_MovementUtilityCmp::ToAngularVelocityQuat(DegreesRemainder * BaseAngularVelocity) * BaseTransform.GetRotation();
    const auto& PreviousRotation = UGMC_MovementUtilityCmp::ToAngularVelocityQuat(PreviousDegreesRemainder * BaseAngularVelocity) * BaseTransform.GetRotation();
    const auto& NewAngularVelocity = (NewRotation * PreviousRotation.Inverse()).Rotator() * (1. / DeltaTime);
    SetActorRotation(NewRotation);
    SetAngularVelocityInDegrees({-NewAngularVelocity.Roll, -NewAngularVelocity.Pitch, NewAngularVelocity.Yaw});
  }
}
