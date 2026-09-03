# GoHome UI 구조 가이드 (Blueprint)

> `UI/`는 C++ 베이스 없이 전부 Blueprint(`Content/GoHome/UI/`) — 이 문서가 코드가 강제 못 하는 UI 구조 규칙의 단일 출처다. 개별 위젯 필드/함수는 담지 않고 화면 분할·소유·연출 배치만 고정한다. 표시 전용/입력 위젯의 데이터 방향은 [ARCHITECTURE.md UI 절](ARCHITECTURE.md#ui)이 출처.

## 목차

- [UI 패턴](#ui-패턴)
- [UI 레이어 모델](#ui-레이어-모델)
- [화면 카탈로그](#화면-카탈로그)
- [HUD 묶음 소유 규칙](#hud-묶음-소유-규칙)
- [화면 위젯 규칙](#화면-위젯-규칙)
- [라우터 패턴 (정산 화면)](#라우터-패턴-정산-화면)
- [연출 규칙](#연출-규칙)
- [정산 화면군 상세 스펙 (담당자 개인 문서)](#정산-화면군-상세-스펙-담당자-개인-문서)
- [네이밍](#네이밍)

## UI 패턴

**"뷰는 멍청하게, 상태가 민다" (수동 ViewModel).** 위젯은 폴링하지 않고 델리게이트/복제 프로퍼티를 구독만 하며, 화면마다 `Setup(구조체)` 하나로 데이터를 받는다.

반려:

- **UMG MVVM 플러그인** — 보일러플레이트·학습 비용 대비 이득 없음. 화면 십수 개 미만이면 `Setup(struct)`가 ViewModel 몫을 커버. 상점·강화·도감까지 늘고 게임패드 포커스가 필요해지면 재검토.
- **CommonUI** — 메뉴 계층 깊고 콘솔 대응 필요할 때의 프레임워크. 협동 서바이벌 + 화면 소수엔 러닝커브가 이득을 넘음.

## UI 레이어 모델

성격별 층으로 나눈다. 같은 층은 동시에 하나만.

| 레이어 | 예 | 성격 |
|---|---|---|
| HUD | 산소·나침반(`WBP_Compass`)·HP·무게·상호작용 프롬프트(`WBP_InteractionPrompt`)·위험 비네트(`WBP_DangerVignette`)·인벤토리(`WBP_InventorySlots`)·제한시간·할당량 | 상시, 게임플레이 위. 표시 전용 |
| Modal | 존 선택(`WBP_ZoneSelect`), 강화 상점(미구현) | 게임 위, 입력 잡음. 확정은 Server RPC |
| Fullscreen / Sequence | 정산·게임오버·엔딩(`WBP_Settlement` 계열), 타이틀(`WBP_Title`), 로딩(`WBP_Loading`) | 게임 가림. 자체 enter/exit 연출. 동시에 하나만 |
| System | 세션 진입(`WBP_SessionEntry`), 연결 끊김/에러 | 최상단 |

## 화면 카탈로그

새 화면은 이 표에 한 줄. 소유자 = `Create Widget` + `AddToViewport` 호출 클래스.

| 화면 | 레이어 | 소유자 | 생성/파괴 | 데이터 소스 |
|---|---|---|---|---|
| 정산/게임오버/엔딩 (`WBP_Settlement` 라우터) | Sequence | BP PlayerController | 클라 BeginPlay에 숨겨 생성 + `OnSettlementReady` 바인딩 / 로비 트래블 시 파괴 | `FSettlementResult` (이벤트 인자). 사망자명 = `PlayerState->GetPlayerName()` — 스팀 페르소나가 실리는지 세션 배선 확인, 아니면 로그인 시 `IOnlineIdentity::GetPlayerNickname` → `ServerChangeName` 보정 |
| HUD 묶음 | HUD | BP PlayerController | 탐사 레벨 Possess 후 / 레벨 전환 | GameState 복제 필드 / 컴포넌트 델리게이트 |
| 존 선택 | Modal | 존 셀렉트 모니터 또는 PlayerController | 상호작용 트리거 / 확정·취소 | Zone DataAsset |

## HUD 묶음 소유 규칙

- **상시 HUD 위젯은 BP PlayerController(또는 그 `AHUD`)가 소유. Pawn/Character 소유 금지.** 근거: (1) 데이터가 폰 스코프여도 위젯은 뷰 — `GetOwningPlayerPawn()`으로 읽고 `OnPossessedPawnChanged`에 재바인딩. (2) 사망 시 부활 없이 관전이 수 분 지속되고 그동안도 목표·타이머 HUD가 필요 → 지속 HUD는 필수이고, 폰 소유면 관전 내내 검은 화면 + 별도 관전 HUD를 또 만듦. (3) 폰은 로비 도착마다 재생성되는 소모품이라 폰 소유 위젯은 매 사이클 재생성.
- **per-pawn 패널(HP·산소·무게·인벤토리)은 "폰 없음/사망" 상태를 명시적으로 가짐** — 재바인딩 함수 안 `if` 하나로 패널이 자기 visibility 관리. 토글이 커지면 대안: 캐릭터의 `UHUDPanelComponent`가 `BeginPlay`에 PC HUD NamedSlot로 위젯 주입 / `EndPlay`에 제거 (라이프타임은 폰, 트리는 PC HUD).
- **현황**: HUD 요소가 캐릭터 BP·컨트롤러 BP에 흩어짐. 신규(제한시간·할당량)는 처음부터 PC HUD에 넣어 이주 레퍼런스로 삼고, 기존 요소는 해당 시스템 PR마다 하나씩 이주. 빅뱅 리팩터 금지.
- **데이터**: 제한시간 = `AExplorationGameState::GetRemainingSeconds()` / `HasTimeLimit()` (복제됨). 할당량 = 라이브 복제 소스 없음, `AExplorationGameState`에 `CurrentMapQuota` / `CurrentRoundDelivered` + OnRep 필요 (ARCHITECTURE.md UI 절 known_gaps).
- **배치**: 팀 공유 상태(제한시간·할당량) 상단, per-player(HP·산소·무게) 하단. 상세는 담당자 개인 문서.

## 화면 위젯 규칙

각 Sequence/Modal 화면은 **자기완결 유닛** — 자기 UMG 애니메이션, `Setup(구조체)` / (필요 시) `Teardown()`, 자기 연출 시퀀스를 소유한다. `Setup` 밖에서 게임 상태를 조회하지 않는다. 라우터/부모는 **어느 화면을 켤지 + 데이터 전달**만 하고 연출 로직을 갖지 않는다.

## 라우터 패턴 (정산 화면)

`WBP_Settlement`는 화면이 아니라 라우터. `OnSettlementReady(FSettlementResult Result)`:

```
if Result.bForfeited:
    → 임무 실패 페이지 (정산표 없음)
    if Result.Outcome ∈ {GameOver_Strike, GameOver_CheckPoint, Ending}:
        임무 실패 페이지 hold 끝 → 같은 게임오버/엔딩 페이지로 자동 전환
else switch Result.Outcome:
    Normal / CheckPointPassed             → 정산표 페이지
    GameOver_Strike / GameOver_CheckPoint → 게임오버 페이지
    Ending                                → 엔딩 페이지
켜는 페이지마다 Setup(Result) → 라우터 Visible → 페이지 인트로 연출 → (연출 끝난 뒤) 자동복귀 카운트다운
```

`FSettlementResult` 하나가 `bForfeited`와 `Outcome`을 둘 다 싣고 `DetermineOutcome`은 forfeit와 무관하게 돈다(`GoHomeSaveSubsystem.cpp`) → forfeit→게임오버 연쇄에 추가 이벤트 불필요, 게임오버 페이지는 정상복귀·forfeit 경로 공용. forfeit인데 `Outcome`이 terminal 아니면 임무 실패 페이지만 뜨고 로비로.

**데이터 갭 — 체크포인트 레일**: 정산표가 상시 표시하는 진행도 레일(턴 3/6/9 관문)은 전체 스케줄이 필요하나 `FSettlementResult` / `FExpeditionProgress`는 다음(`NextCheckPoint*`)·이번(`CheckPointQuota`) 것만 싣는다. 전체는 `UEconomyConfigDataAsset::CheckPoints`(`{Round, TargetQuota}[]`)에만 있음 → 구조체에 배열 복사 또는 GameState가 config 참조 노출, 미정.

**페이지 인스턴스화**: `WidgetSwitcher` 말고 **on-demand 생성**(`NamedSlot` + `Create Widget` → `SetContent`). 세션당 한 페이지만 뜨고 각자 인트로가 있어 안 쓸 페이지를 미리 Construct할 이유가 없다. 재검토 트리거:

- 화면군을 **왕복 토글**하기 시작하면 그 부분만 `WidgetSwitcher`.
- 한 페이지에 **수십 행 반복 항목**이 생기면 커스텀 풀이 아니라 UMG `ListView`/`TileView`. 풀스크린 화면 자체는 세션당 1회 생성이라 위젯 풀링 대상이 아니다 — 무거워지는 건 텍스처/연출이지 위젯 수가 아님.
- 페이지가 독립 위젯 + `Setup` 계약이라 라우터의 생성/전환 방식만 바꾸면 됨(페이지 내부 불변). 이행 비용 낮아 지금 안 만듦.

**클라 바인딩 타이밍**: 클라는 `OnSettlementReady`(→ `OnRep_SettlementResult`) 전에 바인딩이 살아있어야 한다. PlayerController BeginPlay에 `AExplorationGameState`가 null일 수 있어 유효화 대기 가드 필요. state 델리게이트가 아니라 `OnSettlementReady`에 바인딩 — CurrentState/SettlementResult OnRep 순서 미보장(ARCHITECTURE.md Save 절 "정산 결과 복제").

**자동복귀 카운트다운**: `AExplorationGameMode::AutoReturnDelay`(현재 8초)는 서버 전용·복제 안 됨. `GetRemainingSeconds()` / `ExpeditionDeadline`은 탐사 제한시간 전용이라 재사용 불가 → 위젯이 클라 로컬 타이머로 카운트다운(트래블은 서버 권한, 드리프트 무해). `AutoReturnDelay` 값만 BP 노출.

- **페이지 연출이 전부 끝난 뒤 시작** (`OnSettlementReady` 수신 즉시가 아님) — 연출 중엔 바 꽉 참 + 숫자 라벨 고정. 카운트다운 시작 = "이제 읽고 나갈 시간".
- **스킵 없음** — 입력 경로 없음. 타이머 만료 = 유일한 복귀 트리거.
- **모든 Outcome은 로비로 복귀** — `ReturnToLobby` → `ServerTravelViaLoadingScreen(로비맵)` 공통. 게임오버/엔딩은 `FinalizeRound`가 `ResetSave()`한 상태로 도착 → 로비가 새 런 허브. "타이틀로" 버튼 없음.
- **forfeit 2페이지는 `AutoReturnDelay` 안에 들어가야** 함 — 두 hold 합이 넘으면 트래블이 두 번째 페이지를 자른다. hold를 튜너블로 두고 필요 시 `AutoReturnDelay` 상향.

## 연출 규칙

우선순위: (1) UMG Widget Animation — 대부분의 트랜지션. (2) 위젯 BP 소규모 시퀀스(Delay/Timeline) — "카운트업 → 카운터 갱신 → outro" 같은 화면 내부 순서. (3) 화면 간 연출(정산 → 로비 페이드) — 라우터/상위 레이어 소유, 개별 페이지에 두지 않음.

## 정산 화면군 상세 스펙 (담당자 개인 문서)

페이지별 `FSettlementResult` 매핑, UMG 구현 매핑, 연출 타이밍, 비주얼 아이덴티티는 정산 UI 담당의 개인 문서(`Docs/Dev/_personal/settlement-ui-spec.md`, gitignore). 담당 이관 시 승계. 팀 계약(라우터 패턴·바인딩 대상·로비 복귀·forfeit 연쇄)은 위 [라우터 패턴](#라우터-패턴-정산-화면) 절.

## 네이밍

- `WBP_<화면>` — 화면/라우터 (`WBP_Settlement`, `WBP_Title`).
- `WBP_<화면>_<파트>` 또는 `WBP_<파트>` — 재사용 조각 (`WBP_CasualtyRow`, `WBP_ZoneButton`).
- 폴더는 화면군 단위 (`Content/GoHome/UI/Compass/`).
