


#include "Player/UnderwaterMine.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Player/Damageable.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"

AUnderwaterMine::AUnderwaterMine()
{
    bReplicates = true;

    TriggerArea = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerArea"));
    SetRootComponent(TriggerArea);
    TriggerArea->InitSphereRadius(500.f);
    TriggerArea->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    TriggerArea->SetGenerateOverlapEvents(true);

    MineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MineMesh"));
    MineMesh->SetupAttachment(TriggerArea);
    MineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AUnderwaterMine::BeginPlay()
{
    Super::BeginPlay();
    TriggerArea->OnComponentBeginOverlap.AddDynamic(this, &AUnderwaterMine::OnTriggerBeginOverlap);
}

void AUnderwaterMine::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AUnderwaterMine, State);
}

void AUnderwaterMine::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority() || bHasTriggered)
    {
        return;
    }

    if (!Cast<ACharacter>(OtherActor))
    {
        return;
    }

    bHasTriggered = true;
    ArmMine();
}

void AUnderwaterMine::ArmMine()
{
    State = EMineState::Warning;
    HandleStateChanged();

    GetWorldTimerManager().SetTimer(DetonationTimerHandle, this, &AUnderwaterMine::Detonate, WarningDuration, false);
}

void AUnderwaterMine::Detonate()
{
    TArray<AActor*> IgnoreActors;
    TArray<AActor*> OverlappingActors;
    UKismetSystemLibrary::SphereOverlapActors(this, GetActorLocation(), ExplosionRadius,
        TArray<TEnumAsByte<EObjectTypeQuery>>{ UEngineTypes::ConvertToObjectType(ECC_Pawn) },
        ACharacter::StaticClass(), IgnoreActors, OverlappingActors);

    for (AActor* Actor : OverlappingActors)
    {
        if (UActorComponent* DamageableComponent = Actor->FindComponentByInterface(UDamageable::StaticClass()))
        {
            IDamageable::Execute_ApplyDamage(DamageableComponent, ExplosionDamage, this, DamageTypeName);
        }
    }

    State = EMineState::Detonated;
    HandleStateChanged();

    SetLifeSpan(5.f);
}

void AUnderwaterMine::OnRep_State()
{
    HandleStateChanged();
}

void AUnderwaterMine::HandleStateChanged()
{
    switch (State)
    {
    case EMineState::Warning:
        OnMineArmed();
        break;
    case EMineState::Detonated:
        TriggerArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        MineMesh->SetVisibility(false);

        if (ExplosionVFXAsset)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionVFXAsset, GetActorLocation());
        }

        OnMineDetonated();
        break;
    default:
        break;
    }
}