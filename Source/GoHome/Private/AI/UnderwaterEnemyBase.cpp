// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/UnderwaterEnemyBase.h"

// Sets default values
AUnderwaterEnemyBase::AUnderwaterEnemyBase()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	BodyCollision = CreateDefaultSubobject<USphereComponent>(TEXT("BodyCollision"));
	SetRootComponent(BodyCollision);

	BodyCollision->SetSphereRadius(120.0f);
	BodyCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BodyCollision->SetCollisionObjectType(ECC_Pawn);
	BodyCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BodyCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	BodyCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	BodyCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(BodyCollision);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EyePoint = CreateDefaultSubobject<USceneComponent>(TEXT("EyePoint"));
	EyePoint->SetupAttachment(BodyCollision);

}

// Called when the game starts or when spawned
void AUnderwaterEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	
}

// Called every frame
void AUnderwaterEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AUnderwaterEnemyBase::ReceiveDamage(float DamageAmount)
{
	if (!HasAuthority())
	{
		return;
	}
	CurrentHealth -= DamageAmount;
	if (CurrentHealth <= 0.0f)
	{
		CurrentHealth = 0.0f;
		Die();
	}
}

void AUnderwaterEnemyBase::Die()
{
	if (!HasAuthority())
	{
		return;
	}
	Destroy();
}

// Called to bind functionality to input
void AUnderwaterEnemyBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

