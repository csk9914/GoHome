#include "Interaction/BreakableWallActor.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Navigation/NavLinkProxy.h"
#include "GeometryCollection/GeometryCollectionActor.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AI/NoiseType.h"

ABreakableWallActor::ABreakableWallActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	IntactMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IntactMesh"));
	RootComponent = IntactMesh;
	IntactMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	IntactMesh->SetCollisionObjectType(ECC_WorldStatic);

	FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
	FrameMesh->SetupAttachment(IntactMesh);
	FrameMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FrameMesh->SetCanEverAffectNavigation(false);
	FrameMesh->SetVisibility(false);
}

void ABreakableWallActor::BeginPlay()
{
	Super::BeginPlay();

	// 통로 링크는 부서지기 전엔 비활성
	// 에디터에서 잘못 켜져있어도 방어됨.
	if (HasAuthority() && PassageNavLink)
	{
		PassageNavLink->SetSmartLinkEnabled(false);
	}

}


void ABreakableWallActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABreakableWallActor, bIsBroken);
}

void ABreakableWallActor::ServerBreak(AActor* InInstigator, const FVector& HitDirection)
{
	if (!HasAuthority() || bIsBroken) return;

	bIsBroken = true;
	ApplyBrokenState(true); // 서버 자신은 OnRep이 안 뜨므로 수동 호출.

	// 통로 열기 (AI 경로탐색은 서버에서만 돎 -> 서버 전용으로 충분).
	if (PassageNavLink)
	{
		PassageNavLink->SetSmartLinkEnabled(true);
	}

	// 큰 소음 -> 몬스터 유인.
	UGoHomeNoiseLibrary::GenerateNoise(this, 
		                               GetActorLocation(), 
		                               BreakNoiseRadius, 
		                               BreakNoiseType, 
		                               InInstigator ? InInstigator : this);
}

void ABreakableWallActor::OnRep_IsBroken()
{
	// 이미 begun play면(beginplay를 불러왔었으면) 런타임 파괴 -> 연출 재생.
	// 아니면 늦게 접속한 클라이언트의 상태 동기화 -> 스왑만.
	ApplyBrokenState(HasActorBegunPlay());
}

void ABreakableWallActor::ApplyBrokenState(bool bPlayEffects)
{
	// 온전 벽: 숨김 + 콜리전 끔 (내비 영향도 끄지만 정적 내비메시라 실제 경로는 step 2의 내비링크가 담당).
	IntactMesh->SetVisibility(false);
	IntactMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	IntactMesh->SetCanEverAffectNavigation(false);

	// 구멍 테두리 표시.
	FrameMesh->SetVisibility(true);

	if (!bPlayEffects) return;

	// 로컬 비복제 파편 액터(BP_Shatter_X : 컬렉션 지정 + 물리 On + Replicates Off + 폰 콜리전 무시).
	if (ShatterActorClass && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AGeometryCollectionActor* Shatter = 
			GetWorld()->SpawnActor<AGeometryCollectionActor>(ShatterActorClass, GetActorTransform(), Params))
		{
			if (UGeometryCollectionComponent* GCC = Shatter->GetGeometryCollectionComponent())
			{

				GCC->CrumbleActiveClusters(); // 모든 클러스터 연결 해제 -> 벽 전체가 조각남.
				GCC->AddRadialImpulse(GetActorLocation(), 500.f, ShatterImpulseStrength, RIF_Linear, true);
			}
			Shatter->SetLifeSpan(ShatterLifespan);
		}
	}

	if (BreakEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BreakEffect, GetActorLocation(), GetActorRotation());
	}

	if (BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BreakSound, GetActorLocation());
	}

}