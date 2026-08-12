


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
	}
}

void AGoHomeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		GetMesh()->HideBoneByName(TEXT("Head"), EPhysBodyOp::PBO_None);

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
}
