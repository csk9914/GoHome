#include "Player/OxygenSafeZone.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Player/OxygenComponent.h"
#include "TimerManager.h"

AOxygenSafeZone::AOxygenSafeZone()
{
	PrimaryActorTick.bCanEverTick = false;

	// 네모난 투명 충돌 박스 컴포넌트를 만들고 밑에 자식으로
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ZoneVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneVolume"));
	ZoneVolume->SetupAttachment(SceneRoot);

	// 물리적으로 캐릭터를 튕겨내지 않고 오직 겹침(센서 감지)으로만 쓰겠다고 선언.
	ZoneVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// 바닥, 벽, 아이템 등 모든 것은 전부 무시.
	ZoneVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 하지만 오직 살아 움직이는 캐릭터(Pawn)가 들어왔을 때만 "감지(Overlap)"함.
	ZoneVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	// 충돌(겹침) 이벤트 신호를 발생시키도록 켬.
	ZoneVolume->SetGenerateOverlapEvents(true);

	// 박스 기본 크기
	ZoneVolume->SetBoxExtent(FVector(200.f, 150.f, 100.f));
}

void AOxygenSafeZone::BeginPlay()
{
	Super::BeginPlay();

	// [멀티플레이 대비] 서버(방장 컴퓨터)가 아닐 경우 아래 로직을 실행하지 않고 패스.
	if (!HasAuthority())
	{
		return;
	}

	// 누군가 박스에 발을 들이거나 나갈 때 실행될 함수를 연결(바인딩).
	ZoneVolume->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnZoneBeginOverlap);
	ZoneVolume->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnZoneEndOverlap);

	// 시작하자마자 박스 안에 서 있는 사람이 있는지 즉시 한 번 검사함.
	RefreshOverlappingActors();

	// 0.2초 뒤에 한 번 더 확실하게 재검사하는 타이머를 켬.
	if (InitialOverlapCheckDelay > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			InitialOverlapCheckTimerHandle,
			this,
			&ThisClass::RefreshOverlappingActors,
			InitialOverlapCheckDelay,
			false);
	}
}

// [액터 퇴장/삭제] 맵이 바뀌거나 안전지대가 사라질 때 뒷정리하는 함수
void AOxygenSafeZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 켜두었던 0.2초 타이머를 메모리에서 깔끔하게 취소.
	GetWorldTimerManager().ClearTimer(InitialOverlapCheckTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AOxygenSafeZone::OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 들어온 대상에게 "너 안전지대 안이야! (true)"라고 신호를 보냄.
	SetActorInSafeZone(OtherActor, true);
}

void AOxygenSafeZone::OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	SetActorInSafeZone(OtherActor, false);
}

// [겹친 캐릭터 싹 훑어보기] 시작 시 박스 안에 이미 들어와 있던 대상들을 찾는 함수
void AOxygenSafeZone::RefreshOverlappingActors()
{
	// 박스와 겹쳐 있는 모든 'Pawn(캐릭터)' 목록을 담을 바구니를 만듬.
	TArray<AActor*> OverlappingActors;
	ZoneVolume->GetOverlappingActors(OverlappingActors, APawn::StaticClass());

	// 바구니에 담긴 캐릭터들에게 차례대로 "너 안전지대 안이야! (true)"를 알려줌.
	for (AActor* Actor : OverlappingActors)
	{
		SetActorInSafeZone(Actor, true);
	}
}

// [산소 컴포넌트에 최종 신호 전달]
void AOxygenSafeZone::SetActorInSafeZone(AActor* Actor, bool bNewInSafeZone) const
{
	// 만약 대상이 비어있다면 에러가 나지 않도록 그냥 넘어감 (방어 코드).
	if (!Actor)
	{
		return;
	}

	// 그 대상에게 "산소 관리 주머니(UOxygenComponent)"가 달려있는지 확인함.
	UOxygenComponent* OxygenComponent = Actor->FindComponentByClass<UOxygenComponent>();
	if (!OxygenComponent)
	{
		return; // 산소 기능이 없는 돌멩이나 다른 물체라면 무시.
	}

	// 산소 주머니에게 상태를 전달 (true: 산소 스톱, false: 산소 닳기 시작)
	OxygenComponent->SetInSafeZone(bNewInSafeZone);
}