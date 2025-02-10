// Copyright 2022-2024 Dominik Lips. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/CapsuleComponent.h"
#include "GMCCapsuleComponent.generated.h"

/// General capsule collision that can assume arbitrary rotations. Not directly supported by UGMC_MovementUtilityCmp, use either the super-class default capsule
/// or the horizontally oriented sub-class capsule.
UCLASS(NotBlueprintable, meta = (DisplayName = "General Capsule Collision"))
class GMCCORE_API UGMC_CapsuleCmp : public UCapsuleComponent
{
  GENERATED_BODY()

public:

  UGMC_CapsuleCmp(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

  FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
  void CalcBoundingCylinder(float& CylinderRadius, float& CylinderHalfHeight) const override;

  /// Returns the capsule rotation.
  ///
  /// @returns      FRotator    The current capsule rotation.
  UFUNCTION(BlueprintCallable, Category = "Components|Capsule", DisplayName = "Get GMC Capsule Rotation")
  FRotator GetGMCCapsuleRotation() const;

  /// Set the capsule rotation. Arbitrary rotations are not supported by UGMC_MovementUtilityCmp.
  ///
  /// @param        NewRotation        The new rotation of the capsule.
  /// @param        bUpdateOverlaps    Whether the touching array for the owner actor should be updated (only if this shape is registered and collides).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "Components|Capsule", DisplayName = "Set GMC Capsule Rotation")
  void SetGMCCapsuleRotation(FRotator NewRotation, bool bUpdateOverlaps = true);

  /// Set the size and rotation of the capsule. Arbitrary rotations are not supported by UGMC_MovementUtilityCmp.
  ///
  /// @param        NewRotation        The new rotation of the capsule.
  /// @param        NewRadius          The new radius of the center cylinder.
  /// @param        NewHalfHeight      The new half-height (including the hemispheres).
  /// @param        bUpdateOverlaps    Whether the touching array for the owner actor should be updated (only if this shape is registered and collides).
  /// @returns      void
  UFUNCTION(BlueprintCallable, Category = "Components|Capsule", DisplayName = "Set GMC Capsule Size And Rotation")
  void SetGMCCapsuleSizeAndRotation(FRotator NewRotation, float NewRadius, float NewHalfHeight, bool bUpdateOverlaps = true);

  ///~ Begin UPrimitiveComponent Interface.
  FPrimitiveSceneProxy* CreateSceneProxy() override;
  ///~ End UPrimitiveComponent Interface.

  ///~ Begin UShapeComponent Interface
  void UpdateBodySetup() override;
  ///~ End UShapeComponent Interface

protected:

  /// The rotation of the capsule component.
  FRotator CapsuleRotation{0.};

  /// Explicit template instantiations are not accessible from the engine binaries so we must copy the template functions that we need (see UShapeComponent).
  template<EShapeBodySetupHelper, typename BodySetupType> bool InvalidateOrUpdateCapsuleBodySetup(BodySetupType&, bool, const FRotator&, float, float);
  template<typename ShapeElemType> void CreateShapeBodySetupIfNeeded();
  template<typename ShapeElemType> void AddShapeToGeomArray();
  template<typename ShapeElemType> void SetShapeToNewGeom(const FPhysicsShapeHandle&);
};
