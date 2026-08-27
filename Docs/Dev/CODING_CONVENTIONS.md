# GoHome 코딩 컨벤션 (개발용)

> `Docs/Dev/` 문서군. GoHome 기술분석서(Notion) "전제 조건" 절에서 팀 결정된 것을 실행 규칙으로 구체화한 것이다. 에셋/변수 명명 규칙은 [Unreal Engine 스타일 가이드(Allar, 한글)](https://github.com/ymkim50/ue4-style-guide/blob/master/README_Kor.md)를 GoHome 범위로 축약.

## C++ 표준

Epic 공식 C++ 코딩 표준(Epic Coding Standard)을 그대로 따른다. 이 프로젝트 특이사항만:

- **파일명에는 접두사를 붙이지 않는다** (`IDamageable.h`가 아니라 `Damageable.h`). "Add C++ Class" 마법사가 Name 필드 값 앞에 타입별 접두사를 자동으로 붙이므로 Name 필드에는 베이스 이름만 입력(예: `Damageable`) — 안 그러면 `IIDamageable`처럼 접두사가 중복된다.
- `bool`은 `b` + 형용사(`Is` 생략, 예: `bReloading`) — [블루프린트 변수 규칙](#블루프린트-변수-규칙)도 동일 규칙을 따른다.
- `UPROPERTY`/`UFUNCTION` 매크로에 `BlueprintReadOnly`/`BlueprintReadWrite`/`Replicated` 등 의도를 명시적으로 표기(기본값 의존 금지) — 자동 검증 수단 없음, 코드 리뷰로 확인

## OOP/SOLID 적용 방식

객체지향/SOLID 원칙은 지키되, 순수 C++ 패턴이 아니라 언리얼이 이미 제공하는 대체 수단으로 구현한다:

- 옵저버 패턴 → 델리게이트(`DECLARE_DYNAMIC_MULTICAST_DELEGATE`)
- 상태 동기화 → Replication/RepNotify (수동 폴링 금지)
- 횡단 관심사 분리 → Actor Component
- 전역/생명주기 서비스 → Subsystem(`UGameInstanceSubsystem` 등), 수동 싱글톤 금지
- 타입 무관 계약 → `UINTERFACE`/`IInterface`

## 에셋 명명 규칙

콘텐츠 브라우저 에셋은 `Prefix_BaseAssetName_Suffix` 형식을 따른다. 이 프로젝트에서 실제로 쓰는 접두사만 추린 목록:

| 에셋 타입 | 접두사 |
|---|---|
| Blueprint | `BP_` |
| Static Mesh | `SM_` |
| Skeletal Mesh | `SK_` |
| Material | `M_` |
| Material Instance | `MI_` |
| Texture | `T_` |
| Widget Blueprint | `WB_` |
| Animation Sequence | `AS_` |
| Behavior Tree | `BT_` |
| Data Asset | `DA_` |

AI 컨트롤러(`AIC_`), 파티클 시스템(`PS_`) 등 위 표에 없는 타입을 실제로 쓰게 되면 그때 이 표에 추가한다.

텍스처는 용도별 접미사를 붙인다: 확산/알베도 `_D`, 노멀 `_N`, 러프니스 `_R`, 알파 `_A`, AO `_AO`.

## 폴더 규칙

### Source (C++)

기능별 분리, [ARCHITECTURE.md](ARCHITECTURE.md)의 폴더 구조를 그대로 따른다: `Public`/`Private`을 `Source/GoHome/` 최상위에 두고, 그 아래 각 기능 폴더(`Core`/`Player`/`Interaction`/`AI`/`Item`/`UI`/`Save`)를 중첩한다(예: `Public/Core/`, `Private/Core/`).

- 새 시스템이 기존 7개 폴더([ARCHITECTURE.md 모듈/폴더 구조](ARCHITECTURE.md#모듈폴더-구조) 기준) 중 어디에도 안 맞으면, 새 폴더를 만들기 전에 그 문서에 먼저 매핑을 기록한다 (폴더 구조의 단일 출처는 그 문서).
- 기존 폴더에 기술적으로 끼워 넣을 수 있어도 새 폴더로 분리하는 게 나은 경우의 기준은 [ARCHITECTURE.md "새 폴더를 파야 하는가"](ARCHITECTURE.md#새-폴더를-파야-하는가) 참고.
- 폴더를 넘나드는 의존은 인터페이스(`IInteractable`, `IWeightProvider`류)로만 한다 — 컴포넌트가 다른 폴더의 구체 클래스를 직접 include하지 않는다. 단, `AGoHomeGameState`처럼 여러 폴더가 공유하는 클래스는 예외 — 퍼블릭 서버 함수(`AddDeliveredValue` 등) 호출 목적의 include는 허용한다([ARCHITECTURE.md 헤더 소유권](ARCHITECTURE.md#헤더-소유권) 참고).

### Content (에셋)

`Content/GoHome/` 아래는 위 Source의 7개 기능 폴더(`Core`/`Player`/`Interaction`/`AI`/`Item`/`UI`/`Save`)를 기본으로 하되, 아래처럼 여러 기능 폴더가 공유하는 애셋 타입은 별도 폴더로 분리한다 — 무리하게 7개에 끼워 맞추지 않는다:

- `Maps/`: 모든 레벨(기획서 맵별로 하위 폴더 분리 가능)
- `MaterialLibrary/`: 여러 기능 폴더에서 공유하는 마스터 머티리얼/유틸리티만
- `Animation/`, `Data/`, `FX/`, `GameMode/`, `GameObject/`, `Input/`, `Sound/`: 타입별 공유 애셋 폴더(현재 저장소에 이미 존재). 특정 기능 폴더 전용 에셋이면 해당 기능 폴더 아래로 넣는다 — 여기는 여러 시스템이 공유하거나 분류상 애매한 것만
- `Developers/<이름>/`: 개인 실험용 샌드박스. 완성된 에셋은 반드시 해당 기능 폴더로 옮긴다 — 다른 시스템이 직접 참조하지 않는다
- 폴더명은 `PascalCase`, 공백·특수문자 금지

## C++ / Blueprint 경계

**C++ 위주** — Blueprint는 다음 용도로만 제한한다:

- 데이터 애셋 인스턴스(아이템 종별 무게/가치/파손·소음 플래그 등 수치 데이터)
- AI 상태 머신의 비주얼 구성 (몬스터 BP — `BP_UnderwaterMonster`/`BP_WormBase`/`BP_Bloop` 등, 기술분석서(Notion)가 이미 BP 기반으로 명시)
- 레벨 블루프린트 (트리거 배치, 연출용 이벤트 그래프)
- 이펙트/애니메이션 블루프린트

게임플레이 로직(리플리케이션, 서버 권위 판정, 상태 머신 핵심 전이)은 C++로 작성한다. BP에서 C++ 로직을 오버라이드해야 하면 `BlueprintNativeEvent`/`BlueprintImplementableEvent`로 확장 지점을 명시적으로 연다.

### AI(몬스터 BP) 작업 방식

AI는 다른 시스템보다 BP 비중이 크지만, 필요하면 담당자가 C++ 쪽도 직접 고칠 수 있다.

- `AUnderwaterEnemyBase`(C++)가 경계 인터페이스([ARCHITECTURE.md "AI (경계만)"](ARCHITECTURE.md#ai-경계만))를 `BlueprintNativeEvent`로 노출하고, 그 자식 BP인 `BP_UnderwaterMonster`가 상태 머신·Steering Behaviors를 구현한다. **주의**: `BP_WormBase`(`AActor` 직속)·`BP_Bloop`(`APawn` 직속)는 `AUnderwaterEnemyBase`를 상속하지 않아 이 경계 인터페이스가 적용되지 않는다 — 신규 몬스터를 이 경계에 태우려면 `AUnderwaterEnemyBase` 상속부터 확인([ARCHITECTURE.md "AI (경계만)"](ARCHITECTURE.md#ai-경계만) known_gaps 참고).
- 데미지 적용·소음 판정처럼 서버 권위가 걸린 처리는 BP가 직접 구현하지 않고 C++ 함수 호출로 끝낸다.
- 상태 머신을 Behavior Tree + Blackboard로 구성해도 무방(기술분석서(Notion)의 "Enum 상태 머신 + Steering Behaviors"와 병행 가능).
- 경계 인터페이스 시그니처 변경은 C++ 담당과 사전 협의([ARCHITECTURE.md 소유권 규칙](ARCHITECTURE.md#헤더-소유권)).

## 블루프린트 변수 규칙

위 경계에서 BP가 실제로 맡는 범위(데이터 애셋, `BP_Monster` 상태 머신, 레벨 BP, 이펙트/애니메이션 BP)에 한해 적용한다.

- `PascalCase`, 서술적 명사 사용
- `bool`은 [C++ 표준](#c-표준)과 동일 (예: `bReloading`)
- 배열은 복수형 명사 (`Targets`, `TargetArray` 금지)
- 상태가 3개 이상이면 bool 여러 개 대신 Enum 하나로 표현 (몬스터 BP 상태 머신이 대표 사례)
- Editable(에디터에 노출) 변수는 Tooltip 필수, 값 범위가 있으면 Slider/Value Range 설정
- 변수가 10개를 넘으면 카테고리를 `|`로 하위 분류 (예: `Config | Noise`)
- 경고·오류 없이 컴파일되는 상태로만 커밋한다 — 깨진 블루프린트를 소스 컨트롤에 올리지 않는다

## 블루프린트 코멘트 박스 색상 규칙

블루프린트 그래프에 코멘트 박스를 넣을 때만 참고 — [BP_COMMENT_COLORS.md](BP_COMMENT_COLORS.md) 참고.

## Git / Git LFS

- 소스 관리는 Git + Git LFS
- LFS 대상: `.uasset`, `.umap`, 이미지/오디오/비디오 원본 등 바이너리 애셋 전체 (`.gitattributes`에 이미 등록됨)
- 새 팀원은 clone 직후 `git lfs install`을 한 번 실행한다 — 이후 `.gitattributes`에 등록된 확장자는 자동으로 LFS를 통해 받아진다. 새 바이너리 확장자를 추가로 트래킹해야 하면 `.gitattributes`에 `<확장자> filter=lfs diff=lfs merge=lfs -text` 패턴을 추가한다.

### 바이너리(`.uasset`/`.umap`) 충돌 시 CLI로 한쪽 선택하기

브랜치 생성/머지 같은 일상 절차는 GoHome Git 워크플로우(Notion)가 사람이 GitHub Desktop으로 따라가는 절차서다. 이 절만 여기 두는 이유: 머지 충돌이 나면 보통 그 자리에서 바로 도움이 필요하고, 에이전트가 실시간으로 커맨드를 알려줘야 하는 상황이라 md로 직접 참조 가능해야 한다.

`.uasset`/`.umap` 같은 Git LFS 바이너리는 텍스트 코드처럼 "부분 병합"이 안 된다. GitHub Desktop은 충돌 사실만 알려줄 뿐 자동으로 합쳐주지 못하므로, 터미널(명령 프롬프트/PowerShell)에서 어느 쪽 파일을 쓸지 직접 골라야 한다.

git은 병합할 때 지금 서 있는 브랜치를 **ours(우리 것)**, 병합해 들어오는 브랜치를 **theirs(상대 것)**라고 부른다. "develop 걸로 고르고 싶다"고 해도 지금 어느 브랜치에 있느냐에 따라 명령어가 달라진다.

**상황 A: 내 브랜치(`csk`)에서 `develop`을 병합할 때**

```
git checkout csk
git merge develop
```

지금 서 있는 브랜치가 `csk`이므로 `csk`가 ours, `develop`이 theirs다. develop 쪽 파일을 쓰고 싶으면 **theirs**:

```
git checkout --theirs Content/GoHome/AI/Enemy/Fish/BP_UnderwaterMonster.uasset
git add Content/GoHome/AI/Enemy/Fish/BP_UnderwaterMonster.uasset
git commit
```

**상황 B: `develop`에서 내 브랜치(`csk`)를 병합할 때**

```
git checkout develop
git merge csk
```

이번엔 `develop`이 ours, `csk`가 theirs로 뒤바뀐다. 여전히 develop 쪽을 쓰고 싶으면 이번엔 **ours**:

```
git checkout --ours Content/GoHome/AI/Enemy/Fish/BP_UnderwaterMonster.uasset
git add Content/GoHome/AI/Enemy/Fish/BP_UnderwaterMonster.uasset
git commit
```

**지켜야 할 것**: 무조건 develop을 고르는 게 정답이 아니다 — 두 파일 중 어느 쪽이 최신 작업인지 먼저 확인하고 고른다(확실하지 않으면 해당 파일 작업자에게 먼저 물어본다). 병합이 끝나면 `git diff develop HEAD -- "*.uasset" "*.umap"`로 의도한 파일만 의도한 방향으로 바뀌었는지 확인한 뒤에 push한다.

## 구현 완료 시 자가검토 (에이전트 기준)

**구현을 마친 에이전트가 사람에게 결과를 넘기기 전에** 직접 수행하는 검토다(팀원 본인이 짠 경우도 동일):

- [OOP/SOLID 적용 방식](#oopsolid-적용-방식) 위반 없는지
- [C++ / Blueprint 경계](#c--blueprint-경계) 위반 없는지: 서버 권위가 걸린 로직이 BP에만 있고 C++ 호출 없이 끝나지 않았는지
- 폴더 간 경계(인터페이스 시그니처, decoupling 규칙)를 바꿨으면 [ARCHITECTURE.md](ARCHITECTURE.md)의 "시스템별 결정과 경계"·"시스템 간 인터페이스 계약"이 여전히 맞는지
- 기획 문서(GoHome 기획서·기술분석서, Notion)는 예외 상황에서만 연다: 유저에게 보이는 동작·밸런스 수치·시스템 범위 자체를 바꾸는 변경일 때만 해당 절을 확인하고 어긋나지 않는지 본다 — 어긋나면 코드가 아니라 기획서 쪽을 먼저 팀과 조율. ARCHITECTURE.md 스펙대로의 순수 구현 작업(리팩터링, 버그 수정, 이미 확정된 클래스/인터페이스 구현)에는 열 필요 없다
- C++ 빌드 경고 0개, 블루프린트는 경고·오류 없이 컴파일되는 상태

