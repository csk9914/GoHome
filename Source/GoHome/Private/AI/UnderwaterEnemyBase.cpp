// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/UnderwaterEnemyBase.h"

// Sets default values
AUnderwaterEnemyBase::AUnderwaterEnemyBase()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

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


