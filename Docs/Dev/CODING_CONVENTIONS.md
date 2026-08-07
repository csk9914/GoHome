# GoHome 코딩 컨벤션 (개발용)

> `Docs/Dev/` 문서군. 여기 적힌 항목은 [../Design/02_GoHome_기술분석서.md 전제 조건](../Design/02_GoHome_기술분석서.md#전제-조건)에서 이미 팀 결정된 것을 실행 규칙으로 구체화한 것이다.
>
> 참고: [Unreal Engine 스타일 가이드 (Allar, 한글 번역)](https://github.com/ymkim50/ue4-style-guide/blob/master/README_Kor.md) — 아래 "에셋 명명 규칙"·"블루프린트 변수 규칙"은 이 문서를 GoHome 프로젝트 범위에 맞게 축약한 것이다.

## C++ 표준

Epic 공식 C++ 코딩 표준(Epic Coding Standard)을 그대로 따른다.

- 클래스 접두사: `U`(UObject 파생), `A`(Actor 파생), `F`(구조체/일반 클래스), `I`(인터페이스), `E`(Enum), `T`(템플릿)
- **파일명에는 접두사를 붙이지 않는다** (`IDamageable.h`가 아니라 `Damageable.h`). "Add C++ Class" 마법사가 Name 필드 값 앞에 타입별 접두사를 자동으로 붙이므로, Name 필드에는 항상 접두사 없는 베이스 이름만 입력할 것(예: `Damageable`, `WeightProvider`, `Interactable`) — 이미 접두사가 붙은 이름을 입력하면 `IIDamageable`처럼 중복 접두사가 붙는다.
- 헤더(`Public/`)와 구현(`Private/`)을 반드시 분리
- 멤버 변수는 `PascalCase`, `bool`은 `b` + 형용사(`Is` 생략, `bSubmerged`) — Epic 원문 예시(`bPendingDestruction`, `bHasFadedIn`)도 `Is`를 붙이지 않는 쪽이라 이를 따름. 블루프린트 변수도 동일 규칙([블루프린트 변수 규칙](#블루프린트-변수-규칙) 참고)
- 함수는 `PascalCase`, 매개변수/지역 변수는 `PascalCase` (Epic 표준 그대로, camelCase 아님)
- `UPROPERTY`/`UFUNCTION` 매크로에 `BlueprintReadOnly`/`BlueprintReadWrite`/`Replicated` 등 의도를 명시적으로 표기 (기본값에 의존하지 않음)

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

AI 컨트롤러(`AIC_`), 파티클 시스템(`PS_`) 등 위 표에 없는 타입을 실제로 쓰게 되면 그때 이 표에 추가한다.

텍스처는 용도별 접미사를 붙인다: 확산/알베도 `_D`, 노멀 `_N`, 러프니스 `_R`, 알파 `_A`, AO `_AO`.

## 폴더 규칙

### Source (C++)

기능별 분리, [ARCHITECTURE.md](ARCHITECTURE.md)의 폴더 구조를 그대로 따른다: `Public`/`Private`을 `Source/GoHome/` 최상위에 두고, 그 아래 각 기능 폴더(`Core` / `Player` / `Interaction` / `AI` / `Item` / `UI` / `Save`)를 중첩한다(예: `Public/Core/`, `Private/Core/`).

- 새 시스템이 기존 7개 폴더 중 어디에도 안 맞으면, 새 폴더를 만들기 전에 [ARCHITECTURE.md](ARCHITECTURE.md)에 먼저 매핑을 기록한다 (폴더 구조의 단일 출처는 그 문서).
- 폴더를 넘나드는 의존은 인터페이스(`IInteractable`, `IWeightProvider`류)로만 한다 — 컴포넌트가 다른 폴더의 구체 클래스를 직접 include하지 않는다.

### Content (에셋)

`Content/GoHome/` 아래를 위 Source와 동일한 7개 기능 폴더로 맞춘다 — 코드-에셋 대응이 바로 보이도록 함. 그 외:

- `Maps/`: 모든 레벨을 여기 모은다 (기획서 맵별로 하위 폴더 분리 가능)
- `MaterialLibrary/`: 여러 기능 폴더에서 공유하는 마스터 머티리얼/유틸리티만
- `Developers/<이름>/`: 개인 실험용 샌드박스. 완성된 에셋은 반드시 해당 기능 폴더로 옮긴다 — `Developers/` 안의 에셋을 다른 시스템이 직접 참조하지 않는다
- 폴더명은 `PascalCase`, 공백·특수문자 금지

## C++ / Blueprint 경계

**C++ 위주** (`Docs/Design/02_GoHome_기술분석서.md` 전제 조건에서 팀 결정), Blueprint는 다음 용도로만 제한한다:

- 데이터 애셋 인스턴스(아이템 종별 무게/가치/파손·소음 플래그 등 수치 데이터)
- AI 상태 머신의 비주얼 구성 (`BP_Monster` — 02문서가 이미 BP 기반으로 명시)
- 레벨 블루프린트 (트리거 배치, 연출용 이벤트 그래프)
- 이펙트/애니메이션 블루프린트

게임플레이 로직(리플리케이션, 서버 권위 판정, 상태 머신 핵심 전이)은 C++로 작성한다. BP에서 C++ 로직을 오버라이드해야 하면 `BlueprintNativeEvent`/`BlueprintImplementableEvent`로 확장 지점을 명시적으로 연다.

### AI(BP_Monster) 작업 방식

AI는 워크플로 선택으로 다른 시스템보다 BP 비중이 크지만, 필요하면 AI 담당도 C++ 쪽을 직접 고칠 수 있다.

- `AMonsterBase`(C++)가 경계 인터페이스([ARCHITECTURE.md "AI (경계만)"](ARCHITECTURE.md#ai-경계만) 참고)를 `BlueprintNativeEvent`로 노출하고, `BP_Monster`(`AMonsterBase` 자식 BP)에서 상태 머신·Steering Behaviors를 구현한다.
- 데미지 적용·소음 판정·상태 브로드캐스트처럼 서버 권위가 걸린 처리는 BP가 직접 구현하지 않고 C++ 함수 호출로 끝낸다 — 위 "C++ / Blueprint 경계" 절의 원칙이 AI에도 동일하게 적용된다.
- 상태 머신을 Behavior Tree + Blackboard로 구성해도 무방하다(권장일 뿐, 02문서의 "Enum 상태 머신 + Steering Behaviors" 결정을 대체하는 게 아니라 병행 가능한 구현 수단).
- 경계 인터페이스 시그니처 변경이 필요하면 C++ 담당과 사전 협의한다 — 소유권 규칙은 [ARCHITECTURE.md "소유권 규칙"](ARCHITECTURE.md#병렬-착수를-위한-헤더-스텁과-소유권) 참고.

## 블루프린트 변수 규칙

위 경계에서 BP가 실제로 맡는 범위(데이터 애셋, `BP_Monster` 상태 머신, 레벨 BP, 이펙트/애니메이션 BP)에 한해 적용한다.

- `PascalCase`, 서술적 명사 사용
- `bool`은 `b` + 형용사만 (C++와 동일 규칙, `Is` 생략 — 예: `bReloading`)
- 배열은 복수형 명사 (`Targets`, `TargetArray` 금지)
- 상태가 3개 이상이면 bool 여러 개 대신 Enum 하나로 표현 (`BP_Monster` 상태 머신이 대표 사례)
- Editable(에디터에 노출) 변수는 Tooltip 필수, 값 범위가 있으면 Slider/Value Range 설정
- 변수가 10개를 넘으면 카테고리를 `|`로 하위 분류 (예: `Config | Noise`)
- 경고·오류 없이 컴파일되는 상태로만 커밋한다 — 깨진 블루프린트를 소스 컨트롤에 올리지 않는다

## Git / Git LFS

- 소스 관리는 Git + Git LFS ([../Design/02_GoHome_기술분석서.md 전제 조건](../Design/02_GoHome_기술분석서.md#전제-조건)에서 미션 가이드 명시로 확정)
- LFS 대상: `.uasset`, `.umap`, 이미지/오디오/비디오 원본 등 바이너리 애셋 전체 (`.gitattributes`에 이미 등록됨)
- 새 팀원은 clone 직후 `git lfs install`을 한 번 실행한다 — 이후 `.gitattributes`에 등록된 확장자는 자동으로 LFS를 통해 받아진다. 새 바이너리 확장자를 추가로 트래킹해야 하면 `.gitattributes`에 `<확장자> filter=lfs diff=lfs merge=lfs -text` 패턴을 추가한다.

## 커밋 전 체크

- 새 클래스를 추가했으면 [ARCHITECTURE.md](ARCHITECTURE.md)의 매핑 표가 여전히 맞는지 확인
- 기획 문서(`Docs/Design/`)의 스펙을 확인해야 하는 변경이면 해당 절을 먼저 읽고 구현이 스펙과 어긋나지 않는지 확인 — 어긋나면 코드가 아니라 기획서 쪽을 먼저 팀과 조율
- 블루프린트를 수정했으면 경고·오류 없이 컴파일되는지 확인
