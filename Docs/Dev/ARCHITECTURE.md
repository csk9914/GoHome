# GoHome 기술 아키텍처 (개발용)

> `Docs/Dev/` 문서군. 코드로 알 수 있는 것(클래스/필드 목록)은 담지 않는다 — 컴파일러가 강제하지 않는 경계 규칙·코드만 봐선 안 보이는 계약·결정 이유만 다룬다. 인일/복잡도는 GoHome 기술분석서(Notion), 정확한 시그니처는 `Source/GoHome/Public/` 헤더가 각각 유일한 출처.
>
> AI 내부 구현(상태 머신·조향 로직)은 팀원 재량이라 다루지 않는다 — 데미지 전달·소음 감지·도킹 문 상태 구독 등 경계 인터페이스만 고정한다.
>
> 기획 문서(GoHome 기획서·기술분석서)는 Notion에 있고 이 저장소엔 없다 — 설계 근거가 필요하면 사용자에게 요청할 것.

## 목차

- [모듈/폴더 구조](#모듈폴더-구조)
- [시스템별 결정과 경계](#시스템별-결정과-경계)
- [시스템 간 인터페이스 계약](#시스템-간-인터페이스-계약)
- [공유 헤더 규칙](#공유-헤더-규칙)
- [남은 의존성](#남은-의존성)
- [2인 협동 게이트 실제 배치](#2인-협동-게이트-실제-배치)
- [Replication 권한 원칙](#replication-권한-원칙)

## 모듈/폴더 구조

`Source/GoHome/`은 기능별 분리(`Player`/`Interaction`/`AI`/`Item`/`UI`/`Save`)를 따르되 `Core/`를 추가 신설 — GameState 상태 머신·세션/로비 흐름은 특정 시스템이 아닌 게임 흐름 자체라서.

`Core/` 내부: 흐름 제어 클래스(GameMode/GameState/PlayerController/SessionSubsystem, 월드 미배치)는 바로 아래, 월드 배치 액터(`AZoneSelectMonitor`, `ADepartureButton`)는 `Core/Actors/`로 분리(`Item/AItemActorBase`처럼 배치형은 성격이 달라 구분).

폴더별 클래스/인터페이스 출처는 `Source/GoHome/<폴더>/`. 아래 표는 코드에 없는 정보만 — 성격이 섞인 폴더의 비고("정식 홈"이 아니라 "일단 여기" — 기준은 아래 "새 폴더를 파야 하는가"):

| 폴더 | 대응 시스템 | 비고 |
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

기준: 기술적 의존만으론 기존 폴더에 끼워 넣지 않는다 — 전용 Component/Actor 세트를 갖는 독립 시스템이면 새 폴더("이 시스템만의 클래스 개수"로 판단). 실제 사례는 위 표 "비고" 열 참고.

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
    detail: "GoHomeGameMode.cpp: bUseSeamlessTravel = true. GameState(따라서 서브오브젝트인 DockingDoorComponent)는 트래블마다 새로 스폰되므로, Save뿐 아니라 도킹 문 상태를 구독하는 AI/ASubmarine도 매 레벨 BeginPlay에서 재구독해야 한다. 전환용 임시 GameState 구간의 재구독 타이밍은 미검증."
  - name: 상태 전이 트리거 (EExpeditionState 7종)
    detail: |
      - Lobby → Departure: 출발 버튼 상호작용 즉시(전원 확정 없음). **주의**: `SetState(Departure)`는 `ServerTravelToMap` 내부에 무조건 있어, `ServerTravelViaLoadingScreen`(Return도 재사용)을 포함한 모든 서버 트래블에서 발동 — 이 전이 전용이 아니다.
      - `ZoneSelect`: 미사용 — `LobbyGameState::SelectedZoneId` 값 변경만으로 처리(상태 전이 아님)
      - Departure → Exploration: 트래블 완료 후 `ExplorationGameState` 생성자가 즉시 초기화, `BeginPlay`(서버)에서 `DockingDoorComponent->SetOpen(true)` 자동 호출(로딩 UI 붙으면 타이밍 재검증 필요)
      - Exploration → Return: 귀환 버튼 상호작용 시 `SetOpen(false)` → `ASubmarine`이 `InteriorVolume` 밖 플레이어 즉시 사망 처리 → `ServerTravelViaLoadingScreen`(로비 맵). **주의**: 이 경로도 `ServerTravelToMap`을 타므로, 곧 파괴될 `ExplorationGameState`가 `SetState(Departure)`로 세팅된 채 트래블에 들어간다 — 로딩 경유 중 그 상태를 구독하면 "Return"이 "Departure"로 보임
      - Exploration → Return → Settlement: (설계 확정, 미구현) Return/Settlement를 탐사맵 인맵 페이즈로 사용 — 귀환버튼이 문 닫고 연출 대기 후 Settlement 진입, FinalizeRound + 정산 UI + 자동복귀 타이머, 그 다음에 로비 트래블. Save 절 decisions '정산 페이즈는 탐사맵 안에서' 참고.
      - Exploration/Return → Failed: 제한 시간 만료, 도킹 문 위협 판정, 생존자 0명(→ Player 절 HP 0 처리) 중 하나. Failed도 FinalizeRound(bForfeited=true) 후 "구조 실패" 화면 + 자동복귀.

known_gaps:
  - name: 정산 값 누적 미연결
    detail: "`UInventoryComponent`(Interaction)가 납품 시 `GameState->AddDeliveredValue(int32)`를 호출하지만 `AGoHomeGameState::AddDeliveredValue`가 빈 스텁이라 값이 누적되지 않는다 — `SaveSubsystem::AccumulateDeliveredValue`로 포워드하는 배선 필요(미연결 스텁 전체는 `남은 의존성`)."
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
    detail: "설계 의도: HealthComponent가 서버 전용 OnDeath 브로드캐스트, GameState가 탐사마다 재구독해 생존자 수 추적, 0명이면 Fail(EFailReason::AllPlayersDead). 현재 OnDeath 브로드캐스트는 구현됐지만 GameState 구독부와 Fail()은 빈 스텁 — 미연결(`남은 의존성` 참고)."
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
    detail: Interaction이 GameState->AddDeliveredValue(int32)를 직접 호출(델리게이트로 감싸지 않음). 수신 측 미연결 — `남은 의존성` 참고.
  - name: 상호작용 대상 탐지 방식
    detail: 로컬 컨트롤 폰에서만 탐지가 동작한다 — 다른 클라이언트의 리플리케이트된 폰까지 트레이스하면 안 된다. 구체 파라미터는 `UInteractionComponent` 소스.
```

### AI (경계만)
```yaml
note: 내부 구현 범위는 문서 상단 note 참고(AI 상태 머신·조향 로직은 팀원 재량).

boundary_interfaces:
  - 공격 시 대상의 IDamageable::ApplyDamage를 호출 — 몬스터는 대상이 Player인지 몰라도 된다.
  - 소음 감지는 발생원이 호출하는 GenerateNoise가 대신 처리 — AI는 발생원이 Player인지 Item인지 몰라도 된다.
  - 도킹 문 위협 판정은 UDockingDoorComponent의 공개 상태만 읽는다(현재 C++/BP 어느 쪽에도 이를 구독하는 코드가 없다 — 아래 known_gaps 참고).

decisions:
  - name: GenerateNoise 호출 방식
    detail: GenerateNoise는 동기 함수로 반경 내 각 몬스터의 IMonsterNoiseListener::OnNoiseHeard를 호출까지만 책임진다. 전이 로직은 각 몬스터의 내부 구현(경계 밖)이 정한다. IMonsterNoiseListener는 C++ 베이스가 아니라 몬스터 BP(`BP_UnderwaterMonster`, `BP_WormBase`)가 직접 구현한다(Blueprint Implemented Interfaces — C++ 부모 상속과 무관, `.uasset`에 `MonsterNoiseListener`/`OnNoiseHeard` 참조로 확인됨. 현재 BP 쪽에서 구현 진행 중). `AMonsterBase`(C++, 이 인터페이스를 포함한 경계 3종을 다 구현)는 실제로는 아무 BP도 상속하지 않는 고아 클래스다 — 아래 known_gaps 참고.

known_gaps:
  - name: AMonsterBase 미사용 (고아 클래스)
    detail: "`AI/MonsterBase.h`의 `AMonsterBase`는 경계 3종(IDamageable 호출·IMonsterNoiseListener 구현·도킹 문 상태 구독)을 다 갖췄지만 아무 BP도 상속하지 않는다. 실제 스폰되는 `BP_UnderwaterMonster`는 `AUnderwaterEnemyBase`(빈 APawn), `BP_WormBase`는 `AActor`를 직접 상속한다. 데미지 처리·도킹 문 위협 구독은 `AMonsterBase` 경유 없이 각 BP가 별도 구현해야 한다(소음 감지는 원래 BP가 직접 구현이라 무관)."
  - name: 도킹 문 위협 판정 미구현
    detail: "boundary_interfaces에 계약만 있고, UDockingDoorComponent::OnDoorStateChanged를 구독해 위협 판정 후 GameState::Fail(EFailReason)을 호출하는 코드가 AI 쪽에 없다."
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
    detail: FCoreUObjectDelegates::PostLoadMapWithWorld 구독으로 레벨 전환마다 새 AGoHomeGameState::OnStateChanged에 재구독하고, NewState == Lobby일 때 SaveToDisk() 호출. 재구독 직후 이미 Lobby 상태면 즉시 저장(델리게이트 엣지 트리거 누락 방지). FinalizeRound()도 끝에서 직접 SaveToDisk() 하므로 정산 결과는 트래블 전에 디스크에 남는다.
  - name: 저장 주체
    detail: OnPostLoadMap이 NM_Client면 즉시 리턴 — 호스트만 저장을 수행한다. SaveGame은 호스트에만 실재(클라의 서브시스템은 자기 로컬 빈 세이브만 가짐) → 클라 UI는 세이브를 직접 읽지 못하고 리플리케이트된 struct(FSettlementResult / FExpeditionProgress)로만 상태를 안다.
  - name: 진행/판정 모델
    detail: |
      탐사 1회 = 1턴. 게임오버 경로 2개가 병존:
        - 3스트라이크: 매 턴 유효 납품액 < 그 맵의 MapQuota → QuotaMissCount +1(감소 없음). 3 도달 시 게임오버.
        - 체크포인트: EconomyConfigDataAsset.CheckPoints의 Round(예: 3·6·9) 종료 시 CurrentFunds < 그 체크포인트 목표 → 즉시 게임오버.
      마지막 체크포인트 Round = 게임 총 턴 수 = 엔딩 판정 턴. 목표 달성 시 엔딩 → 세이브 초기화 → 타이틀.
      게임오버·엔딩 모두 ResetSave()(새 SaveGame 인스턴스).
      CurrentFunds: 납품 +, 강화/구매 −, 음수 허용(빚). 강화 투자가 곧 체크포인트 리스크(의도된 텐션).
  - name: 정산 페이즈는 탐사맵 안에서 (트래블 아님)
    detail: |
      귀환/실패가 확정되는 순간(탐사맵, 서버)에 SaveSubsystem::FinalizeRound(bForfeited, CasualtyNames, MapQuota)를 명시적으로 호출해 세이브 데이터를 확정하고, 그 결과 FSettlementResult를 정산 UI로 넘긴다. 로비 도착 감지·라운드진행 플래그 같은 우회 없음.
      - 정상 복귀: ADepartureButton 탐사 브랜치 → 문 닫힘 연출 대기 후 SetState(Settlement) → FinalizeRound(false, ...). 자동복귀 타이머 또는 "복귀" 버튼으로 로비 트래블.
      - 완전 실패(전원사망·타임오버): GameState::Fail() → SetState(Failed) → FinalizeRound(true, ...) (그 턴 납품액 몰수 = CurrentFunds에서 되돌림). "구조 실패" 화면 + 자동복귀만(정산표 없음). 실패도 그 턴 스트라이크·체크포인트 판정은 정상 수행.
      - Submarine 외부인원 즉사(SetOpen(false) 구독)가 FinalizeRound보다 먼저 동기 실행되므로 사망자 수가 정산에 반영됨 — 이 순서 의존성 유지.
      - EExpeditionState의 Return/Settlement/Failed 값을 탐사맵 인맵 페이즈로 실제 사용(그 전엔 맵별 GameState 클래스 생성자가 상태를 하드셋할 뿐 미사용).
  - name: EconomyConfig 로딩
    detail: "UGoHomeSaveSubsystem::Initialize()에서 하드코딩 경로로 LoadObject<UEconomyConfigDataAsset>(/Game/GoHome/Data/DA_EconomyConfig) + ensureMsgf. DeveloperSettings 방식은 클래스 하나 더 필요해 보류. CheckPoints 배열은 Round 오름차순+중복 없음을 UEconomyConfigDataAsset::IsDataValid(#if WITH_EDITOR)가 강제 — FindNextCheckPoint 등은 이 불변식을 가정한다."

known_gaps:
  - name: 정산 배선 절반 완료 (SaveSubsystem 로직 O, 호출부·UI·애셋 X)
    detail: |
      구현됨: UGoHomeSaveSubsystem::AccumulateDeliveredValue / FinalizeRound(forfeit 롤백·CasualtyFee 차감·스트라이크·DetermineOutcome·터미널 시 ResetSave·SaveToDisk) / BuildProgress / ResetSave.
      관련 struct: FSettlementResult(ESettlementOutcome), FExpeditionProgress, UEconomyConfigDataAsset(FCheckPoint+CasualtyFee), UGoHomeSaveGame 필드(CurrentFunds/CurrentRoundDeliveredValue/CurrentRound/QuotaMissCount), ExpeditionZoneDataAsset.MapQuota.
      미연결분은 `남은 의존성` 참고.
```

## 시스템 간 인터페이스 계약

시스템 경계를 넘는 접점만 모은 표(시그니처 출처는 위 "모듈/폴더 구조" 참고).

| 접점 | 정의 위치 | 호출·구독 측 |
|---|---|---|
| `IWeightProvider` | Interaction | `UCarryWeightComponent`(Player)가 원시 무게 합산에만 소비 — Oxygen은 이 인터페이스를 직접 보지 않음(아래 `ICarryWeightProvider` 참고) |
| `ICarryWeightProvider` | Player (`CarryWeightProvider.h`) | `UCarryWeightComponent`가 구현(최대무게/업그레이드 반영한 초과무게 계산), `UOxygenComponent`(산소 소모 계산)·환경 위해요소(수류 존)·장비 강화(페널티 곡선 파라미터)가 소비 |
| `IInteractable` | Interaction | Item이 구현, 협동 게이트의 레버(`ACoopLeverActor`)가 구현, `UInteractionComponent`(Interaction)가 트레이스로 찾은 대상에 호출 |
| `IDamageable` | Player | Player(HealthComponent)가 구현, AI(몬스터 공격)와 Player 자신(`UOxygenComponent`의 질식 데미지)·환경 위해요소(열수구)가 소비 |
| `GenerateNoise` | AI | Item(구현됨, `ItemActorBase.cpp`)과 Player(구현됨, 수영 중 주기 소음 — `GoHomeCharacter.cpp`의 `SwimNoiseInterval`/`SwimNoiseRadius`, 보이스챗 발화 시)가 발생원. 호출 즉시 서버 동기 처리 |
| `IMonsterNoiseListener` | AI | `BP_UnderwaterMonster`/`BP_WormBase`가 각각 직접 구현(BP 쪽에서 구현 진행 중, C++ 몬스터 베이스 상속과 무관 — AI 절 decisions 참고) |
| GameState 상태 델리게이트 | Core | UI(바인딩), `UGoHomeSaveSubsystem`(진행 지점 저장), Interaction(정산 시점 갱신) |
| 도킹 문 상태 | Core | AI(위협 판정 구독 — 설계 의도, 현재 미연결. AI 절 known_gaps 참고), `ASubmarine`(안전볼륨 밖 즉사 처리 구독, 구현됨), `UMultiActorGateComponent`(기반 재사용) |
| 인벤토리 슬롯 | Interaction | UI(슬롯별 바인딩) |
| `IDeathNotifier::OnDeath` | Player (`DeathNotifier.h`, `UHealthComponent`가 구현) | Interaction(`UInventoryComponent`가 `BeginPlay`에서 `FindComponentByInterface<IDeathNotifier>()`로 구독 → `ServerDropAllItems` 구현됨), Core(설계 의도 — 현재 미연결). 구체 타입을 몰라도 사망 시점을 구독하게 하는 게 목적 |
| 정산/납품 값 흐름 | Interaction → Core → Save | Interaction(딜리버리 존 진입 시 호출), Save(스키마·로직 O, GameState 포워드 미연결 — `남은 의존성` 참고) |

## 공유 헤더 규칙

**공유 헤더**(`GoHomeGameState.h`, 인터페이스 헤더 — `IInteractable`/`IWeightProvider`/`IDeathNotifier` 등)는 다른 곳에서 include해 **소비만** 한다: 값 변경은 퍼블릭 함수로만 하고 필드 직접 대입은 금지한다(검증: `grep -rn "GameState->\w*\s*=" Source/GoHome/`가 Core 밖에서 나오면 위반).

**단, GameState에 게임 규칙 로직을 쌓지 않는다**: `AGoHomeGameState`는 클라가 읽을 복제 상태(read-model)만 소유한다. 서버 권위 규칙(정산 누적, 파산·게임오버 판정, 파티 이탈 처리)은 서버 전용인 `AGoHomeGameMode`/`AExplorationGameMode` 또는 서버 서브시스템(`UGoHomeSaveSubsystem`)에 둔다 — GameMode는 클라에 존재하지 않아 `HasAuthority` 가드조차 필요 없다. cross-folder 호출부가 GameState를 거쳐야 하면 `AddDeliveredValue`류 퍼블릭 서버 함수는 **로직 없는 얇은 포워더**로 두고 실제 처리는 GameMode/서브시스템에 위임한다.

## 남은 의존성

미연결 스텁 마스터 목록 — 각 절 known_gaps는 여기를 가리킨다.

- **`AGoHomeGameState::AddDeliveredValue` / `Fail` / `OnPlayerRemovedFromParty` 빈 스텁** — 여기에 아래가 다 걸려 있다:
  - 정산 누적: `UInventoryComponent`가 `AddDeliveredValue` 호출하지만 `SaveSubsystem::AccumulateDeliveredValue`로 포워드 안 됨
  - 생존자 수 추적 / 파산·전원사망 판정: `Fail()` 빈 스텁
  - 접속 종료: `OnPlayerRemovedFromParty` 빈 함수 (생존자 수에 미반영)
- **도킹 문 위협 판정** — AI가 `OnDoorStateChanged` 구독해 `Fail()` 호출하는 코드 없음
- **정산 배선 나머지** — 사망자 추적(`IDeathNotifier::OnDeath`), 정산 페이즈 머신·자동복귀 타이머·복귀 RPC, DA_EconomyConfig 애셋 생성, DepartureButton 재배선, 정산/게임오버/엔딩 UI 위젯
- `UI/`는 C++ 베이스 클래스 없음(Blueprint 전용).
- `Save/` 장비 강화 구매 로직 미구현 — 스키마 필드(`PurchasedUpgrades`)만 있음.
- 레벨/그레이박스는 `Source/GoHome/` 코드가 아니라 레벨 애셋 작업.

## 2인 협동 게이트 실제 배치

`UMultiActorGateComponent`/`ACoopLeverActor`/`AWeightPlateActor`: 도킹 문 컴포넌트 재사용 이유로 `Core/` 배치를 검토했으나 실제론 `Interaction/`에 구현(상호작용 트리거 계열 판단) — 계획과 코드가 갈린 사례(폴더 기준은 "새 폴더를 파야 하는가" 참고).

**동시 활성화 판정**: 트리거별 서버 전용 `bool` 배열 소유, 각 `OnInteract`가 `Server_SetTriggerActive(int32, bool)` 호출 시 같은 호출 안에서 전원 활성 여부를 동기 검사해 게이트를 연다 — 서버는 한 틱에 한 RPC씩 순차 처리하므로 "동시" 입력도 레이스 없음(Interaction 동시 픽업과 같은 논리).

## Replication 권한 원칙

- 서버 권위: 도킹 문 상태, 이동/위치, 산소/HP, 아이템 소유권, 소음 이벤트, 정산/재화, 목표 점수
- 클라이언트: 로컬 폰(autonomous proxy)은 `CharacterMovementComponent` 표준 클라이언트 예측 + 서버 보정, 다른 클라이언트의 폰(simulated proxy)은 순수 보간만 (AGoHomeCharacter는 이동 관련 오버라이드 없이 표준 ACharacter 권한 모델을 그대로 사용). 입력 요청만 (상호작용/구매 — 서버 승인 후 반영)
