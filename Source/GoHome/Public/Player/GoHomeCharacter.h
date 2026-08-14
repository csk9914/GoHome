

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Player/SocketProvider.h"
#include "GoHomeCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UCameraComponent;
class USkeletalMeshComponent;

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
	void Look(const FInputActionValue& Value);

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
	TObjectPtr<UInputAction> LookAction;

	

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
	UPROPERTY(ReplicatedUsing = OnRep_IsHoldingItem, BlueprintReadOnly, Category = "Interaction")
	bool bIsHoldingItem = false;

	UFUNCTION()
	void OnRep_IsHoldingItem();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedPitch)
	float ReplicatedPitch = 0.f;

	UFUNCTION()
	void OnRep_ReplicatedPitch();

	UFUNCTION(Server, Unreliable)
	void ServerUpdatePitch(float NewPitch);
};

