// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UnderwaterEnemyBase.generated.h"

class USceneComponent;

UCLASS()
class GOHOME_API AUnderwaterEnemyBase : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AUnderwaterEnemyBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> BodyCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> EyePoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float MaxHealth = 100.0f;
	UPROPERTY(BlueprintReadOnly,Category ="Enemy")
	float CurrentHealth = 100.0f;
	 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy")
	 AActor* TargetActor = nullptr;
	 UFUNCTION(BlueprintCallable, Category ="Enemy")
	 virtual void ReceiveDamage(float DamageAmount);

	 UFUNCTION(BlueprintCallable, Category ="Enemy")
	 virtual void Die();

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
