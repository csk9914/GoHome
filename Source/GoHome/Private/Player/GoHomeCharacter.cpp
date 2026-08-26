


#include "Player/GoHomeCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

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
}

void AGoHomeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
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
}

void AGoHomeCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();
	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y);
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

void AGoHomeCharacter::AttachFlashlightToLeftHand(UStaticMeshComponent* FlashlightMeshComponent)
{
	if (!FlashlightMeshComponent) return;

	FlashlightMeshComponent->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		LeftHandSocketName);

	bIsHoldingFlashlight = true;
}

void AGoHomeCharacter::DetachFlashlightFromLeftHand()
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
