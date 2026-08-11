# GoHome 기술 아키텍처 (개발용)

> `Docs/Dev/` 문서군. 링크 검증은 아래 "Docs/Design 참조 점검 체크리스트" 절 참고.
>
> 이 문서는 **코드를 읽어도 알 수 없는 것만** 담는다 — 어떤 클래스가 있고 어떤 필드를 가졌는지는 `Source/GoHome/`이 유일한 출처이므로 여기 옮겨 적지 않는다. 이 문서가 다루는 건 컴파일러가 강제하지 않는 경계 규칙, 여러 파일에 흩어져 있어 코드만 봐서는 한눈에 안 보이는 계약, 그리고 그 결정을 내린 이유(왜 이렇게 했는지)다. **인일/복잡도/담당자 수치**의 유일한 출처는 [../Design/02_GoHome_기술분석서.md](../Design/02_GoHome_기술분석서.md)다. **경계 인터페이스가 왜 필요한지·누가 호출/구독하는지(아래 "시스템별 결정과 경계", "시스템 간 인터페이스 계약" 두 절)는 이 문서가 유일한 출처**이며, 반대로 **정확한 시그니처는 `Source/GoHome/Public/`의 헤더가 유일한 출처**다(헤더 커밋 후 문서와 어긋날 수 있으므로 표에는 시그니처를 옮겨 적지 않는다).
>
> AI(`Source/GoHome/AI/`)의 내부 구현(상태 머신, 조향 로직 등)은 팀원 작업에 따라 바뀔 수 있으므로 이 문서에서 다루지 않는다. AI는 다른 시스템과 맞닿는 **경계 인터페이스**(데미지 전달, 소음 감지, 도킹 문 상태 구독)만 이 문서에서 고정한다 — 이 경계만 지키면 AI 내부 구현이 바뀌어도 다른 시스템은 영향받지 않는다.

## 목차

- [Docs/Design 참조 점검 체크리스트](#docsdesign-참조-점검-체크리스트)
- [모듈/폴더 구조](#모듈폴더-구조)
- [시스템별 결정과 경계](#시스템별-결정과-경계)
- [시스템 간 인터페이스 계약](#시스템-간-인터페이스-계약)
- [헤더 소유권](#헤더-소유권)
- [의존성/착수 순서](#의존성착수-순서)
- [2차 프로토타입 추가 시스템 배치](#2차-프로토타입-추가-시스템-배치)
- [Replication 권한 원칙](#replication-권한-원칙)

## Docs/Design 참조 점검 체크리스트

스크립트 동작 방식은 저장소 루트 [CLAUDE.md 검증 스크립트](../../CLAUDE.md#validation-script) 절 참고. 스크립트가 못 잡는 부분(본문 텍스트로만 언급된 절 번호)이 있으니, `Docs/Design/01`·`02`문서의 절 번호나 헤더 제목을 바꿀 때마다 아래를 추가로 확인한다:

1. `grep -rn "Design/0" Docs/Dev/` 로 이 문서군에서 Design 문서를 참조하는 모든 줄을 찾아, 절 번호가 **본문 텍스트로만**(링크가 아닌 형태로) 언급된 곳이 있는지 확인한다.
2. 헤더 번호(예: "10. 착수 순서", "13-1. ...")가 바뀌었다면, 이 문서의 "의존성/착수 순서"·"2차 프로토타입 추가 시스템 배치" 절에서 같은 번호를 인용한 곳도 함께 갱신한다.

## 모듈/폴더 구조

`Source/GoHome/` 하위는 02문서가 이미 확정한 기능별 분리 규칙(`Player`/`Interaction`/`AI`/`Item`/`UI`/`Save`)을 그대로 따르되, 02문서에 없던 `Core/` 폴더를 이 문서에서 신설한다 (GameState 상태 머신·세션/로비 흐름은 특정 아이템/AI 계열이 아니라 게임 흐름 자체를 다루므로 별도 폴더가 필요했음 — **이 결정의 유일한 출처는 이 문서**).

각 폴더가 실제로 어떤 클래스를 담고 있고 어떤 인터페이스를 구현하는지는 `Source/GoHome/<폴더>/`가 유일한 출처이므로(`grep -rl`로 인터페이스 구현체 검색 가능) 여기 옮겨 적지 않는다. 폴더 ↔ 02문서 시스템 번호 대응만 코드에 없는 정보라 아래에 남긴다:

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

각 폴더에서 코드로는 강제되지 않는 결합도 경계와, 왜 그렇게 결정했는지만 다룬다. 클래스 목록·필드·구현 상태는 `Source/GoHome/<폴더>/`가 유일한 출처이므로 여기 옮겨 적지 않는다.

### Core
```yaml
decoupling:
  - 도킹 문 상태는 GameState가 직접 들고 있지 않고 UDockingDoorComponent가 별도로 소유한다 — GameState는 탐사 진행 단계(로비/탐사/정산 등)만 책임진다.
  - AI가 도킹 문 상태를 읽을 때 GameState 전체를 참조하지 않고 UDockingDoorComponent의 공개 상태(IsOpen(), 상태 변경 델리게이트)만 구독한다 — AI가 게임 흐름 전체와 결합되지 않도록.

decisions:
  - name: 세션 생명주기
    detail: USessionSubsystem은 UGameInstanceSubsystem(트래블 간 유지). UI는 BP 델리게이트만 구독, GameState는 세션 트리거로 쓰지 않음.
  - name: 상태 전이 트리거 (02문서에 없어 이 문서에서 확정)
    detail: |
      - 로비 → 탐사 지역 선택: 전원 지역 확정 시
      - 탐사 지역 선택 → 출발: 전원 준비 완료 시
      - 탐사 → 복귀/정산: 도킹 문 닫힘 + 생존자 전원 잠수정 콜리전 볼륨 내부
      - 탐사 → 실패: 제한 시간 만료 또는 도킹 문 위협 판정(→ AI 경계 참고)
```

### Player
```yaml
decoupling:
  - "UOxygenComponent는 소모 속도 계산 시 Interaction의 IWeightProvider를 참조하되, UInventoryComponent를 직접 참조하지 않고 인터페이스로만 접근한다."
  - "UHealthComponent는 \"누가 데미지를 주는지\" 몰라야 한다 → IDamageable(아래 인터페이스 계약 참고)로 노출해 호출자가 Character 타입을 몰라도 되게 한다."

decisions:
  - name: HP 0 처리
    detail: UHealthComponent가 서버 전용 OnDeath 브로드캐스트. GameState가 탐사마다 각 Character의 OnDeath에 재구독해 생존자 수 추적, 0명이면 Fail(EFailReason::AllPlayersDead).
  - name: 산소 0 → 데미지 경로
    detail: UOxygenComponent가 산소 0시 매 틱 IDamageable::ApplyDamage(질식 데미지, Owner, "Suffocation") 동기 호출.
  - name: 중복 사망 방지
    detail: OnDeath는 캐릭터당 탐사 1회만 브로드캐스트됨 — Core가 생존자 수를 추적하므로 이 보장이 깨지면 카운트가 틀어진다.
  - name: 접속 종료 처리
    detail: AGoHomeGameMode::Logout도 GameState::OnPlayerRemovedFromParty(APlayerState*)를 호출(OnDeath와 동일 경로 합류, TSet으로 멱등 보장).
```

### Interaction
```yaml
decoupling:
  - 소켓 어태치는 Player의 스켈레탈 메시 소켓에 접근해야 하므로 두 폴더 사이의 결합 지점이다 — 소켓 어태치 헬퍼는 Interaction에 두되, Character는 "오른손/왼손 소켓 이름"만 공개 프로퍼티로 노출해 Interaction이 Character 내부 구조를 몰라도 되게 한다.

decisions:
  - name: 동시 픽업 레이스 컨디션
    detail: 동시 픽업 요청 시 한 명만 성공이 보장됨(구현 방식은 아이템 액터 내부).
  - name: 정산 값 전달
    detail: Interaction이 GameState->AddDeliveredValue(int32)를 직접 호출(델리게이트로 감싸지 않음).
  - name: 상호작용 대상 탐지 방식
    detail: 로컬 컨트롤 폰에서만 탐지가 동작한다 — 다른 클라이언트의 리플리케이트된 폰까지 트레이스하면 안 되므로(코드만 봐서는 이 의도가 드러나지 않는 subtle한 제약). 구체 탐지 파라미터는 `UInteractionComponent` 소스가 유일한 출처.
```

### AI (경계만)
```yaml
note: 내부 상태 머신·조향 로직은 팀원 구현에 맡기며 이 문서에서 다루지 않는다.

boundary_interfaces:
  - 공격 시 대상의 IDamageable::ApplyDamage를 호출 — 몬스터는 대상이 Player인지 몰라도 된다.
  - 소음 감지는 발생원이 호출하는 GenerateNoise가 대신 처리 — AI는 발생원이 Player인지 Item인지 몰라도 된다.
  - 도킹 문 위협 판정은 UDockingDoorComponent의 공개 상태만 읽는다.

decisions:
  - name: GenerateNoise 호출 방식 (02문서 5절 근거)
    detail: GenerateNoise는 동기 함수로 반경 내 각 몬스터의 IMonsterNoiseListener::OnNoiseHeard를 호출까지만 책임진다. 전이 로직은 각 몬스터의 내부 구현(경계 밖)이 정한다.
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
```

## 시스템 간 인터페이스 계약

시스템 경계를 넘는 접점만 모은 표. 서버/클라 권한 표시가 없는 항목은 서버 전용이다. "신설 제안" 표시는 02문서에 없는 이 문서의 새 결정이다. **정확한 시그니처는 아래 "정의 위치" 폴더의 헤더(`Source/GoHome/Public/`)가 유일한 출처** — 헤더 커밋 후 바뀌었을 수 있으니 표에 다시 옮겨 적지 않는다.

| 접점 | 정의 위치 | 호출·구독 측 | 근거 |
|---|---|---|---|
| `IWeightProvider` | Interaction | Player(산소 소모 계산), 13-3 수류 구간, 8번 장비 강화(페널티 곡선 파라미터) | 02문서 3·4·8·13-3절 |
| `IInteractable` | Interaction | Item이 구현, 13-1 게이트의 두 트리거가 각각 구현, `UInteractionComponent`(Interaction)가 트레이스로 찾은 대상에 호출 | 02문서 4·13-1절 |
| `IDamageable` (신설 제안) | Player | Player(HealthComponent)가 구현, AI(몬스터 공격)와 Player 자신(`UOxygenComponent`의 질식 데미지)이 소비 | 02문서에 데미지 전달 경로가 없어 이 문서에서 신설 |
| `GenerateNoise` | AI | Player(이동 사운드)·Item(소음 유발형)이 발생원(호출 즉시 동기 처리, 위 "AI (경계만)" 참고) | 02문서 2·4·5절 |
| `IMonsterNoiseListener` (신설 제안) | AI | `AMonsterBase`가 구현, `GenerateNoise`가 반경 내 각 몬스터에 호출(반응 로직은 구현체 내부, 경계 밖) | 위 "AI (경계만)" `GenerateNoise` 호출 방식 정정 참고 |
| GameState 상태 델리게이트 | Core | UI(바인딩), `UGoHomeSaveSubsystem`(진행 지점 저장), Interaction(정산 시점 갱신) | 02문서 1·6·9절 |
| 도킹 문 상태 | Core | AI(위협 판정 구독), 13-1 게이트(기반 재사용 후보) | 02문서 1·13-1절 |
| 인벤토리 슬롯 | Interaction | UI(슬롯별 바인딩) | 02문서 4·6절 |
| `OnDeath` (신설 제안) | Player | Core(GameState 생존자 수 추적, 탐사마다 재구독) | 위 "Player" 절 HP 0 처리 결정 참고 |
| 정산/납품 값 흐름 | Interaction → Core → Save | Interaction(딜리버리 존 진입 시 호출), Save(스키마 반영) | 02문서 4·9·13-2절 |

## 헤더 소유권

Day 1 병렬 착수용 헤더 스텁(`IWeightProvider`, `IInteractable`, `IDamageable`, `ENoiseType`, `EExpeditionState`, `EFailReason`, `UDockingDoorComponent`, `AGoHomeGameState`, `UHealthComponent` 등)은 이미 `Source/GoHome/Public/` 하위에 커밋되어 있다 — 선언 내용은 그쪽 헤더 파일이 유일한 출처이며, 시그니처는 위 "시스템 간 인터페이스 계약" 표를 따른다.

**소유권 규칙**: 각 헤더는 소유 폴더(위 "모듈/폴더 구조" 표) 담당자만 수정한다. 다른 폴더는 include해서 인터페이스/타입만 소비하며, 필드 추가나 시그니처 변경이 필요하면 소유자 리뷰를 거친다. `AGoHomeGameState`처럼 여러 폴더가 값을 쓰는 공유 클래스는 **필드 선언은 Core만** 하고, 다른 폴더는 반드시 퍼블릭 서버 함수(`AddDeliveredValue` 등)로만 값을 바꾼다 — `GameState.h`를 여러 사람이 동시에 손대며 생기는 병합 충돌을 피하기 위함이다.

## 의존성/착수 순서

> 아래는 [02문서 10절](../Design/02_GoHome_기술분석서.md#10-착수-순서) 표를 코드 폴더 단위로 환산한 것이다. 시스템 번호→폴더 대응은 위 "모듈/폴더 구조" 표를 따른다. 순서를 정한 이유(왜 먼저/나중인지)는 여기 옮겨 적지 않으며, 근거는 전부 02문서 10절에 있다.

```
Player/ (2. 이동)                          ─ 최우선 착수, 크리티컬 패스
   │
   ├──▶ Core/ (1. 온라인 서브시스템)         ─ 최소 기능은 병행 가능
   ├──▶ Player/ (3. 산소+HP)                 ─ Interaction의 IWeightProvider 스텁 합의 후
   └──▶ Interaction/ (4. 상호작용/운반/납품)

AI/ (5. AI 몬스터)                          ─ 2번 완료 대기 없이 즉시 착수 가능

Core/ + Player/ + Interaction/ ──▶ UI/ (6. UI)

Core/ (1. 도킹 문 배치 협의) ──▶ (7. 레벨 — 코드 폴더 없음, 그레이박스 작업)

Save/ (9. 세이브 스키마) ──▶ Save/ (8. 장비 강화)
   ▲                             ▲
   └── Interaction/(4, 정산 로직 안정) · Core/(1, 로비 구조 안정)
```

7번(레벨/그레이박스)은 `Source/GoHome/` 코드 폴더가 아니라 레벨 애셋 작업이므로 위 트리에 폴더로 넣지 않았다.

## 2차 프로토타입 추가 시스템 배치

[../Design/02_GoHome_기술분석서.md 13. 2차 프로토타입 추가 시스템](../Design/02_GoHome_기술분석서.md#13-2차-프로토타입-추가-시스템) 참고. 폴더 배치는:

- 13-1 동시 트리거 게이트(`UMultiActorGateComponent`) → `Core/` (도킹 문 컴포넌트 확장). **동시 활성화 판정 (결정)**: `UMultiActorGateComponent`가 트리거별 서버 전용 `bool` 배열을 소유한다. 각 트리거(`IInteractable` 구현체)의 `OnInteract`는 `Server_SetTriggerActive(int32 Index, bool bActive)`를 공유 컴포넌트에 호출하고, 이 함수가 같은 호출 안에서 전원 활성 여부를 동기 검사해 게이트를 연다 — 위 "Interaction" 절 동시 픽업 레이스 컨디션 결정과 동일한 "단일 스레드 서버 틱" 논리가 여기도 적용된다.
- 13-2 목표 점수 시스템 → `Core/`(GameState 필드) + `Save/`(스키마) + `UI/`(표시)
- 13-3 수류 구간(`PhysicsVolume`) → `Player/`(이동 시스템과 통합) 또는 레벨 배치, `IWeightProvider` 참조

## Replication 권한 원칙

- 서버 권위: 도킹 문 상태, 이동/위치, 산소/HP, 아이템 소유권, 소음 이벤트, 정산/재화, 목표 점수
- 클라이언트: 단순 보간만 (이동), 입력 요청만 (상호작용/구매 — 서버 승인 후 반영)
