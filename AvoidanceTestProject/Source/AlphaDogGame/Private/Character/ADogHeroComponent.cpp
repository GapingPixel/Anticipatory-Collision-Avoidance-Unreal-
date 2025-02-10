// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ADogHeroComponent.h"

#include "Components/GameFrameworkComponentDelegates.h"
#include "Logging/MessageLog.h"
#include "Debug/ADogLogging.h"
#include "EnhancedInputSubsystems.h"
#include "Player/ADogPlayerController.h"
#include "Player/ADogPlayerState.h"
#include "Player/ADogLocalPlayer.h"
#include "Character/ADogPawnExtensionComponent.h"
#include "Character/ADogPawnData.h"
#include "Character/ADogCharacter.h"
#include "AbilitySystem/ADogAbilitySystemComponent.h"
#include "Input/ADogInputConfig.h"
#include "Input/ADogInputComponent.h"
#include "Camera/ADogCameraComponent.h"
#include "ADogGameplayTags.h"
#include "Components/GameFrameworkComponentManager.h"
#include "PlayerMappableInputConfig.h"
#include "Camera/ADogCameraMode.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "InputMappingContext.h"
#include "AbilitySystem/ADogAbilitySet.h"
#include "Character/ADogMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ADogHeroComponent)

#if WITH_EDITOR
#include "Misc/UObjectToken.h"
#endif	// WITH_EDITOR

namespace ADogHero
{
	static const float LookYawRate = 300.0f;
	static const float LookPitchRate = 165.0f;
};

const FName UADogHeroComponent::NAME_BindInputsNow("BindInputsNow");
const FName UADogHeroComponent::NAME_ActorFeatureName("Hero");

UADogHeroComponent::UADogHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilityCameraMode = nullptr;
	bReadyToBindInputs = false;
}

void UADogHeroComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetPawn<APawn>())
	{
		UE_LOG(LogADog, Error, TEXT("[UADogHeroComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."));

#if WITH_EDITOR
		if (GIsEditor)
		{
			static const FText Message = NSLOCTEXT("ADogHeroComponent", "NotOnPawnError", "has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint. This will cause a crash if you PIE!");
			static const FName HeroMessageLogName = TEXT("ADogHeroComponent");
			
			FMessageLog(HeroMessageLogName).Error()
				->AddToken(FUObjectToken::Create(this, FText::FromString(GetNameSafe(this))))
				->AddToken(FTextToken::Create(Message));
				
			FMessageLog(HeroMessageLogName).Open();
		}
#endif
	}
	else
	{
		// Register with the init state system early, this will only work if this is a game world
		RegisterInitStateFeature();
	}
}

bool UADogHeroComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	if (!CurrentState.IsValid() && DesiredState == ADogGameplayTags::InitState_Spawned)
	{
		// As long as we have a real pawn, let us transition
		if (Pawn)
		{
			return true;
		}
	}
	else if (CurrentState == ADogGameplayTags::InitState_Spawned && DesiredState == ADogGameplayTags::InitState_DataAvailable)
	{

		// If we're authority or autonomous, we need to wait for a controller with registered ownership of the player state.
		if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
		{
			AController* Controller = GetController<AController>();

			const bool bHasControllerPairedWithPS = (Controller != nullptr) && \
				(Controller->PlayerState != nullptr) && \
				(Controller->PlayerState->GetOwner() == Controller);

			if (!bHasControllerPairedWithPS)
			{
				return false;
			}
		}

		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
		const bool bIsBot = Pawn->IsBotControlled();

		if (bIsLocallyControlled && !bIsBot)
		{
			AADogPlayerController* ADogPC = GetController<AADogPlayerController>();

			// The input component and local player is required when locally controlled.
			if (!Pawn->InputComponent || !ADogPC || !ADogPC->GetLocalPlayer())
			{
				return false;
			}
		}

		// Make sure PawnExtensionComponent has PawnData
		bool pawnExtDataAvailable = Manager->HasFeatureReachedInitState(Pawn, UADogPawnExtensionComponent::NAME_ActorFeatureName, ADogGameplayTags::InitState_DataAvailable);
		if (!pawnExtDataAvailable)
		{
			return false;
		}
		
		return true;
	}
	else if (CurrentState == ADogGameplayTags::InitState_DataAvailable && DesiredState == ADogGameplayTags::InitState_DataInitialized)
	{
		// Wait for player state and extension component
		AADogPlayerState* ADogPS = GetPlayerState<AADogPlayerState>();

		return ADogPS && Manager->HasFeatureReachedInitState(Pawn, UADogPawnExtensionComponent::NAME_ActorFeatureName, ADogGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == ADogGameplayTags::InitState_DataInitialized && DesiredState == ADogGameplayTags::InitState_GameplayReady)
	{
		// TODO add ability initialization checks?
		return true;
	}

	return false;
}

void UADogHeroComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (CurrentState == ADogGameplayTags::InitState_DataAvailable && DesiredState == ADogGameplayTags::InitState_DataInitialized)
	{
		APawn* Pawn = GetPawn<APawn>();
		AADogPlayerState* ADogPS = GetPlayerState<AADogPlayerState>();
		if (!ensure(Pawn && ADogPS))
		{
			return;
		}

		AADogPlayerController* aDogPC = GetController<AADogPlayerController>();
		if (aDogPC)
		{
			if (Pawn->InputComponent != nullptr)
			{
				InitializePlayerInput(Pawn->InputComponent);
			}
		}

		if (const UADogPawnExtensionComponent* PawnExtComp = UADogPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			const UADogPawnData* pawnData = PawnExtComp->GetPawnData<UADogPawnData>();
			check(pawnData);

			if (UADogAbilitySystemComponent* ASC = UADogAbilitySystemComponent::FindAbilitySystemComponent(Pawn))
			{
				for (UADogAbilitySet* abilitySet : pawnData->AbilitySets)
				{
					abilitySet->GiveToAbilitySystem(ASC);
				}

				ASC->SetTagRelationshipMapping();
				
				UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(Pawn, AADogCharacter::NAME_ADogAbilityReady);
			}
			
			// Hook up the delegate for all pawns, in case we spectate later
			if (UADogCameraComponent* CameraComponent = UADogCameraComponent::FindCameraComponent(Pawn))
			{
				CameraComponent->DetermineCameraModeDelegate.BindUObject(this, &ThisClass::DetermineCameraMode);
			}

			if (aDogPC)
			{
				aDogPC->SetInitialSpawn();
			}
		}
	}
}

void UADogHeroComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == UADogPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == ADogGameplayTags::InitState_DataInitialized)
		{
			// If the extension component says all all other components are initialized, try to progress to next state
			CheckDefaultInitialization();
		}
	}
}

void UADogHeroComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = { ADogGameplayTags::InitState_Spawned, ADogGameplayTags::InitState_DataAvailable, ADogGameplayTags::InitState_DataInitialized, ADogGameplayTags::InitState_GameplayReady };

	// This will try to progress from spawned (which is only set in BeginPlay) through the data initialization stages until it gets to gameplay ready
	ContinueInitStateChain(StateChain);
}

void UADogHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for when the pawn extension component changes init state
	BindOnActorInitStateChanged(UADogPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// Notifies that we are done spawning, then try the rest of initialization
	ensure(TryToChangeInitState(ADogGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UADogHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void UADogHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const UADogLocalPlayer* LP = Cast<UADogLocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	Subsystem->ClearAllMappings();

	if (const UADogPawnExtensionComponent* PawnExtComp = UADogPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UADogPawnData* PawnData = PawnExtComp->GetPawnData<UADogPawnData>())
		{
			if (const UADogInputConfig* InputConfig = PawnData->InputConfig)
			{
				for (const FInputMappingContextAndPriority& Mapping : DefaultInputMappings)
				{
					check(Mapping.InputMapping.IsValid())
					if (UInputMappingContext* IMC = Mapping.InputMapping.Get())
					{
						if (Mapping.bRegisterWithSettings)
						{
							if (UEnhancedInputUserSettings* Settings = Subsystem->GetUserSettings())
							{
								Settings->RegisterInputMappingContext(IMC);
							}
							
							FModifyContextOptions Options = {};
							Options.bIgnoreAllPressedKeysUntilRelease = false;
							// Actually add the config to the local player							
							Subsystem->AddMappingContext(IMC, Mapping.Priority, Options);
						}
					}
				}

				// The ADog Input Component has some additional functions to map Gameplay Tags to an Input Action.
				// If you want this functionality but still want to change your input component class, make it a subclass
				// of the UADogInputComponent or modify this component accordingly.
				UADogInputComponent* ADogIC = Cast<UADogInputComponent>(PlayerInputComponent);
				if (ensureMsgf(ADogIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UADogInputComponent or a subclass of it.")))
				{
					// Add the key mappings that may have been set by the player
					ADogIC->AddInputMappings(InputConfig, Subsystem);

					// This is where we actually bind and input action to a gameplay tag, which means that Gameplay Ability Blueprints will
					// be triggered directly by these input actions Triggered events. 
					TArray<uint32> BindHandles;
					ADogIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

					ADogIC->BindNativeAction(InputConfig, ADogGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
					ADogIC->BindNativeAction(InputConfig, ADogGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse, /*bLogIfNotFound=*/ false);
					ADogIC->BindNativeAction(InputConfig, ADogGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered, this, &ThisClass::Input_LookStick, /*bLogIfNotFound=*/ false);
				}
			}
		}
	}

	if (ensure(!bReadyToBindInputs))
	{
		bReadyToBindInputs = true;
	}
 
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APlayerController*>(PC), NAME_BindInputsNow);
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(const_cast<APawn*>(Pawn), NAME_BindInputsNow);
}

void UADogHeroComponent::AddAdditionalInputConfig(const UADogInputConfig* InputConfig)
{
	TArray<uint32> BindHandles;

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}
	
	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	if (const UADogPawnExtensionComponent* PawnExtComp = UADogPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		UADogInputComponent* ADogIC = Pawn->FindComponentByClass<UADogInputComponent>();
		if (ensureMsgf(ADogIC, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UADogInputComponent or a subclass of it.")))
		{
			ADogIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);
		}
	}
}

void UADogHeroComponent::RemoveAdditionalInputConfig(const UADogInputConfig* InputConfig)
{
	//@TODO: Implement me!
}

bool UADogHeroComponent::IsReadyToBindInputs() const
{
	return bReadyToBindInputs;
}

void UADogHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag, const UInputAction* InputAction)
{
	const AADogCharacter* Pawn = GetPawn<AADogCharacter>();
	const UADogPawnExtensionComponent* PawnExtComp = UADogPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!IsValid(PawnExtComp))
	{
		return;
	}
	
	UADogAbilitySystemComponent* ASC = Pawn->GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		return;
	}

	if (ASC->CanQueueAbility(InputTag))
	{
		ASC->QueueAbility(InputTag, InputAction);
	}

}

void UADogHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag, const UInputAction* InputAction)
{
}

void UADogHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
    
    if (!Pawn)
    {
    	return;
    }
	
	const FVector Value = InputActionValue.Get<FVector>();

	if (Value.X != 0.0f || Value.Y != 0.0f)
	{
		// reverse x/y because add input expects X forward?
		Pawn->GetMovementComponent()->AddInputVector(FVector(Value.Y, Value.X, 0));
	}
	
}

void UADogHeroComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}
	
	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(-Value.Y);
	}
}

void UADogHeroComponent::Input_LookStick(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}
	
	const FVector2D Value = InputActionValue.Get<FVector2D>();

	const UWorld* World = GetWorld();
	check(World);
	
	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X * ADogHero::LookYawRate * World->GetDeltaSeconds());
	}

	if (Value.Y != 0.0f)
	{
		// disable pitch
		//Pawn->AddControllerPitchInput(-Value.Y * ADogHero::LookPitchRate * World->GetDeltaSeconds());
	}
}

TSubclassOf<UADogCameraMode> UADogHeroComponent::DetermineCameraMode() const
{
	if (AbilityCameraMode)
	{
		return AbilityCameraMode;
	}

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return nullptr;
	}

	if (UADogPawnExtensionComponent* PawnExtComp = UADogPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UADogPawnData* PawnData = PawnExtComp->GetPawnData<UADogPawnData>())
		{
			return PawnData->DefaultCameraMode;
		}
	}

	return nullptr;
}

void UADogHeroComponent::SetAbilityCameraMode(TSubclassOf<UADogCameraMode> CameraMode, const int OwningID)
{
	if (CameraMode)
	{
		AbilityCameraMode = CameraMode;
		AbilityCameraModeOwningID = OwningID;
	}
}

void UADogHeroComponent::ClearAbilityCameraMode(const int OwningID)
{
	if (AbilityCameraModeOwningID == OwningID)
	{
		AbilityCameraMode = nullptr;
		AbilityCameraModeOwningID = -1;
	}
}

