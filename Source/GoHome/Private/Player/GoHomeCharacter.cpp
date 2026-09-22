


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
#include "Interaction/ElectricSwitchboardActor.h"
#include "Net/UnrealNetwork.h"
#include "Materials/MaterialInstanceDynamic.h"
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
	// UHealthComponent::BeginPlay()가 Super::BeginPlay() 안에서 곧바로 초기 HP 브로드캐스트를 쏘기 때문에,
	// 구독을 그 전에 먼저 걸어야 첫 브로드캐스트를 놓치지 않는다(안 그러면 LastKnownHP가 -1로 남아
	// 첫 데미지를 초기화 호출로 오인해서 그때만 강제 운반 해제가 안 걸림).
	if (UHealthComponent* Health = FindComponentByClass<UHealthComponent>())
	{
		Health->OnHPChanged.AddDynamic(this, &AGoHomeCharacter::HandleHPChanged);
	}

	Super::BeginPlay();

	FirstPersonArmsMesh->SetLeaderPoseComponent(GetMesh());

	// 캐릭터 수영 모드 강제 진입
	GetCharacterMovement()->SetMovementMode(MOVE_Swimming);
	GetCharacterMovement()->Buoyancy = 1.0f;

	DefaultMaxSwimSpeed = GetCharacterMovement()->MaxSwimSpeed;
	CachedOxygenComponent = FindComponentByClass<UOxygenComponent>();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
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
		if (CurrentCarryObject)
		{
			// 서버가 평균 낸 합산 입력을 "내 로컬 폰"에 직접 적용 -> 표준 예측/ServerMove 흐름 그대로.
			AddMovementInput(CombinedCarryInput);
		}
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
		EIC->BindAction(MoveAction, ETriggerEvent::Started, this, &ThisClass::HandleFocusMoveStarted);
		EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &ThisClass::StartSprint);
		EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::StopSprint);
		EIC->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ThisClass::StopSprint);
		EIC->BindAction(PushToTalkAction, ETriggerEvent::Started, this, &ThisClass::StartTalking);
		EIC->BindAction(PushToTalkAction, ETriggerEvent::Completed, this, &ThisClass::StopTalking);
		
	}
}

void AGoHomeCharacter::Move(const FInputActionValue& Value)
{
	if (bIsStunned) return;
	if (FocusedSwitchboard) return;

	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FRotator FullRotation = GetControlRotation();

	const FVector ForwardDirection = FRotationMatrix(FullRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(FullRotation).GetUnitAxis(EAxis::Y);
	const FVector WorldIntent = ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X;
	
	if (CurrentCarryObject)
	{
		// 협동 운반 중이면 서버가 두 캐리어 입력을 평균 낼 수 있게 월드 스페이스 이동 의도를 알려줌.
		if (HasAuthority())
		{
			// 호스트 자기 자신이면 굳이 RPC 안 거치고 바로 반영.
			LastCarryInputWorld = WorldIntent;
		}
		else
		{
			Server_UpdateCarryInput(WorldIntent);
		}
		return;
	}

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);	
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
	if (bIsStunned) return;

	const float UpDownValue = Value.Get<float>();
	AddMovementInput(FVector::UpVector, UpDownValue);
}

void AGoHomeCharacter::StartSprint()
{
	if (FocusedSwitchboard)
	{
		// 배전반 포커스 중엔 Sprint 입력을 '취소'로 재사용 - 힌트 재생 중이거나 스턴 중이어도 탈출 가능해야 함.
		FocusedSwitchboard->ServerCancelFocus(this);
		return;
	}

	if (bIsStunned) return;

	// 협동 운반 중엔 스프린트 불가 -> 캐리어 간 속도 차이로 이탈되는 것 방지.
	if (CurrentCarryObject) return;

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
	// 협동 운반 중엔 서버도 스프린트 요청 무시.
	if (bNewSprinting && CurrentCarryObject) return;

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

void AGoHomeCharacter::Client_ForceStopSprint_Implementation()
{
	ApplySprintState(false);
}

void AGoHomeCharacter::Look(const FInputActionValue& Value)
{
	if (bIsStunned) { return; }

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

	// bIsHoldingItem = true; 는
	// UInventoryComponent(SetActiveSlot/RemoveItem)가 단일 소스로 관리함 -> 여기선 안건드림.
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
	// bIsHoldingItem은 UInventoryComponent(SetActiveSlot/RemoveItem)가 단일 소스로 관리함 - 여기선 안 건드림.
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
	DOREPLIFETIME(AGoHomeCharacter, CurrentCarryObject);
	DOREPLIFETIME_CONDITION(AGoHomeCharacter, CombinedCarryInput, COND_OwnerOnly);
	DOREPLIFETIME(AGoHomeCharacter, bIsStunned);
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
		// 놓을 때든 새로 잡을 때든 잔여 입력값 리셋 -> 이전 세션 값이 새 세션에 넘어가지 않게.
		LastCarryInputWorld = FVector::ZeroVector;
		CombinedCarryInput = FVector::ZeroVector;
	}

	else
	{
		// 운반 시작 지점에 스프린트 중이었을 수 있으니 강제로 끔.
		// 서버 + 원격 클라 둘다.
		ApplySprintState(false);
		if (!IsLocallyControlled())
		{
			Client_ForceStopSprint();
		}
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


void AGoHomeCharacter::SetCombinedCarryInput(const FVector& NewInput)
{
	CombinedCarryInput = NewInput; 
}


void AGoHomeCharacter::ApplyStun_Implementation(float Duration, FVector KnockbackImpulse, AActor* InInstigator)
{
	if (!HasAuthority()) return;

	bIsStunned = true;
	LaunchCharacter(KnockbackImpulse, true, true);

	if (IsLocallyControlled())
	{
		OnRep_IsStunned(); // 서버 자신은 RepNotify 안 뜸.
	}
	else
	{
		Client_ApplyStun(Duration, KnockbackImpulse);
	}

	GetWorldTimerManager().SetTimer(StunTimerHandle, this, &AGoHomeCharacter::EndStun, Duration, false);
}

void AGoHomeCharacter::Client_ApplyStun_Implementation(float Duration, FVector KnockbackImpulse)
{
	// 로컬 예측을 이 프레임에 바로 멈추기 위해 직접 반영.
	bIsStunned = true;
	LaunchCharacter(KnockbackImpulse, true, true);
}

void AGoHomeCharacter::EndStun()
{
	if (!HasAuthority()) return;

	bIsStunned = false;

	if (IsLocallyControlled())
	{
		OnRep_IsStunned();
	}
}

void AGoHomeCharacter::OnRep_IsStunned()
{
	// 필요한 경우 여기서 스턴 사운드/이펙트 등 클라 전용 후처리.
}

void AGoHomeCharacter::EnterSwitchboardFocus(AElectricSwitchboardActor* Switchboard)
{
	FocusedSwitchboard = Switchboard;
	HighlightedWireIndex = 0;
	EnteredPasswordDigits.Reset();

	UpdateWireHighlight(-1, HighlightedWireIndex);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetViewTargetWithBlend(Switchboard, 0.3f);
	}

	StartHintPlayback();
}

void AGoHomeCharacter::ExitSwitchboardFocus()
{
	GetWorld()->GetTimerManager().ClearTimer(HintPlaybackTimerHandle);
	bPlayingHint = false;
	HintPlaybackStep = -1;

	UpdateWireHighlight(HighlightedWireIndex, -1);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetViewTargetWithBlend(this, 0.3f);
	}

	FocusedSwitchboard = nullptr;
	HighlightedWireIndex = -1;

}

void AGoHomeCharacter::Client_EnterSwitchboardFocus_Implementation(AElectricSwitchboardActor* Switchboard)
{
	EnterSwitchboardFocus(Switchboard);
}

void AGoHomeCharacter::Client_ExitSwitchboardFocus_Implementation()
{
	ExitSwitchboardFocus();
}

void AGoHomeCharacter::HandleFocusMoveStarted(const FInputActionValue& Value)
{
	if (!FocusedSwitchboard || bPlayingHint) return;

	const FVector2D MovementVector = Value.Get<FVector2D>();

	int32 RowDelta = 0;
	int32 ColDelta = 0;

	if (MovementVector.X > 0.5f) ColDelta = 1;
	else if (MovementVector.X < -0.5f) ColDelta = -1;

	if (MovementVector.Y > 0.5f) RowDelta = -1; // W = 위.
	else if (MovementVector.Y < -0.5f) RowDelta = 1; // S = 아래.

	if (RowDelta != 0 || ColDelta != 0)
	{
		MoveHighlightedKey(RowDelta, ColDelta);
	}
}

static void IndexToRowCol(int32 Index, int32& OutRow, int32& OutCol)
{
	if (Index == 10) { OutRow = 0; OutCol = 0; return; } // Back
	if (Index == 11) { OutRow = 0; OutCol = 4; return; } // Enter
	if (Index >= 5) { OutRow = 1; OutCol = Index - 5; return; } // Wire5~9 = 윗줄
	OutRow = 2; OutCol = Index; // Wire0~4 = 아랫줄
}

static int32 RowColToIndex(int32 Row, int32 Col)
{
	if (Row <= 0)
	{
		return (Col <= 2) ? 10 : 11; // 왼쪽 절반 = Back, 오른쪽 절반 = Enter
	}

	const int32 ClampedCol = FMath::Clamp(Col, 0, 4);

	if (Row == 1)
	{
		return 5 + ClampedCol; // Wire5~9 (윗줄)
	}

	return ClampedCol; // Wire0~4 (아랫줄)
}

void AGoHomeCharacter::MoveHighlightedKey(int32 RowDelta, int32 ColDelta)
{
	if (!FocusedSwitchboard) return;

	int32 Row, Col;
	IndexToRowCol(HighlightedWireIndex, Row, Col);

	const int32 NewRow = FMath::Clamp(Row + RowDelta, 0, 2);

	int32 NewCol;
	if (NewRow == 0)
	{
		// Back/Enter 두 칸뿐 -> 좌우 입력 한번으로 바로 토글.
		if (ColDelta > 0) NewCol = 4; // Enter.
		else if (ColDelta < 0) NewCol = 0; // Back.
		else NewCol = (Col <= 2) ? 0 : 4; // 위/아래로 넘어온 경우 가까운 쪽 유지.
	}
	else
	{
		NewCol = FMath::Clamp(Col + ColDelta, 0, 4);
	}

	const int32 OldIndex = HighlightedWireIndex;
	HighlightedWireIndex = RowColToIndex(NewRow, NewCol);

	UpdateWireHighlight(OldIndex, HighlightedWireIndex);
}



void AGoHomeCharacter::PressHighlightedKey()
{
	if (!FocusedSwitchboard || bPlayingHint) return;

	const int32 DigitCount = FocusedSwitchboard->GetWireTargetCount(); // 10

	if (HighlightedWireIndex < DigitCount)
	{
		if (EnteredPasswordDigits.Num() < FocusedSwitchboard->GetPasswordLength())
		{
			EnteredPasswordDigits.Add(HighlightedWireIndex);
			FocusedSwitchboard->UpdatePasswordDisplay(EnteredPasswordDigits);
		}
	}
	else if (HighlightedWireIndex == DigitCount) // Back
	{
		if (EnteredPasswordDigits.Num() > 0)
		{
			EnteredPasswordDigits.Pop();
			FocusedSwitchboard->UpdatePasswordDisplay(EnteredPasswordDigits);
		}
	}
	else // Enter
	{
		FocusedSwitchboard->ServerSubmitPassword(EnteredPasswordDigits, this);
	}
}

void AGoHomeCharacter::UpdateWireHighlight(int32 OldIndex, int32 NewIndex)
{
	if (!FocusedSwitchboard) return;

	if (UPrimitiveComponent* OldWire = FocusedSwitchboard->GetKeypadElementMesh(OldIndex))
	{
		OldWire->SetRenderCustomDepth(false);
	}
	if (UPrimitiveComponent* NewWire = FocusedSwitchboard->GetKeypadElementMesh(NewIndex))
	{
		NewWire->SetRenderCustomDepth(true);
		NewWire->SetCustomDepthStencilValue(1);
	}
}

void AGoHomeCharacter::StartHintPlayback()
{
	if (!FocusedSwitchboard) return;

	bPlayingHint = true;
	HintPlaybackStep = 0;
	bHintShowingGap = true;

	SetAllWiresOff();
	GetWorld()->GetTimerManager().SetTimer(HintPlaybackTimerHandle, this, 
		                                   &AGoHomeCharacter::AdvanceHintPlayback, 
		                                   FocusedSwitchboard->GetHintGapDuration(), false);
}

void AGoHomeCharacter::AdvanceHintPlayback()
{
	if (!FocusedSwitchboard) return;

	if (bHintShowingGap)
	{
		// 갭 끝남 -> 이번 라운드 색 패턴 표시.
		bHintShowingGap = false;

		const TArray<FLinearColor>& Palette = FocusedSwitchboard->GetHintColorPalette();
		if (Palette.Num() < 2) return;

		const int32 MajorIdx = FMath::RandRange(0, Palette.Num() - 1);
		int32 MinorIdx;
		do { MinorIdx = FMath::RandRange(0, Palette.Num() - 1); } while (MinorIdx == MajorIdx);

		const int32 CorrectIndex = FocusedSwitchboard->GetCorrectWireIndexForStep(HintPlaybackStep);

		for (int32 i = 0; i < FocusedSwitchboard->GetWireTargetCount(); ++i)
		{
			const FLinearColor& Color = (i == CorrectIndex) ? Palette[MinorIdx] : Palette[MajorIdx];
			SetWireColor(FocusedSwitchboard->GetWireTarget(i), Color);
		}

		GetWorld()->GetTimerManager().SetTimer(HintPlaybackTimerHandle, this,
			&AGoHomeCharacter::AdvanceHintPlayback, FocusedSwitchboard->GetHintRoundDuration(), false);
	}
	else
	{
		// 이번 라운드 표시 끝 -> 다음 라운드로.
		++HintPlaybackStep;

		if (HintPlaybackStep >= FocusedSwitchboard->GetPasswordLength())
		{
			SetAllWiresOff();
			bPlayingHint = false;
			HintPlaybackStep = -1;
			return; // 힌트 재생 끝 - 이제 조작 가능.
		}

		bHintShowingGap = true;
		SetAllWiresOff();
		GetWorld()->GetTimerManager().SetTimer(HintPlaybackTimerHandle, this,
			&AGoHomeCharacter::AdvanceHintPlayback, FocusedSwitchboard->GetHintGapDuration(), false);
	}
}

void AGoHomeCharacter::SetWireColor(UMeshComponent* Wire, const FLinearColor& Color)
{
	if (!Wire) return;

	if (UMaterialInstanceDynamic* MID = Wire->CreateAndSetMaterialInstanceDynamic(0))
	{
		MID->SetVectorParameterValue(TEXT("Color"), Color);
	}
}

void AGoHomeCharacter::SetAllWiresOff()
{
	if (!FocusedSwitchboard) return;

	for (int32 i = 0; i < FocusedSwitchboard->GetWireTargetCount(); ++i)
	{
		SetWireColor(FocusedSwitchboard->GetWireTarget(i), FLinearColor::Black);
	}
}