


#include "Player/GoHomeCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"

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

}

void AGoHomeCharacter::BeginPlay()
{
	Super::BeginPlay();

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

		const float Pitch = FRotator::NormalizeAxis(GetControlRotation().Pitch);
		constexpr float BodyHidePitchThreshold = 30.f; // 이 각도 이상이면 몸 전체 숨김

		if (FMath::Abs(Pitch) > BodyHidePitchThreshold)
		{
			GetMesh()->HideBoneByName(TEXT("Pelvis"), EPhysBodyOp::PBO_None);
		}
		else
		{
			GetMesh()->UnHideBoneByName(TEXT("Pelvis"));
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
