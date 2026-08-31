

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Player/SocketProvider.h"
#include "AI/NoiseType.h"
#include "GoHomeCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UCameraComponent;
class USkeletalMeshComponent;
class UOxygenComponent;

UCLASS()
class GOHOME_API AGoHomeCharacter : public ACharacter, public ISocketProvider
{
	GENERATED_BODY()

public:
	AGoHomeCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);
	void MoveUpDown(const FInputActionValue& Value);
	
	void StartSprint();
	void StopSprint();
	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewSprinting);
	void ApplySprintState(bool bNewSprinting);

	void Look(const FInputActionValue& Value);
	void StartTalking();
	void StopTalking();
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> FirstPersonArmsMesh;
	
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveUpDownAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> PushToTalkAction;
	
public:	
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float CurrentPitch = 0.f;
	
	// 핸들 소켓 선언.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FName RightHandSocketName = "Hand_R";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FName LeftHandSocketName = "Hand_L";

	// 아이템 오른손 소켓에 부착, 애니메이션 상태 전환
	// ItemActorBase에서 픽업 확정 시 호출
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void AttachItemToRightHand(UStaticMeshComponent* ItemMeshComponent);
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void AttachFlashlightToLeftHand(UStaticMeshComponent* FlashlightMeshComponent);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void DetachFlashlightFromLeftHand();

	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsHoldingFlashlight() const { return bIsHoldingFlashlight; }

	virtual FName GetRightHandSocketName() const override { return RightHandSocketName; }
	virtual FName GetLeftHandSocketName() const override { return LeftHandSocketName; }

	virtual void SetHoldingItem(bool bHolding) override;

	// 아이템을 오른손에서 떼고 애니메이션 상태를 원복
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void DetachItemFromRightHand();

	// AnimBP가 읽는 홀드 상태 (Layered Blend per Bone의 알파 보간용).
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsHoldingItem() const { return bIsHoldingItem; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(Server, Unreliable)
	void ServerUpdatePitch(float NewPitch);
	
	UFUNCTION()
	void OnRep_IsHoldingItem();

	UFUNCTION()
	void OnRep_IsHoldingFlashlight();
	
	UFUNCTION()
	void OnRep_ReplicatedPitch();
	
	UPROPERTY(ReplicatedUsing = OnRep_IsHoldingItem, BlueprintReadOnly, Category = "Interaction")
	bool bIsHoldingItem = false;
	
	UPROPERTY(ReplicatedUsing = OnRep_IsHoldingFlashlight, BlueprintReadOnly, Category = "Interaction")
	bool bIsHoldingFlashlight = false;
	
	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedPitch)
	float ReplicatedPitch = 0.f;
	
	// 수영, 소음 등급
	UPROPERTY(EditDefaultsOnly, Category = "Noise")
	ENoiseType SwimNoiseType = ENoiseType::Small;
	
	// 수영, 소음 반경
	UPROPERTY(EditDefaultsOnly, Category = "Noise")
	float SwimNoiseRadius = 800.f;
	
	// 소음 몇 초 간격으로 쏠 지
	UPROPERTY(EditDefaultsOnly, Category = "Noise")
	float SwimNoiseInterval = 1.5f;

	// 마지막 소음 발생 이후 누적 시간
	float TimeSinceLastSwimNoise = 0.f;

	// 스프린트 관련
	private:
		UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "1.0", UIMin = "1.0"))
		float SprintSpeedMultiplier = 1.5f;

		UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "1.0", UIMin = "1.0"))
		float SprintOxygenDrainMultiplier = 1.5f;

		bool bIsSprinting = false;
		float DefaultMaxSwimSpeed = 0.f;

		UPROPERTY()
		TObjectPtr<UOxygenComponent> CachedOxygenComponent;
};

