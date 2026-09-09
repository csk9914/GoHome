


#include "Item/SonarItemActor.h"
#include "Item/ItemActorBase.h"
#include "Engine/OverlapResult.h"
#include "Item/SonarPingMarkerActor.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

ASonarItemActor::ASonarItemActor()
{
	bAlwaysRelevant = true;
}

void ASonarItemActor::ServerUseSpecialAction()
{
	if (!HasAuthority() || !HoldingPawn || !CanUse()) return;

	const FVector Origin = GetActorLocation();

	AActor* Target = FindNearestDetectable(Origin);

	bPingFound = (Target != nullptr);
	PingLocation = Target ? Target->GetActorLocation() : FVector::ZeroVector;
	++PingEventId;

	if (Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Sonar] found=%s dist=%.0f"),
			*Target->GetName(), FVector::Dist(Origin, PingLocation));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Sonar] not found"));
	}

	// OnRep은 서버 자신에게 호출되지 않음. 리슨 서버 호스트도 여기서 직접 호출
	PlayPingCosmetic();

	// 탐지 실패해도 쿨다운은 적용
	LastUseTime = GetWorld()->GetTimeSeconds();
}

bool ASonarItemActor::CanUse() const
{
	const UWorld* World = GetWorld();
	if (!World) return false;

	return (World->GetTimeSeconds() - LastUseTime) >= UseCooldown;
}

void ASonarItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASonarItemActor, PingLocation);
	DOREPLIFETIME(ASonarItemActor, bPingFound);
	DOREPLIFETIME(ASonarItemActor, PingEventId);
}

AActor* ASonarItemActor::FindNearestDetectable(const FVector& Origin) const
{
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		FCollisionObjectQueryParams::AllDynamicObjects,
		FCollisionShape::MakeSphere(BaseDetectRadius),
		Params);

	AActor* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();

	const float MinDistSq = MinDetectRadius * MinDetectRadius;

	for (const FOverlapResult& Result : Overlaps)
	{
		AItemActorBase* Item = Cast<AItemActorBase>(Result.GetActor());
		if (!Item) continue;

		// 한 번이라도 주운 아이템은 이미 발견된 것 - 근접 힌트와 동일 규칙
		if (Item->HasBeenPickedUp()) continue;

		const float DistSq = FVector::DistSquared(Origin, Item->GetActorLocation());

		// 근접 힌트(NearbyHintRadius) 안쪽은 이미 알려주고 있으므로 제외
		if (DistSq < MinDistSq) continue;

		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Item;
		}
	}

	return Nearest;
}

bool ASonarItemActor::ShouldShowMarkerLocally() const
{
	if (MarkerShareRadius <= 0.f) return true;

	const APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
	if (!LocalPC) return false;

	const APawn* LocalPawn = LocalPC->GetPawn();
	if (!LocalPawn) return false;

	// 소나는 발사자 손에 들려 있으므로, 소나까지의 거리가 곧 발사자까지의 거리다.
	const float DistSq = FVector::DistSquared(LocalPawn->GetActorLocation(), GetActorLocation());

	return DistSq <= (MarkerShareRadius * MarkerShareRadius);
}

void ASonarItemActor::OnRep_PingEventId()
{
	PlayPingCosmetic();
}

void ASonarItemActor::PlayPingCosmetic()
{
	if (!bPingFound)
	{
		// 탐지 실패 - 나중에 실패 사운드 부착 자리
		return;
	}

	if (!MarkerClass) return;

	if (!ShouldShowMarkerLocally()) return;

	ASonarPingMarkerActor* Marker = GetWorld()->SpawnActor<ASonarPingMarkerActor>(MarkerClass, PingLocation, FRotator::ZeroRotator);

	if (Marker)
	{
		Marker->InitMarker(MarkerLifetime);
	}
}
