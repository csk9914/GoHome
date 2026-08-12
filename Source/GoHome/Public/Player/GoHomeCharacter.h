

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "GoHomeCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UCameraComponent;

UCLASS()
class GOHOME_API AGoHomeCharacter : public ACharacter
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
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

public:	
	
	// 핸들 소켓 선언.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FName RightHandSocketName = "Hand_R";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FName LeftHandSocketName = "Hand_L";

	// 아이템 오른손 소켓에 부착, 애니메이션 상태 전환
	// ItemActorBase에서 픽업 확정 시 호출
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void AttachItemToRightHand(UStaticMeshComponent* ItemMeshComponent);

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
};

