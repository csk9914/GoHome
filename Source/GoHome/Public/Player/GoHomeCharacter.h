

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Player/SocketProvider.h"
#include "GoHomeCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UCameraComponent;

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

	virtual FName GetRightHandSocketName() const override { return RightHandSocketName; }
	virtual FName GetLeftHandSocketName() const override { return LeftHandSocketName; }



};

