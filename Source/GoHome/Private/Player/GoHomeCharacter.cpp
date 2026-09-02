


#include "Player/GoHomeCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/HealthComponent.h"
#include "Player/DeathNotifier.h"
#include "Interaction/CoopCarryObjectBase.h"
#include "Camera/CameraComponent.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/OxygenComponent.h"

AGoHomeCharacter::AGoHomeCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetMesh(), "Spine_03"); // 카메라는 임시로 몸통(Spine_03)에 부착
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
	Camera->bUsePawnControlRotation = true;

	//GetMesh()->SetOwnerNoSee(true); // 소유자가 자신을 보지 못하도록 하는 코드(일단 주석처리)

	GetMesh()->SetCastShadow(false); // 그림자 끄기

	bUseControllerRotationYaw = true; // 몸통도 시선 Yaw를 따라가게 함

	AddTickPrerequisiteComponent(GetMesh());

	// FirstPersonArmsMesh 부분
	FirstPersonArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArmsMesh"));
	FirstPersonArmsMesh->SetupAttachment(GetCapsuleComponent()); // GetMesh()랑 같은 부모에 붙여서 트랜스폼 맞춤
	FirstPersonArmsMesh->SetRelativeLocation(GetMesh()->GetRelativeLocation());
	FirstPersonArmsMesh->SetRelativeRotation(GetMesh()->GetRelativeRotation());
	FirstPersonArmsMesh->SetOnlyOwnerSee(true);
	FirstPersonArmsMesh->SetCastShadow(false); // 본인 시야에 이상한 팔 그림자 안 생기게

	GetMesh()->SetOwnerNoSee(true); // 본인한테는 전신 메시 안 보이게

}

void AGoHomeCharacter::BeginPlay()
{
	Super::BeginPlay();

	FirstPersonArmsMesh->SetLeaderPoseComponent(GetMesh());

	// 캐릭터 수영 모드 강제 진입
	GetCharacterMovement()->SetMovementMode(MOVE_Swimming);
	GetCharacterMovement()->Buoyancy = 1.0f;

	DefaultMaxSwimSpeed = GetCharacterMovement()->MaxSwimSpeed;
	CachedOxygenComponent = FindComponentByClass<UOxygenComponent>();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem< UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
		
		if (PC->IsLocalController())
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->bShowMouseCursor = false;
		}
	}

	if (UHealthComponent* Health = FindComponentByClass<UHealthComponent>())
	{
		Health->OnHPChanged.AddDynamic(this, &AGoHomeCharacter::HandleHPChanged);
	}

	if (IDeathNotifier* DeathNotifier = FindComponentByInterface<IDeathNotifier>())
	{
		DeathNotifier->GetOnDeathDelegate().AddUObject(this, &AGoHomeCharacter::HandleForcedCarryRelease);
	}
}

void AGoHomeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// BP가 되돌린 직후, 스프린트 중이면 다시 덮어씌운다
	if (bIsSprinting)
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->MaxSwimSpeed = DefaultMaxSwimSpeed * SprintSpeedMultiplier;
		}
	}

	// 블랜더로 메시를 자체 수정함에 따라 해당 코드 불필요, 주석처리
	if (IsLocallyControlled())
	{
		//GetMesh()->HideBoneByName(TEXT("Head"), EPhysBodyOp::PBO_None);

		//const float Pitch = FRotator::NormalizeAxis(GetControlRotation().Pitch);
		//constexpr float BodyHidePitchThreshold = 30.f; // 이 각도 이상이면 몸 전체 숨김

		//if (FMath::Abs(Pitch) > BodyHidePitchThreshold)
		//{
		//	GetMesh()->HideBoneByName(TEXT("Pelvis"), EPhysBodyOp::PBO_None);
		//}
		//else
		//{
		//	GetMesh()->UnHideBoneByName(TEXT("Pelvis"));
		//}
	}

	if (IsLocallyControlled() || HasAuthority())
	{
		// 본인 클라이언트거나 서버가 그 캐릭터를 볼 때
		FRotator ControlRot = GetControlRotation();
		FRotator ActorRot = GetActorRotation();
		FRotator DeltaRot = (ControlRot - ActorRot).GetNormalized();

		CurrentPitch = DeltaRot.Pitch;

		if (HasAuthority())
		{
			// 호스트(서버+로컬조종) 자기 자신인 경우 -> 바로 리플리케이트 변수에 반영
			ReplicatedPitch = CurrentPitch;
		}
		else
		{
			// 순수 원격 클라이언트인 경우 -> 서버에 전송
			ServerUpdatePitch(CurrentPitch);
		}
	}

	if (HasAuthority())
	{
		const bool bIsSwimming = GetVelocity().Size() > 50.f;

		if (bIsSwimming)
		{
			TimeSinceLastSwimNoise += DeltaTime;
			if (TimeSinceLastSwimNoise >= SwimNoiseInterval)
			{
				UGoHomeNoiseLibrary::GenerateNoise(this, GetActorLocation(), SwimNoiseRadius, SwimNoiseType, this);
				TimeSinceLastSwimNoise = 0.f;
			}
		}
		else
		{
			TimeSinceLastSwimNoise = 0.f; // 멈추면 타이머 리셋 -> 멈췄다 바로 움직였을 때 즉시 안 쏘게
		}
	}
}

void AGoHomeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
		EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::StopCarryInput);
		EIC->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ThisClass::StopCarryInput);
		EIC->BindAction(MoveUpDownAction, ETriggerEvent::Triggered, this, &ThisClass::MoveUpDown);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
		EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &ThisClass::StartSprint);
		EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::StopSprint);
		EIC->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ThisClass::StopSprint);
		EIC->BindAction(PushToTalkAction, ETriggerEvent::Started, this, &ThisClass::StartTalking);
		EIC->BindAction(PushToTalkAction, ETriggerEvent::Completed, this, &ThisClass::StopTalking);
		
	}
}

void AGoHomeCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FRotator FullRotation = GetControlRotation();

	const FVector ForwardDirection = FRotationMatrix(FullRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(FullRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);

	if (CurrentCarryObject)
	{
		// 협동 운반 중이면 서버가 두 캐리어 입력을 평균 낼 수 있게 월드 스페이스 이동 의도를 알려줌.
		const FVector WorldIntent = ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X;
		if (HasAuthority())
		{
			// 호스트 자기 자신이면 굳이 RPC 안 거치고 바로 반영.
			LastCarryInputWorld = WorldIntent;
		}
		else
		{
			Server_UpdateCarryInput(WorldIntent);
		}
	}
}

void AGoHomeCharacter::StopCarryInput()
{
	if (!CurrentCarryObject) return;

	if (HasAuthority())
	{
		LastCarryInputWorld = FVector::ZeroVector;
	}
	else
	{
		Server_UpdateCarryInput(FVector::ZeroVector);
	}
}

void AGoHomeCharacter::MoveUpDown(const FInputActionValue& Value)
{
	const float UpDownValue = Value.Get<float>();
	AddMovementInput(FVector::UpVector, UpDownValue);
}

void AGoHomeCharacter::StartSprint()
{
	ApplySprintState(true);

	if (!HasAuthority())
	{
		ServerSetSprinting(true);
	}
}

void AGoHomeCharacter::StopSprint()
{
	ApplySprintState(false);

	if (!HasAuthority())
	{
		ServerSetSprinting(false);
	}
}

void AGoHomeCharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
	ApplySprintState(bNewSprinting);
}

void AGoHomeCharacter::ApplySprintState(bool bNewSprinting)
{
	if (bIsSprinting == bNewSprinting)
	{
		return;
	}

	bIsSprinting = bNewSprinting;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxSwimSpeed = bIsSprinting ? DefaultMaxSwimSpeed * SprintSpeedMultiplier : DefaultMaxSwimSpeed;
	}

	if (HasAuthority() && CachedOxygenComponent)
	{
		CachedOxygenComponent->SetSprintDrainMultiplier(bIsSprinting ? SprintOxygenDrainMultiplier : 1.f);
	}
}

void AGoHomeCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y);
}

void AGoHomeCharacter::StartTalking()
{
	if (APlayerController* PlayerController = GetController<APlayerController>())
	{
		PlayerController->ToggleSpeaking(true);
	}
}

void AGoHomeCharacter::StopTalking()
{
	if (APlayerController* PlayerController = GetController<APlayerController>())
	{
		PlayerController->ToggleSpeaking(false);
	}
}

void AGoHomeCharacter::AttachItemToRightHand(UStaticMeshComponent* ItemMeshComponent)
{
	if (!ItemMeshComponent) return;

	ItemMeshComponent->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		RightHandSocketName);

	bIsHoldingItem = true;
}

void AGoHomeCharacter::AttachFlashlightToChest(UStaticMeshComponent* FlashlightMeshComponent)
{
	if (!FlashlightMeshComponent) return;

	FlashlightMeshComponent->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		FlashlightSocketName);

	bIsHoldingFlashlight = true;
}

void AGoHomeCharacter::DetachFlashlightFromChest()
{
	bIsHoldingFlashlight = false;
}

void AGoHomeCharacter::OnRep_IsHoldingFlashlight()
{
	// 필요하면 여기서 사운드/이펙트 등 클라 전용 후처리
}


void AGoHomeCharacter::SetHoldingItem(bool bHolding)
{
	if (HasAuthority())
	{
		bIsHoldingItem = bHolding;
	}
}

void AGoHomeCharacter::DetachItemFromRightHand()
{
	bIsHoldingItem = false;
	// 실제 Detach(월드에 다시 떨어뜨리는 것)는 ItemActorBase 쪽에서
	// 자기 자신을 Detach + 위치 지정하는 게 자연스러움 (소유권 문제라).
}

void AGoHomeCharacter::OnRep_IsHoldingItem()
{
	// 필요하면 여기서 사운드/이펙트 등 클라 전용 후처리
}

void AGoHomeCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGoHomeCharacter, bIsHoldingItem);
	DOREPLIFETIME_CONDITION(AGoHomeCharacter, ReplicatedPitch, COND_SkipOwner);
	DOREPLIFETIME(AGoHomeCharacter, bIsHoldingFlashlight);
}

void AGoHomeCharacter::OnRep_ReplicatedPitch()
{
	CurrentPitch = ReplicatedPitch;
}

void AGoHomeCharacter::ServerUpdatePitch_Implementation(float NewPitch)
{
	ReplicatedPitch = NewPitch;
}

void AGoHomeCharacter::Server_UpdateCarryInput_Implementation(FVector WorldIntent)
{
	LastCarryInputWorld = WorldIntent;
}

void AGoHomeCharacter::SetCoopCarryObject(ACoopCarryObjectBase* NewCarryObject)
{
	CurrentCarryObject = NewCarryObject;
	if (!NewCarryObject)
	{
		// 놓는 순간 묵은 입력값도 같이 리셋 -> 다음에 다시 잡을 때 재생되는 것 방지.
		LastCarryInputWorld = FVector::ZeroVector;
	}
	OnRep_CurrentCarryObject(); // 서버 자신에게는 RepNotify가 안 뜨므로 직접 호출 -> 호스트 로컬도 즉시 반영.
}

void AGoHomeCharacter::OnRep_CurrentCarryObject()
{
	const bool bIsCarrying = (CurrentCarryObject != nullptr);

	// 운반 중엔 시야는 자유롭게, 몸통 Yaw는 고정(잡은 모습 유지). 운반 아니면 원래대로 시야를 따라감.
	bUseControllerRotationYaw = !bIsCarrying;

	// 1인칭 팔은 몸통(캡슐) 기준 Yaw를 따라가는데, 운반 중엔 몸통 Yaw가 고정되고 카메라만 돌아서
	// 팔이 시야랑 어긋나 이상하게 늘어져 보임 -> 운반 중엔 숨김(잡는 자세 애니메이션은 아직 없음).
	if (FirstPersonArmsMesh)
	{
		FirstPersonArmsMesh->SetVisibility(!bIsCarrying);
	}
}

void AGoHomeCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority() && CurrentCarryObject)
	{
		CurrentCarryObject->ReleaseCarriers();
	}

	Super::EndPlay(EndPlayReason);
}

void AGoHomeCharacter::HandleHPChanged(float CurrentHP, float MaxHP)
{
	if (LastKnownHP >= 0.f && CurrentHP < LastKnownHP)
	{
		HandleForcedCarryRelease();
	}
	LastKnownHP = CurrentHP;
}

void AGoHomeCharacter::HandleForcedCarryRelease()
{
	if (HasAuthority() && CurrentCarryObject)
	{
		CurrentCarryObject->ReleaseCarriers();
	}
}


