// Copyright 2022-2024 Dominik Lips. All Rights Reserved.

#include "GMCCapsuleComponent.h"
#include "GMCAssert.h"

UGMC_CapsuleCmp::UGMC_CapsuleCmp(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  ShapeColor = FColor(223, 149, 157, 255);
  CapsuleRotation = FRotator::ZeroRotator;
  CapsuleRadius = 22.f;
  CapsuleHalfHeight = 44.f;
  bUseEditorCompositing = true;
}

FBoxSphereBounds UGMC_CapsuleCmp::CalcBounds(const FTransform& LocalToWorld) const
{
  FTransform LocalToWorldCapsuleRot = LocalToWorld;
  const FQuat LocalToWorldRotation = LocalToWorldCapsuleRot.GetRotation();
  const FQuat RotationDiff = CapsuleRotation.Quaternion() * LocalToWorldRotation.Inverse();
  LocalToWorldCapsuleRot.SetRotation(LocalToWorldCapsuleRot.TransformRotation(RotationDiff * LocalToWorldRotation));

  FVector BoxPoint = FVector(CapsuleRadius, CapsuleRadius, CapsuleHalfHeight);
  return FBoxSphereBounds(FVector::ZeroVector, BoxPoint, CapsuleHalfHeight).TransformBy(LocalToWorldCapsuleRot);
}

void UGMC_CapsuleCmp::CalcBoundingCylinder(float& CylinderRadius, float& CylinderHalfHeight) const
{
  FTransform LocalToWorldCapsuleRot = GetComponentTransform();
  const FQuat LocalToWorldRotation = LocalToWorldCapsuleRot.GetRotation();
  const FQuat RotationDiff = CapsuleRotation.Quaternion() * LocalToWorldRotation.Inverse();
  LocalToWorldCapsuleRot.SetRotation(LocalToWorldCapsuleRot.TransformRotation(RotationDiff * LocalToWorldRotation));

  const FVector& ComponentScale = LocalToWorldCapsuleRot.GetScale3D();
  const float CapsuleEndCapCenter = FMath::Max(CapsuleHalfHeight - CapsuleRadius, 0.f);
  const FVector ZAxis = LocalToWorldCapsuleRot.TransformVectorNoScale(FVector(0.f, 0.f, CapsuleEndCapCenter * ComponentScale.Z));

  const float ScaledRadius = CapsuleRadius * (ComponentScale.X > ComponentScale.Y ? ComponentScale.X : ComponentScale.Y);

  CylinderRadius = ScaledRadius + FMath::Sqrt(FMath::Square(ZAxis.X) + FMath::Square(ZAxis.Y));
  CylinderHalfHeight = ScaledRadius + ZAxis.Z;
}

FRotator UGMC_CapsuleCmp::GetGMCCapsuleRotation() const
{
  return CapsuleRotation;
}

void UGMC_CapsuleCmp::SetGMCCapsuleRotation(FRotator NewRotation, bool bUpdateOverlaps)
{
  SetGMCCapsuleSizeAndRotation(NewRotation, GetScaledCapsuleRadius(), GetScaledCapsuleHalfHeight(), bUpdateOverlaps);
}

void UGMC_CapsuleCmp::SetGMCCapsuleSizeAndRotation(
  FRotator NewRotation,
  float NewRadius,
  float NewHalfHeight,
  bool bUpdateOverlaps
)
{
  CapsuleRotation = NewRotation;
  SetCapsuleSize(NewRadius, NewHalfHeight, bUpdateOverlaps);
}

void UGMC_CapsuleCmp::UpdateBodySetup()
{
  if (PrepareSharedBodySetup<UGMC_CapsuleCmp>())
  {
    bUseArchetypeBodySetup = InvalidateOrUpdateCapsuleBodySetup<EShapeBodySetupHelper::InvalidateSharingIfStale>(
      ShapeBodySetup,
      bUseArchetypeBodySetup,
      CapsuleRotation,
      CapsuleRadius,
      CapsuleHalfHeight
    );
  }
  CreateShapeBodySetupIfNeeded<FKSphylElem>();
  if (!bUseArchetypeBodySetup)
  {
    InvalidateOrUpdateCapsuleBodySetup<EShapeBodySetupHelper::UpdateBodySetup>(
      ShapeBodySetup,
      bUseArchetypeBodySetup,
      CapsuleRotation,
      CapsuleRadius,
      CapsuleHalfHeight
    );
  }
}

FPrimitiveSceneProxy* UGMC_CapsuleCmp::CreateSceneProxy()
{
  class FDrawCylinderSceneProxy final : public FPrimitiveSceneProxy
  {
  public:
    SIZE_T GetTypeHash() const override
    {
      static size_t UniquePointer;
      return reinterpret_cast<size_t>(&UniquePointer);
    }

    FDrawCylinderSceneProxy(const UGMC_CapsuleCmp* InComponent)
      : FPrimitiveSceneProxy(InComponent),
        bDrawOnlyIfSelected(InComponent->bDrawOnlyIfSelected),
        CapsuleRotation(InComponent->CapsuleRotation),
        CapsuleRadius(InComponent->GetUnscaledCapsuleRadius()),
        CapsuleHalfHeight(InComponent->GetUnscaledCapsuleHalfHeight()),
        ShapeColor(InComponent->ShapeColor)
    {
      bWillEverBeLit = false;
    }

    void GetDynamicMeshElements(
      const TArray<const FSceneView*>& Views,
      const FSceneViewFamily& ViewFamily,
      uint32 VisibilityMap,
      FMeshElementCollector& Collector
    ) const override
    {
      const FMatrix& LocalToWorld = GetLocalToWorld();
      const int32 CapsuleSides = FMath::Clamp<int32>(CapsuleRadius / 4.f, 16, 64);
      for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
      {
        if (VisibilityMap & (1 << ViewIndex))
        {
          const FSceneView* View = Views[ViewIndex];
          const FLinearColor DrawCapsuleColor = GetViewSelectionColor(
            ShapeColor,
            *View,
            IsSelected(),
            IsHovered(),
            false,
            IsIndividuallySelected()
          );
          FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
          DrawWireCapsule(
            PDI,
            LocalToWorld.GetOrigin(),
            LocalToWorld.TransformVector(FRotationMatrix(CapsuleRotation).GetScaledAxis(EAxis::X)),
            LocalToWorld.TransformVector(FRotationMatrix(CapsuleRotation).GetScaledAxis(EAxis::Y)),
            LocalToWorld.TransformVector(FRotationMatrix(CapsuleRotation).GetScaledAxis(EAxis::Z)),
            DrawCapsuleColor,
            CapsuleRadius,
            CapsuleHalfHeight,
            CapsuleSides,
            SDPG_World
          );
        }
      }
    }

    FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
    {
      const bool bProxyVisible = !bDrawOnlyIfSelected || IsSelected();
      const bool bShowForCollision = View->Family->EngineShowFlags.Collision && IsCollisionEnabled();
      FPrimitiveViewRelevance Result;
      Result.bDrawRelevance = (IsShown(View) && bProxyVisible) || bShowForCollision;
      Result.bDynamicRelevance = true;
      Result.bShadowRelevance = IsShadowCast(View);
      Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
      return Result;
    }

    uint32 GetMemoryFootprint(void) const override
    {
      return (sizeof(*this) + GetAllocatedSize());
    }

    uint32 GetAllocatedSize(void) const
    {
      return (FPrimitiveSceneProxy::GetAllocatedSize());
    }

  private:
    const uint32 bDrawOnlyIfSelected:1;
    const FRotator CapsuleRotation;
    const float CapsuleRadius;
    const float CapsuleHalfHeight;
    const FColor ShapeColor;
  };

  return new FDrawCylinderSceneProxy(this);
}

template<EShapeBodySetupHelper UpdateBodySetupAction, typename BodySetupType>
bool UGMC_CapsuleCmp::InvalidateOrUpdateCapsuleBodySetup(
  BodySetupType& ShapeBodySetup,
  bool bUseArchetypeBodySetup,
  const FRotator& Rotation,
  float Radius,
  float HalfHeight
)
{
  gmc_ck(
    (bUseArchetypeBodySetup && UpdateBodySetupAction == EShapeBodySetupHelper::InvalidateSharingIfStale) ||
    (!bUseArchetypeBodySetup && UpdateBodySetupAction == EShapeBodySetupHelper::UpdateBodySetup))
  gmc_ck(ShapeBodySetup->AggGeom.SphylElems.Num() == 1)
  FKSphylElem* SE = ShapeBodySetup->AggGeom.SphylElems.GetData();
  const float Length = 2 * FMath::Max(HalfHeight - Radius, 0.f);
  if (UpdateBodySetupAction == EShapeBodySetupHelper::UpdateBodySetup)
  {
    SE->SetTransform(FTransform(Rotation));
    SE->Radius = Radius;
    SE->Length = Length;
  }
  else
  {
    if (SE->Rotation != Rotation || SE->Radius != Radius || SE->Length != Length)
    {
      ShapeBodySetup = nullptr;
      bUseArchetypeBodySetup = false;
    }
  }
  return bUseArchetypeBodySetup;
}

template<typename ShapeElemType>
void UGMC_CapsuleCmp::CreateShapeBodySetupIfNeeded()
{
  if (!IsValid(ShapeBodySetup))
  {
    ShapeBodySetup = NewObject<UBodySetup>(this, NAME_None, RF_Transient);
    if (GUObjectArray.IsDisregardForGC(this))
    {
      ShapeBodySetup->AddToRoot();
    }
    ShapeBodySetup->AddToCluster(this);
    if (ShapeBodySetup->HasAnyInternalFlags(EInternalObjectFlags::Async) && GUObjectClusters.GetObjectCluster(ShapeBodySetup))
    {
      ShapeBodySetup->ClearInternalFlags(EInternalObjectFlags::Async);
    }
    ShapeBodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
    AddShapeToGeomArray<ShapeElemType>();
    ShapeBodySetup->bNeverNeedsCookedCollisionData = true;
    bUseArchetypeBodySetup = false;
    BodyInstance.BodySetup = ShapeBodySetup;
    {
      if (BodyInstance.IsValidBodyInstance())
      {
        FPhysicsCommand::ExecuteWrite(
          BodyInstance.GetActorReferenceWithWelding(),
          [this](const FPhysicsActorHandle& Actor) {
            TArray<FPhysicsShapeHandle> Shapes;
            BodyInstance.GetAllShapes_AssumesLocked(Shapes);
            for (FPhysicsShapeHandle& Shape : Shapes)
            {
              if (BodyInstance.IsShapeBoundToBody(Shape)) SetShapeToNewGeom<ShapeElemType>(Shape);
            }
          }
        );
      }
    }
  }
}

template<>
void UGMC_CapsuleCmp::AddShapeToGeomArray<FKSphylElem>()
{
  ShapeBodySetup->AggGeom.SphylElems.Add(FKSphylElem());
}

template<>
void UGMC_CapsuleCmp::SetShapeToNewGeom<FKSphylElem>(const FPhysicsShapeHandle& Shape)
{
  FPhysicsInterface::SetUserData(Shape, (void*)ShapeBodySetup->AggGeom.SphylElems[0].GetUserData());
}
