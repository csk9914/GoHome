

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GoHomeCharacter.generated.h"

UCLASS()
class GOHOME_API AGoHomeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGoHomeCharacter();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
