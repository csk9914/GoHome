# GoHome 기술 아키텍처 (개발용)

> `Docs/Dev/` 문서군. 링크 검증은 아래 "Docs/Design 참조 점검 체크리스트" 절 참고.
>
> 코드로 알 수 있는 것(클래스/필드 목록)은 담지 않는다 — 컴파일러가 강제하지 않는 경계 규칙, 코드만 봐선 안 보이는 계약, 그 결정 이유만 다룬다. 인일/복잡도/담당자 수치는 [02문서](../Design/02_GoHome_기술분석서.md), 경계 인터페이스의 이유·호출/구독 관계는 이 문서(아래 "시스템별 결정과 경계", "시스템 간 인터페이스 계약"), 정확한 시그니처는 `Source/GoHome/Public/` 헤더가 각각 유일한 출처(헤더가 최신일 수 있어 시그니처는 옮겨 적지 않는다).
>
> AI 내부 구현(상태 머신·조향 로직)은 팀원 재량이라 다루지 않는다 — 데미지 전달·소음 감지·도킹 문 상태 구독 등 경계 인터페이스만 고정한다. 이 경계만 지키면 내부 구현이 바뀌어도 다른 시스템은 영향받지 않는다.

## 목차

- [Docs/Design 참조 점검 체크리스트](#docsdesign-참조-점검-체크리스트)
- [모듈/폴더 구조](#모듈폴더-구조)
- [시스템별 결정과 경계](#시스템별-결정과-경계)
- [시스템 간 인터페이스 계약](#시스템-간-인터페이스-계약)
- [헤더 소유권](#헤더-소유권)
- [남은 의존성](#남은-의존성)
- [2차 프로토타입 추가 시스템 배치](#2차-프로토타입-추가-시스템-배치)
- [Replication 권한 원칙](#replication-권한-원칙)

## Docs/Design 참조 점검 체크리스트

스크립트 동작 방식은 저장소 루트 [CLAUDE.md 검증 스크립트](../../CLAUDE.md#validation-scripts) 참고. 스크립트가 못 잡는 부분(본문 텍스트로만 언급된 절 번호)이 있으니, `Docs/Design/01`·`02`문서의 절 번호나 헤더 제목을 바꿀 때마다 추가로 확인한다:

1. `grep -rn "Design/0" Docs/Dev/`로 이 문서군에서 Design 문서를 참조하는 줄을 찾아, 절 번호가 **본문 텍스트로만**(링크 아닌 형태로) 언급된 곳이 있는지 확인한다.
2. 헤더 번호(예: "10. 착수 순서", "13-1. ...")가 바뀌었다면 "남은 의존성"·"2차 프로토타입 추가 시스템 배치" 절의 같은 번호 인용도 함께 갱신한다.

## 모듈/폴더 구조

`Source/GoHome/`은 02문서의 기능별 분리(`Player`/`Interaction`/`AI`/`Item`/`UI`/`Save`)를 따르되, 02문서에 없는 `Core/`를 신설한다 — GameState 상태 머신·세션/로비 흐름은 특정 시스템이 아닌 게임 흐름 자체라 별도 폴더가 필요했다(이 문서가 유일한 출처).

`Core/` 내부는 두 성격으로 나뉜다: 흐름 제어 클래스(GameMode/GameState/PlayerController/SessionSubsystem류, 월드 미배치)는 `Core/` 바로 아래, 월드에 실제 배치되는 액터(`AZoneSelectMonitor`, `ADepartureButton` 등 로비 상호작용 오브젝트)는 `Core/Actors/`로 분리한다 — `Item/`의 `AItemActorBase`처럼 배치형 액터는 성격이 달라 같은 상위 폴더 안에서도 구분해두는 편이 탐색에 낫다는 판단.

폴더별 클래스/인터페이스는 `Source/GoHome/<폴더>/`가 유일한 출처(`grep -rl`로 구현체 검색). 코드에 없는 정보인 폴더 ↔ 02문서 시스템 번호 대응만 아래에 남긴다:

| 폴더 | 대응 시스템 (02문서 번호) |
|---|---|
| `Core/` | 1. 온라인 서브시스템 + 보이스 (게임 흐름 부분) |
| `Player/` | 2. 이동 + 사운드, 3. 산소 + HP |
| `Interaction/` | 4. 상호작용/운반/납품 |
| `AI/` | 5. AI 몬스터 |
| `Item/` | 4. 상호작용/운반/납품 (아이템 정의) |
| `UI/` | 6. UI |
| `Save/` | 9. 세이브 데이터 스키마, 8. 장비 강화 |

`Public/`·`Private/` 최상위 배치 규칙은 [CODING_CONVENTIONS.md 폴더 규칙](CODING_CONVENTIONS.md#폴더-규칙) 참고.

## 시스템별 결정과 경계

폴더별로 코드가 강제하지 않는 결합도 경계와 결정 이유만 다룬다(클래스/필드/구현 상태의 출처는 위 "모듈/폴더 구조" 참고).

아래 yaml 블록은 성격이 다른 세 키로 나뉜다:
- `decoupling`: 안 바뀌는 결합도 경계 규칙(누가 누구를 몰라야 하는지). 진행 상태를 언급하지 않는다.
- `decisions`: 구체적 설계 선택. 현재까지 최선의 판단이지 최종 확정이 아니다 — 더 나은 설계가 보이면 갱신한다. 다른 접근을 검토했다가 반려한 경우만 "이전 결정은 X, 이유는 Y로 기각/변경"처럼 사유를 남기고(예시: 아래 Core 절 "상태 전이 트리거"의 Exploration → Return 항목), 단순 진행상태 갱신(미착수→착수, 설계만→구현됨)은 사유 없이 그냥 덮어쓴다. "구현 전"/"미구현"/"착수 전" 같은 단순 상태 태그는 애초에 두지 않는다 — 진행 상태의 출처는 항상 코드이며, 아직 시작 안 한 항목에 붙은 단순 상태 태그는 파일 존재 여부 확인보다 나은 정보를 주지 못한다.
- `known_gaps`: "일부는 구현됐고 나머지는 빈 스텁이라 코드만 봐선 놓치기 쉬운" 구체적 함정. 완전히 연결되면 "완료"로 태그를 바꾸지 말고 항목 자체를 지운다(예시: 아래 Player 절 "HP 0 처리" 항목).

### Core
```yaml
decoupling:
  - 도킹 문 상태는 GameState가 아니라 UDockingDoorComponent가 별도 소유한다 — GameState는 탐사 진행 단계(로비/탐사/정산 등)만 책임진다. (구현 확인: `AGoHomeGameState`가 생성자에서 `UDockingDoorComponent`를 서브오브젝트로 소유하지만, 문 개폐 bool 자체와 `OnDoorStateChanged`는 컴포넌트가 갖는다 — "GameState 미참조"는 이 상태값에 대한 이야기이지 포인터를 안 갖는다는 뜻이 아니다.)
  - AI와 ASubmarine 둘 다 도킹 문 상태를 읽을 때 GameState 전체가 아니라 UDockingDoorComponent의 공개 상태(IsOpen(), OnDoorStateChanged)만 구독하고 GameState는 참조하지 않는다 — 게임 흐름 전체와 결합되지 않도록.

decisions:
  - name: 잠수정 오브젝트 구성
    detail: |
      단일 클래스 ASubmarine(허브 메시, 문 메시, InteriorVolume 트리거 슬롯만 정의, 에셋은 BP에서 배정)을
      로비/탐사 맵에 동일하게 배치한다. 두 맵의 잠수정 내부는 외피 크기 제약상 작아 서브레벨 분리 없이
      ZoneSelectMonitor·ADepartureButton을 Child Actor Component로 붙여 하나의 BP_Submarine으로 관리한다.
      기능 차이는 서브클래스가 아니라 런타임 GameState 타입 판별로 처리한다(외형은 두 맵에서 반드시 동일해야
      하는데, 서브클래스로 나누면 각 BP가 외형을 따로 들고 있어 드리프트 위험이 생기므로 기각). 각 맵은
      서로 다른 GameState 서브클래스를 쓰도록 이미 고정돼 있으므로, `GetWorld()->GetGameState<ALobbyGameState>()`
      유효 여부로 로비/탐사 컨텍스트를 자동 판별한다(EditInstanceOnly 플래그를 인스턴스마다 수동 지정하는
      방식은 새 인스턴스에서 값 설정을 빠뜨리면 조용히 오동작하는 실수 유형이 있어 기각):
        - ZoneSelectMonitor: CanInteract()가 `ALobbyGameState` 유효할 때만 true
        - ADepartureButton: `ALobbyGameState` 유효하면 Departure 동작(현재 구현), 아니면 Return 동작 —
          Return 동작은 DockingDoorComponent->SetOpen(false) 호출 후
          AGoHomeGameMode::ServerTravelViaLoadingScreen(로비 맵 경로) 호출(실제 경유 방식:
          `ServerTravelViaLoadingScreen`이 `UExpeditionTravelSubsystem::PendingDestinationMap`에 최종
          목적지를 저장한 뒤 `LoadingMapPath`로 먼저 트래블 — `LobbyMapPath`라는 필드는 없음)
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
  - name: 상태 전이 트리거 (EExpeditionState 7종, 02문서에 없어 이 문서에서 확정)
    detail: |
      - Lobby → Departure: 출발 버튼(`ADepartureButton`, Departure 모드) 상호작용 시, 전원 확정 없이 즉시(`ServerTravelToMap`이 트래블 직전 `SetState(Departure)` 호출). **주의**: 이 `SetState(Departure)`는 `ServerTravelToMap` 내부에 무조건 있어 이름과 달리 이 전이 전용이 아니다 — `ServerTravelViaLoadingScreen`(Return 전이도 재사용, 아래 참고)을 포함해 GameMode를 경유하는 모든 서버 트래블에서 발동한다.
      - `ZoneSelect`: 미사용 — Zone 선택은 상태 전이가 아니라 `LobbyGameState::SelectedZoneId` 값 변경(단일값, 팀 컨센서스 없음)만으로 처리
      - Departure → Exploration: 트래블 완료 후 `ExplorationGameState` 생성자가 `CurrentState`를 바로 초기화, `BeginPlay`(서버)에서 `DockingDoorComponent->SetOpen(true)` 자동 호출(로딩 연출 UI가 붙으면 이 타이밍 재검증 필요 — 현재는 UI 없이 즉시 열림 전제)
      - Exploration → Return: 귀환 버튼(`ADepartureButton`, Return 모드) 상호작용 시 `SetOpen(false)` → `ASubmarine`이 `InteriorVolume` 밖 플레이어 즉시 사망 처리 → 즉시 `ServerTravelViaLoadingScreen`(로비 맵 경로)(이전 초안이던 "전원 볼륨 진입 시 자동 전이"는 기각 — 버튼 트리거로 변경). 이 경로도 내부적으로 `ServerTravelToMap`을 타므로 곧 파괴될 `ExplorationGameState`가 `SetState(Departure)`로 세팅된 채 트래블에 들어간다 — 로딩 경유 중 그 상태를 구독하는 로직이 생기면 "Return"이 아니라 "Departure"로 보이니 구현 시 주의
      - Return → Settlement: 미착수 — 귀환 트래블은 로비 맵으로 직행하며, 정산 화면 연결은 별도 작업
      - Exploration/Return → Failed: 제한 시간 만료 또는 도킹 문 위협 판정(→ AI 경계, 몬스터가 문 근처를 위협하는 팀 전체 실패 조건 — "문 밖 즉사"와는 별개 개념) 또는 생존자 0명(→ Player 절 HP 0 처리)

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
  - 도킹 문 위협 판정은 UDockingDoorComponent의 공개 상태만 읽는다.

decisions:
  - name: GenerateNoise 호출 방식 (02문서 5절 근거)
    detail: GenerateNoise는 동기 함수로 반경 내 각 몬스터의 IMonsterNoiseListener::OnNoiseHeard를 호출까지만 책임진다. 전이 로직은 각 몬스터의 내부 구현(경계 밖)이 정한다.

known_gaps:
  - name: IMonsterNoiseListener 미연결
    detail: "실제 활성 몬스터 베이스인 `AUnderwaterEnemyBase`(BodyCollision/EyePoint/SkeletalMesh, `ReceiveDamage(float)`/`Die()` 보유)는 `IMonsterNoiseListener`를 구현하지 않는다 — 경계 인터페이스 미연결(코드 존재 자체는 미구현이 아님). **실사용 영향**: 이 인터페이스를 구현하는 액터가 현재 하나도 없어서, Item/Player가 호출하는 `GenerateNoise`(아이템 파손 소음, 플레이어 수영 소음 등)는 반경 내 몬스터가 있어도 아무 반응을 일으키지 않는다 — 호출부는 이미 구현돼 있지만(Player 쪽에서 실제로 사용 중) 몬스터 쪽 구독이 없어 지금은 사실상 no-op이다. `ReceiveDamage`→`IDamageable::ApplyDamage` 전환과 함께 `IMonsterNoiseListener` 구현이 필요."
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
  - name: 저장 트리거
    detail: UGoHomeSaveSubsystem이 FOnExpeditionStateChanged를 구독, NewState == ELobby일 때 저장 수행(Core는 Save 존재를 모름).
  - name: 레벨 이동 간 구독 유지
    detail: UGoHomeSaveSubsystem(UGameInstanceSubsystem, 트래블 간 유지)이 소유하고, 매 레벨의 새 AGoHomeGameState::BeginPlay에서 재구독.
  - name: 로비 맵 재진입 시 저장 누락 방지
    detail: 구독 직후 GameState 현재 상태를 동기 확인해 이미 ELobby면 즉시 저장(델리게이트 엣지 트리거에만 의존하지 않음).

known_gaps:
  - name: 저장 로직 전체 미구현
    detail: "`SaveToDisk()`/`OnExpeditionStateChanged()` 둘 다 빈 함수이고, `OnExpeditionStateChanged`를 GameState 델리게이트에 구독하는 코드도 없다 — 위 decisions 세 항목은 전부 설계 의도이며 아직 아무것도 동작하지 않는다."
```

## 시스템 간 인터페이스 계약

시스템 경계를 넘는 접점만 모은 표. 권한 표시 없는 항목은 서버 전용, "신설 제안"은 02문서에 없는 이 문서의 새 결정이다(시그니처 출처는 위 참고).

| 접점 | 정의 위치 | 호출·구독 측 | 근거 |
|---|---|---|---|
| `IWeightProvider` | Interaction | `UCarryWeightComponent`(Player)가 원시 무게 합산에만 소비 — Oxygen은 이 인터페이스를 직접 보지 않음(아래 `ICarryWeightProvider` 참고) | 02문서 3·4·8·13-3절 |
| `ICarryWeightProvider` (신설 제안) | Player (`CarryWeightProvider.h`) | `UCarryWeightComponent`가 구현(최대무게/업그레이드 반영한 초과무게 계산), `UOxygenComponent`(산소 소모 계산)·13-3 수류 구간·8번 장비 강화(페널티 곡선 파라미터)가 소비 | — |
| `IInteractable` | Interaction | Item이 구현, 13-1 게이트의 두 트리거가 각각 구현, `UInteractionComponent`(Interaction)가 트레이스로 찾은 대상에 호출 | 02문서 4·13-1절 |
| `IDamageable` (신설 제안) | Player | Player(HealthComponent)가 구현, AI(몬스터 공격)와 Player 자신(`UOxygenComponent`의 질식 데미지)이 소비 | — |
| `GenerateNoise` | AI | Item(구현됨, `ItemActorBase.cpp`)과 Player(구현됨, 수영 중 주기 소음 — `GoHomeCharacter.cpp`의 `SwimNoiseInterval`/`SwimNoiseRadius`)가 발생원. 호출 즉시 서버 동기 처리. **호출은 정상 동작하지만 현재 반응하는 몬스터가 없다**(AI 절 known_gaps "IMonsterNoiseListener 미연결" 참고) | 02문서 2·4·5절 |
| `IMonsterNoiseListener` (신설 제안) | AI | 몬스터 베이스가 구현(현재 미연결 — AI 절 known_gaps 참고) | `GenerateNoise`는 호출까지만 책임지고 전이 로직은 몬스터 구현체가 정한다(경계 밖) |
| GameState 상태 델리게이트 | Core | UI(바인딩), `UGoHomeSaveSubsystem`(진행 지점 저장), Interaction(정산 시점 갱신) | 02문서 1·6·9절 |
| 도킹 문 상태 | Core | AI(위협 판정 구독), `ASubmarine`(안전볼륨 밖 즉사 처리 구독), 13-1 게이트(기반 재사용 후보) | 02문서 1·13-1절 |
| 인벤토리 슬롯 | Interaction | UI(슬롯별 바인딩) | 02문서 4·6절 |
| `IDeathNotifier::OnDeath` (신설 제안) | Player (`DeathNotifier.h`, `UHealthComponent`가 구현) | Interaction(`UInventoryComponent`가 `BeginPlay`에서 `FindComponentByInterface<IDeathNotifier>()`로 구독 → `ServerDropAllItems` 실제 구현됨), Core(설계 의도 — 현재 미연결, Player 절 known_gaps "HP 0 처리" 참고) | HP 0 처리 결정(UHealthComponent 소유). `UHealthComponent`가 `IDeathNotifier`를 구현해 Interaction/Core가 구체 타입을 몰라도 사망 시점을 구독하게 한다 |
| 정산/납품 값 흐름 | Interaction → Core → Save | Interaction(딜리버리 존 진입 시 호출), Save(스키마 반영) | 02문서 4·9·13-2절 |

## 헤더 소유권

**소유권 규칙**: 각 헤더는 소유 폴더(대응은 위 "모듈/폴더 구조" 표) 담당자만 수정한다. 다른 폴더는 include해서 소비만 하고, 필드/시그니처 변경은 소유자 리뷰를 거친다. `AGoHomeGameState`처럼 여러 폴더가 쓰는 공유 클래스는 **필드 선언은 Core만**, 다른 폴더는 퍼블릭 서버 함수(`AddDeliveredValue` 등)로만 값을 바꾼다 — `GoHomeGameState.h` 동시 편집 충돌 방지용. 검증: `grep -rn "GameState->\w*\s*=" Source/GoHome/`가 Core 밖에서 나오면 위반.

## 남은 의존성

[02문서 10절](../Design/02_GoHome_기술분석서.md#10-착수-순서) 착수 순서표 중 아직 시작 안 한 항목만 남긴다 — `Player`/`Core`/`Interaction`/`AI`는 이미 구현이 진행 중이라 그 사이 착수 순서는 더 이상 의미가 없다(코드 자체가 순서의 증거).

- `UI/`(6. UI)는 `Core`/`Player`/`Interaction`이 안정된 뒤 착수 — 셋 다 이미 상당히 구현돼 있어 이 조건은 사실상 충족된 상태.
- `Save/`(8. 장비 강화)는 `Interaction`의 정산 로직과 `Core`의 로비 구조가 안정된 뒤 착수.
- 7번(레벨/그레이박스)은 `Source/GoHome/` 코드 폴더가 아니라 레벨 애셋 작업.

## 2차 프로토타입 추가 시스템 배치

[../Design/02_GoHome_기술분석서.md 13. 2차 프로토타입 추가 시스템](../Design/02_GoHome_기술분석서.md#13-2차-프로토타입-추가-시스템) 참고. 폴더 배치는:

- 13-1 동시 트리거 게이트(`UMultiActorGateComponent` (신설 제안)) → `Core/`(도킹 문 컴포넌트 확장). **동시 활성화 판정**: 트리거별 서버 전용 `bool` 배열을 소유, 각 트리거의 `OnInteract`가 `Server_SetTriggerActive(int32, bool)`를 호출하면 같은 호출 안에서 전원 활성 여부를 동기 검사해 게이트를 연다 — 서버는 한 틱에 한 RPC씩 순차 처리하므로 "동시" 입력도 레이스 없이 처리된다(Interaction의 동시 픽업과 같은 논리).
- 13-2 목표 점수 시스템 → `Core/`(GameState 필드) + `Save/`(스키마) + `UI/`(표시)
- 13-3 수류 구간(`PhysicsVolume`) → `Player/`(이동 시스템과 통합) 또는 레벨 배치, `ICarryWeightProvider` 참조

## Replication 권한 원칙

- 서버 권위: 도킹 문 상태, 이동/위치, 산소/HP, 아이템 소유권, 소음 이벤트, 정산/재화, 목표 점수
- 클라이언트: 단순 보간만 (이동), 입력 요청만 (상호작용/구매 — 서버 승인 후 반영)
