# GoHome 기술 아키텍처 (개발용)

> `Docs/Dev/` 문서군. 코드로 알 수 있는 것(클래스/필드 목록)은 담지 않는다 — 컴파일러가 강제하지 않는 경계 규칙·코드만 봐선 안 보이는 계약·결정 이유만 다룬다. 인일/복잡도는 GoHome 기술분석서(Notion), 정확한 시그니처는 `Source/GoHome/Public/` 헤더가 각각 유일한 출처.
>
> AI 내부 구현(상태 머신·조향 로직)은 팀원 재량이라 다루지 않는다 — 데미지 전달·소음 감지·도킹 문 상태 구독 등 경계 인터페이스만 고정한다.
>
> 기획 문서(01_GoHome_기획서, 02_GoHome_기술분석서)는 Notion으로 이관되어 이 저장소에는 없다 — 링크 검증 스크립트 대상 밖이므로 아래 본문 중 "→ 기술분석서 ...", "→ 기획서 ..." 형태의 텍스트 언급은 깨져도 스크립트가 못 잡는다. 절 번호·시스템 번호가 Notion 쪽에서 바뀌면 수동으로 훑어 확인할 것.

## 목차

- [모듈/폴더 구조](#모듈폴더-구조)
- [시스템별 결정과 경계](#시스템별-결정과-경계)
- [시스템 간 인터페이스 계약](#시스템-간-인터페이스-계약)
- [헤더 소유권](#헤더-소유권)
- [남은 의존성](#남은-의존성)
- [2인 협동 게이트 실제 배치](#2인-협동-게이트-실제-배치)
- [Replication 권한 원칙](#replication-권한-원칙)

## 모듈/폴더 구조

`Source/GoHome/`은 기술분석서(Notion) 기능별 분리(`Player`/`Interaction`/`AI`/`Item`/`UI`/`Save`)를 따르되 `Core/`를 추가 신설(기술분석서(Notion)엔 없음) — GameState 상태 머신·세션/로비 흐름은 특정 시스템이 아닌 게임 흐름 자체라서.

`Core/` 내부: 흐름 제어 클래스(GameMode/GameState/PlayerController/SessionSubsystem, 월드 미배치)는 바로 아래, 월드 배치 액터(`AZoneSelectMonitor`, `ADepartureButton`)는 `Core/Actors/`로 분리(`Item/AItemActorBase`처럼 배치형은 성격이 달라 구분).

폴더별 클래스/인터페이스 출처는 `Source/GoHome/<폴더>/`. 아래 표는 코드에 없는 정보만: 폴더 ↔ 기술분석서(Notion) 시스템 번호 대응, 성격이 섞인 폴더의 비고("정식 홈"이 아니라 "일단 여기" — 기준은 아래 "새 폴더를 파야 하는가"):

| 폴더 | 대응 시스템 (기술분석서(Notion) 번호) | 비고 |
|---|---|---|
| `Core/` | 1. 온라인 서브시스템 + 보이스 (세션/게임 흐름/도킹 문/보이스챗) | 보이스챗은 세션/흐름과 무관한 독립 서브시스템 — 얹혀 있을 뿐 |
| `Player/` | 2. 이동 + 사운드 + 환경 위해요소, 3. 산소 + HP | 환경 위해요소(수류/열수구)는 이동 시스템과 무관한 독립 시스템 — 얹혀 있을 뿐 |
| `Interaction/` | 4. 상호작용/운반/납품/협동 게이트 | 협동 게이트는 원래 `Core/` 배치가 검토됐다가 상호작용 트리거라는 이유로 여기 옮겨짐 |
| `AI/` | 5. AI 몬스터 | - |
| `Item/` | 4. 상호작용/운반/납품 (아이템 정의 + 물리/부력) | - |
| `UI/` | 6. UI (현재 Source 파일 없음 — Blueprint 전용) | - |
| `Save/` | 9. 세이브 데이터 스키마, 8. 장비 강화(미구현) | - |

`Public/`·`Private/` 최상위 배치 규칙은 [CODING_CONVENTIONS.md 폴더 규칙](CODING_CONVENTIONS.md#폴더-규칙) 참고.

### 새 폴더를 파야 하는가

기준: 기술적 의존만으론 기존 폴더에 끼워 넣지 않는다 — 전용 Component/Actor 세트를 갖는 독립 시스템이면 새 폴더("이 시스템만의 클래스 개수"로 판단). 실제 사례는 위 표 "비고" 열 참고. 소급 이동 없음.

## 시스템별 결정과 경계

폴더별 결합도 경계·결정 이유만(클래스/필드/구현 상태 출처는 위 "모듈/폴더 구조").

yaml 세 키: `decoupling`(안 바뀌는 경계) · `decisions`(반려된 대안 있을 때만 사유 남기고 단순 상태갱신은 덮어씀 — 상태 태그 없음, 코드가 출처) · `known_gaps`(코드만 봐선 놓치는 함정 — 해소되면 항목 삭제).

### Core
```yaml
decoupling:
  - 도킹 문 상태는 GameState가 아니라 UDockingDoorComponent가 별도 소유한다 — GameState는 탐사 진행 단계(로비/탐사/정산 등)만 책임진다. (구현 확인: `AGoHomeGameState`가 생성자에서 `UDockingDoorComponent`를 서브오브젝트로 소유하지만, 문 개폐 bool 자체와 `OnDoorStateChanged`는 컴포넌트가 갖는다 — "GameState 미참조"는 이 상태값에 대한 이야기이지 포인터를 안 갖는다는 뜻이 아니다.)
  - AI와 ASubmarine 둘 다 도킹 문 상태를 읽을 때 GameState 전체가 아니라 UDockingDoorComponent의 공개 상태(IsOpen(), OnDoorStateChanged)만 구독하고 GameState는 참조하지 않는다 — 게임 흐름 전체와 결합되지 않도록.

decisions:
  - name: 잠수정 오브젝트 구성
    detail: |
      단일 클래스 ASubmarine(허브/문 메시, InteriorVolume 트리거만 정의)을 로비/탐사 맵에 동일 배치, 하나의
      BP_Submarine으로 관리(ZoneSelectMonitor·ADepartureButton은 Child Actor Component). 기능 차이는
      서브클래스가 아니라 런타임 `GetWorld()->GetGameState<ALobbyGameState>()` 유효 여부로 판별한다 —
      서브클래스로 나누면 두 맵의 외형이 각 BP에 따로 들려 드리프트 위험이 생기므로 기각.
        - ZoneSelectMonitor: `ALobbyGameState` 유효할 때만 CanInteract() true
        - ADepartureButton: 로비면 Departure 동작, 아니면 Return 동작(`DockingDoorComponent->SetOpen(false)`
          → `DoorCloseDelay`(기본 8초, 문 닫히는 연출 대기) 타이머 만료 시 `ExecuteDelayedTravel` →
          `AGoHomeGameMode::ServerTravelViaLoadingScreen(GoHomeGameMode->GetLobbyMapPath())` 호출.
          `LobbyMapPath`는 `GoHomeGameMode`의 실재 필드(기본값 `/Game/GoHome/Maps/LV_Lobby`). 목적지는
          `UExpeditionTravelSubsystem::PendingDestinationMap`에 저장 후 `LoadingMapPath`로 먼저 경유.
          타이머 콜백은 `CreateUObject`(약한 참조) 사용 — 액터가 트래블 중 파괴돼도 안전)
  - name: 도킹 문 밖 생존자 처리
    detail: |
      문이 닫히는 순간(bOpen: true → false) ASubmarine이 InteriorVolume 밖에 있는 플레이어를 즉시 사망
      처리한다(리썰컴퍼니식 — 동료를 버리고 출발하는 연출). AUnderwaterEnemyBase도 APawn이라 같은 콜리전
      채널로 트리거에 걸리므로, 킬 로직에서 반드시 PlayerState/Controller 보유 여부로 플레이어만 걸러야
      한다(콜리전 채널만으로는 몬스터/플레이어 구분 불가).
  - name: 세션 생명주기
    detail: USessionSubsystem은 UGameInstanceSubsystem(트래블 간 유지). UI는 BP 델리게이트만 구독, GameState는 세션 트리거로 쓰지 않음.
  - name: 레벨 트래블 방식
    detail: "GoHomeGameMode.cpp: bUseSeamlessTravel = true. GameState(따라서 그 서브오브젝트인 DockingDoorComponent)는 트래블마다 새로 스폰되므로, Save뿐 아니라 도킹 문 상태를 구독하는 AI/ASubmarine도 매 레벨 BeginPlay에서 재구독해야 한다 — Save 절의 '재구독' 전제는 이 기준이며, 전환용 임시 GameState 구간의 재구독 타이밍은 미검증(관련 이슈: 클라 시멀리스 트래블 중 끊김, 원인 미상)."
  - name: 상태 전이 트리거 (EExpeditionState 7종, 기술분석서(Notion)에 없어 이 문서에서 확정)
    detail: |
      - Lobby → Departure: 출발 버튼 상호작용 즉시(전원 확정 없음). **주의**: `SetState(Departure)`는 `ServerTravelToMap` 내부에 무조건 있어, `ServerTravelViaLoadingScreen`(Return도 재사용)을 포함한 모든 서버 트래블에서 발동 — 이 전이 전용이 아니다.
      - `ZoneSelect`: 미사용 — `LobbyGameState::SelectedZoneId` 값 변경만으로 처리(상태 전이 아님)
      - Departure → Exploration: 트래블 완료 후 `ExplorationGameState` 생성자가 즉시 초기화, `BeginPlay`(서버)에서 `DockingDoorComponent->SetOpen(true)` 자동 호출(로딩 UI 붙으면 타이밍 재검증 필요)
      - Exploration → Return: 귀환 버튼 상호작용 시 `SetOpen(false)` → `ASubmarine`이 `InteriorVolume` 밖 플레이어 즉시 사망 처리 → `ServerTravelViaLoadingScreen`(로비 맵). **주의**: 이 경로도 `ServerTravelToMap`을 타므로, 곧 파괴될 `ExplorationGameState`가 `SetState(Departure)`로 세팅된 채 트래블에 들어간다 — 로딩 경유 중 그 상태를 구독하면 "Return"이 "Departure"로 보임
      - Return → Settlement: 미착수 — 귀환 트래블은 로비 맵 직행, 정산 화면 연결은 별도 작업
      - Exploration/Return → Failed: 제한 시간 만료, 도킹 문 위협 판정, 생존자 0명(→ Player 절 HP 0 처리) 중 하나

known_gaps:
  - name: 정산 값 누적 미연결
    detail: "`UInventoryComponent`(Interaction)가 납품 시 `GameState->AddDeliveredValue(int32)`를 실제로 호출하지만(`InventoryComponent.cpp`), `AGoHomeGameState::AddDeliveredValue` 자체는 빈 함수 — 호출은 되는데 값이 어디에도 누적되지 않는다."
```

### Player
```yaml
decoupling:
  - "실제 경계는 3단이다: `UInventoryComponent`(Interaction)가 `IWeightProvider`(Interaction)를 구현해 원시 무게 합을 노출하고, `UCarryWeightComponent`(Player, 신설 — 이 문서에 이전까지 누락)가 같은 Owner의 `IWeightProvider` 구현체를 찾아 합산한 뒤 최대무게/업그레이드를 반영해 `ICarryWeightProvider`(Player, `CarryWeightProvider.h`)로 재노출한다. `UOxygenComponent`는 `IWeightProvider`가 아니라 이 `ICarryWeightProvider`만 참조한다 — Interaction 원시 무게가 아니라 Player가 계산한 초과무게 결과만 본다."
  - "UHealthComponent는 \"누가 데미지를 주는지\" 몰라야 한다 → IDamageable(아래 인터페이스 계약 참고)로 노출해 호출자가 Character 타입을 몰라도 되게 한다."
  - "UHealthComponent는 IDeathNotifier도 구현한다 — Interaction(`UInventoryComponent`)이 사망 시점에 아이템을 드롭시켜야 하는데, HealthComponent 구체 타입 대신 이 인터페이스만 보고 구독한다(구현됨, `InventoryComponent::BeginPlay`에서 `FindComponentByInterface`로 구독)."

decisions:
  - name: 산소 0 → 데미지 경로
    detail: UOxygenComponent가 산소 0시 매 틱 IDamageable::ApplyDamage(질식 데미지, Owner, "Suffocation") 동기 호출.
  - name: 중복 사망 방지
    detail: OnDeath는 캐릭터당 탐사 1회만 브로드캐스트 — Core가 생존자 수를 추적하므로 이 보장이 깨지면 카운트가 틀어진다.

known_gaps:
  - name: HP 0 처리
    detail: "설계 의도: HealthComponent가 서버 전용 OnDeath 브로드캐스트, GameState가 탐사마다 재구독해 생존자 수 추적, 0명이면 Fail(EFailReason::AllPlayersDead). 현재 OnDeath 브로드캐스트는 구현됐지만 GameState 구독부와 Fail()은 빈 스텁 — 미연결(접속 종료 처리와 동일)."
  - name: 접속 종료 처리
    detail: "설계 의도: Logout이 GameState::OnPlayerRemovedFromParty(APlayerState*)를 호출해 OnDeath와 같은 경로로 합류(TSet으로 멱등 보장). 현재 Logout은 Super::Logout 한 줄, OnPlayerRemovedFromParty도 빈 함수 — 미연결이라 접속 종료가 생존자 수에 반영되지 않는다."
```

### Interaction
```yaml
decoupling:
  - 소켓 어태치는 Player의 스켈레탈 메시 소켓에 접근해야 하므로 두 폴더 사이의 결합 지점이다 — 헬퍼는 Interaction에 두되, Character는 "오른손/왼손 소켓 이름"만 공개 프로퍼티로 노출해 Interaction이 Character 내부 구조를 몰라도 되게 한다.

decisions:
  - name: 동시 픽업 레이스 컨디션
    detail: 동시 픽업 요청 시 한 명만 성공이 보장됨(구현 방식은 아이템 액터 내부).
  - name: 정산 값 전달
    detail: Interaction이 GameState->AddDeliveredValue(int32)를 직접 호출(델리게이트로 감싸지 않음). 수신 측 미연결 상태는 Core 절 known_gaps "정산 값 누적 미연결" 참고.
  - name: 상호작용 대상 탐지 방식
    detail: 로컬 컨트롤 폰에서만 탐지가 동작한다 — 다른 클라이언트의 리플리케이트된 폰까지 트레이스하면 안 되므로(코드만 봐선 안 드러나는 subtle한 제약). 구체 탐지 파라미터는 `UInteractionComponent` 소스가 유일한 출처.
```

### AI (경계만)
```yaml
note: 내부 구현 범위는 문서 상단 note 참고(AI 상태 머신·조향 로직은 팀원 재량).

boundary_interfaces:
  - 공격 시 대상의 IDamageable::ApplyDamage를 호출 — 몬스터는 대상이 Player인지 몰라도 된다.
  - 소음 감지는 발생원이 호출하는 GenerateNoise가 대신 처리 — AI는 발생원이 Player인지 Item인지 몰라도 된다.
  - 도킹 문 위협 판정은 UDockingDoorComponent의 공개 상태만 읽는다(현재 C++/BP 어느 쪽에도 이를 구독하는 코드가 없다 — 아래 known_gaps 참고).

decisions:
  - name: GenerateNoise 호출 방식 (기술분석서(Notion) 5절 근거)
    detail: GenerateNoise는 동기 함수로 반경 내 각 몬스터의 IMonsterNoiseListener::OnNoiseHeard를 호출까지만 책임진다. 전이 로직은 각 몬스터의 내부 구현(경계 밖)이 정한다. IMonsterNoiseListener는 C++ 베이스가 아니라 몬스터 BP(`BP_UnderwaterMonster`, `BP_WormBase`)가 직접 구현한다(Blueprint Implemented Interfaces — C++ 부모 상속과 무관, `.uasset`에 `MonsterNoiseListener`/`OnNoiseHeard` 참조로 확인됨. 현재 BP 쪽에서 구현 진행 중). `AMonsterBase`(C++, 이 인터페이스를 포함한 경계 3종을 다 구현)는 실제로는 아무 BP도 상속하지 않는 고아 클래스다 — 아래 known_gaps 참고.

known_gaps:
  - name: AMonsterBase 미사용 (고아 클래스)
    detail: "AI/MonsterBase.h에 IDamageable 호출·IMonsterNoiseListener 구현·도킹 문 상태 구독 3개 경계 인터페이스를 다 갖춘 C++ 베이스(AMonsterBase)가 설계되어 있으나, 실제 스폰되는 BP_UnderwaterMonster는 AUnderwaterEnemyBase(빈 APawn 껍데기, 인터페이스 없음)를, BP_WormBase는 AActor를 직접 상속해 AMonsterBase를 거치지 않는다. 소음 감지(IMonsterNoiseListener)는 BP가 이 베이스와 무관하게 직접 구현 중이라 영향 없지만, 데미지 처리·도킹 문 위협 구독은 AMonsterBase 경유 없이 각 BP가 별도로 구현해야 한다. Day-1 신규 참여자가 MonsterBase.h를 '지금 쓰는 베이스'로 오인해 그 위에 작업을 얹지 않도록 주의."
  - name: 도킹 문 위협 판정 미구현
    detail: "boundary_interfaces에 계약만 정의되어 있을 뿐, UDockingDoorComponent::OnDoorStateChanged를 구독해 위협 판정 후 GameState::Fail(EFailReason)을 호출하는 코드가 AI 쪽에 없다 — Core 절 known_gaps '정산 값 누적 미연결', Player 절 known_gaps 'HP 0 처리'와 같은 계열의 미연결 상태."
```

### Item
```yaml
decisions:
  - name: 타이머/충돌 감지 소유
    detail: 파손형의 충돌 속도 감지, 소음 유발형의 "30초 보유마다 +200" 누적 타이머는 아이템 액터 자신이 소유(인벤토리는 무게/개수만 앎).
```

### UI
```yaml
decoupling:
  - >
    표시 전용 위젯(게이지·인벤토리·카운트다운 등)은 리플리케이티드 프로퍼티/델리게이트를 구독만 한다.
    입력을 발생시키는 위젯(탐사 지역 선택 확정, 장비 강화 구매)은 서버 RPC(Server_ConfirmZoneSelection 등)만
    호출하고 로컬 상태를 직접 바꾸지 않는다.
```

### Save
```yaml
decisions:
  - name: 로드/생성
    detail: UGoHomeSaveSubsystem::Initialize()에서 디스크에 세이브 파일이 있으면 로드, 없으면(또는 캐스트 실패) 새 SaveGame 인스턴스를 생성.
  - name: 저장 트리거
    detail: FCoreUObjectDelegates::PostLoadMapWithWorld 구독으로 레벨 전환마다 새 AGoHomeGameState::OnStateChanged에 재구독하고, NewState == Lobby일 때 SaveToDisk() 호출. 재구독 직후 이미 Lobby 상태면 즉시 저장(델리게이트 엣지 트리거 누락 방지).
  - name: 저장 주체
    detail: OnPostLoadMap이 NM_Client면 즉시 리턴 — 호스트만 저장을 수행한다.

known_gaps:
  - name: 세이브 필드 갱신 미연결
    detail: "저장 파이프라인(로드/재구독/Lobby 진입 시 SaveToDisk) 자체는 동작하지만, `SharedCurrency`/`TotalRecoveredValue`/`QuotaMissCount` 같은 SaveGame 필드를 실제로 갱신하는 GameState 쪽 함수(`AddDeliveredValue`/`Fail`)가 빈 스텁이라 — 값이 갱신되지 않은 채로 저장/로드만 반복된다. Core 절 known_gaps '정산 값 누적 미연결' 참고."
```

## 시스템 간 인터페이스 계약

시스템 경계를 넘는 접점만 모은 표(시그니처 출처는 위 "모듈/폴더 구조" 참고).

| 접점 | 정의 위치 | 호출·구독 측 | 근거 |
|---|---|---|---|
| `IWeightProvider` | Interaction | `UCarryWeightComponent`(Player)가 원시 무게 합산에만 소비 — Oxygen은 이 인터페이스를 직접 보지 않음(아래 `ICarryWeightProvider` 참고) | 기술분석서(Notion) 2·3·4·8절 |
| `ICarryWeightProvider` | Player (`CarryWeightProvider.h`) | `UCarryWeightComponent`가 구현(최대무게/업그레이드 반영한 초과무게 계산), `UOxygenComponent`(산소 소모 계산)·환경 위해요소(수류 존)·8번 장비 강화(페널티 곡선 파라미터)가 소비 | — |
| `IInteractable` | Interaction | Item이 구현, 협동 게이트의 레버(`ACoopLeverActor`)가 구현, `UInteractionComponent`(Interaction)가 트레이스로 찾은 대상에 호출 | 기술분석서(Notion) 4절 |
| `IDamageable` | Player | Player(HealthComponent)가 구현, AI(몬스터 공격)와 Player 자신(`UOxygenComponent`의 질식 데미지)·환경 위해요소(열수구)가 소비 | — |
| `GenerateNoise` | AI | Item(구현됨, `ItemActorBase.cpp`)과 Player(구현됨, 수영 중 주기 소음 — `GoHomeCharacter.cpp`의 `SwimNoiseInterval`/`SwimNoiseRadius`, 보이스챗 발화 시)가 발생원. 호출 즉시 서버 동기 처리 | 기술분석서(Notion) 2·4·5절 |
| `IMonsterNoiseListener` | AI | `BP_UnderwaterMonster`/`BP_WormBase`가 각각 직접 구현(BP 쪽에서 구현 진행 중, C++ 몬스터 베이스 상속과 무관 — AI 절 decisions 참고) | `GenerateNoise`는 호출까지만 책임지고 전이 로직은 몬스터 구현체가 정한다(경계 밖) |
| GameState 상태 델리게이트 | Core | UI(바인딩), `UGoHomeSaveSubsystem`(진행 지점 저장), Interaction(정산 시점 갱신) | 기술분석서(Notion) 1·6·9절 |
| 도킹 문 상태 | Core | AI(위협 판정 구독 — 설계 의도, 현재 미연결. AI 절 known_gaps "도킹 문 위협 판정 미구현" 참고), `ASubmarine`(안전볼륨 밖 즉사 처리 구독, 구현됨), `UMultiActorGateComponent`(기반 재사용) | 기술분석서(Notion) 1·4절 |
| 인벤토리 슬롯 | Interaction | UI(슬롯별 바인딩) | 기술분석서(Notion) 4·6절 |
| `IDeathNotifier::OnDeath` | Player (`DeathNotifier.h`, `UHealthComponent`가 구현) | Interaction(`UInventoryComponent`가 `BeginPlay`에서 `FindComponentByInterface<IDeathNotifier>()`로 구독 → `ServerDropAllItems` 실제 구현됨), Core(설계 의도 — 현재 미연결, Player 절 known_gaps "HP 0 처리" 참고) | HP 0 처리 결정(UHealthComponent 소유). `UHealthComponent`가 `IDeathNotifier`를 구현해 Interaction/Core가 구체 타입을 몰라도 사망 시점을 구독하게 한다 |
| 정산/납품 값 흐름 | Interaction → Core → Save | Interaction(딜리버리 존 진입 시 호출), Save(스키마 반영, 갱신 자체는 미연결 — Save 절 known_gaps 참고) | 기술분석서(Notion) 4·9절 |

## 헤더 소유권

**소유권 규칙**: 각 헤더는 소유 폴더(대응은 위 "모듈/폴더 구조" 표) 담당자만 수정한다. 다른 폴더는 include해서 소비만 하고, 필드/시그니처 변경은 소유자 리뷰를 거친다. `AGoHomeGameState`처럼 여러 폴더가 쓰는 공유 클래스는 **필드 선언은 Core만**, 다른 폴더는 퍼블릭 서버 함수(`AddDeliveredValue` 등)로만 값을 바꾼다 — `GoHomeGameState.h` 동시 편집 충돌 방지용. 검증: `grep -rn "GameState->\w*\s*=" Source/GoHome/`가 Core 밖에서 나오면 위반.

## 남은 의존성

- `UI/`는 아직 C++ 베이스 클래스가 없다(Blueprint 전용).
- `Save/`의 장비 강화(8번) 구매 로직은 미구현 — 스키마 필드(`PurchasedUpgrades`)만 있다.
- Core의 `AddDeliveredValue`/`Fail`/`OnPlayerRemovedFromParty`가 빈 스텁이라, 세이브 필드 갱신·정산 누적·파산 판정이 실제로 연결되지 않았다(Core·Save 절 known_gaps 참고).
- 7번(레벨/그레이박스)은 `Source/GoHome/` 코드 폴더가 아니라 레벨 애셋 작업.

## 2인 협동 게이트 실제 배치

`UMultiActorGateComponent`/`ACoopLeverActor`/`AWeightPlateActor`: 도킹 문 컴포넌트 재사용 이유로 `Core/` 배치를 검토했으나 실제론 `Interaction/`에 구현(상호작용 트리거 계열 판단) — 계획과 코드가 갈린 사례(폴더 기준은 "새 폴더를 파야 하는가" 참고).

**동시 활성화 판정**: 트리거별 서버 전용 `bool` 배열 소유, 각 `OnInteract`가 `Server_SetTriggerActive(int32, bool)` 호출 시 같은 호출 안에서 전원 활성 여부를 동기 검사해 게이트를 연다 — 서버는 한 틱에 한 RPC씩 순차 처리하므로 "동시" 입력도 레이스 없음(Interaction 동시 픽업과 같은 논리).

## Replication 권한 원칙

- 서버 권위: 도킹 문 상태, 이동/위치, 산소/HP, 아이템 소유권, 소음 이벤트, 정산/재화, 목표 점수
- 클라이언트: 로컬 폰(autonomous proxy)은 `CharacterMovementComponent` 표준 클라이언트 예측 + 서버 보정, 다른 클라이언트의 폰(simulated proxy)은 순수 보간만 (AGoHomeCharacter는 이동 관련 오버라이드 없이 표준 ACharacter 권한 모델을 그대로 사용). 입력 요청만 (상호작용/구매 — 서버 승인 후 반영)
