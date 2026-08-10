# GoHome 코딩 컨벤션 (개발용)

> `Docs/Dev/` 문서군. 여기 적힌 항목은 [../Design/02_GoHome_기술분석서.md 전제 조건](../Design/02_GoHome_기술분석서.md#전제-조건)에서 이미 팀 결정된 것을 실행 규칙으로 구체화한 것이다.
>
> 참고: [Unreal Engine 스타일 가이드 (Allar, 한글 번역)](https://github.com/ymkim50/ue4-style-guide/blob/master/README_Kor.md) — 아래 "에셋 명명 규칙"·"블루프린트 변수 규칙"은 이 문서를 GoHome 프로젝트 범위에 맞게 축약한 것이다.

## C++ 표준

Epic 공식 C++ 코딩 표준(Epic Coding Standard)을 그대로 따른다.

- 클래스 접두사: `U`(UObject 파생), `A`(Actor 파생), `F`(구조체/일반 클래스), `I`(인터페이스), `E`(Enum), `T`(템플릿)
- **파일명에는 접두사를 붙이지 않는다** (`IDamageable.h`가 아니라 `Damageable.h`). "Add C++ Class" 마법사가 Name 필드 값 앞에 타입별 접두사를 자동으로 붙이므로 Name 필드에는 접두사 없는 베이스 이름만 입력(예: `Damageable`, `WeightProvider`, `Interactable`) — 안 그러면 `IIDamageable`처럼 중복 접두사가 붙는다.
- 헤더(`Public/`)와 구현(`Private/`)을 반드시 분리
- 멤버 변수는 `PascalCase`, `bool`은 `b` + 형용사(`Is` 생략, `bSubmerged` — Epic 원문 예시 `bPendingDestruction`도 동일). 블루프린트 변수도 동일 규칙([블루프린트 변수 규칙](#블루프린트-변수-규칙) 참고)
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
| Data Asset | `DA_` |

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

## 블루프린트 코멘트 박스 색상 규칙

그래프가 길어지는 BP(`BP_Monster` 상태 머신, 레벨 BP 이벤트 그래프)에서 코멘트 박스(`C` 단축키) 색상으로 블록의 성격을 의미론적으로 표시한다. 기본 뼈대는 [Semantic Comment Colors in Blueprint (danjb.com)](https://www.danjb.com/articles/blueprint_colors)의 Success/Info/Warn/Error 4종에 이 프로젝트용 3종(Network / Debug / Tunable)을 추가했다. 톤 원칙: 평소 자주 붙는 코멘트는 채도를 낮춘(subdued) 색, "이거 놓치면 안 된다" 신호를 줘야 하는 카테고리(Error/Network)만 고채도로 남긴다.

| 색상 | 카테고리 | 톤 | 의미 | HSV (H, S, V) | 참고 Hex(불투명 기준) |
|---|---|---|---|---|---|
| 🟩 | Success | subdued | 정상 동작 확인된 완료 블록, 의도적 설계 | (120, 0.4, 0.4) | `#3D663D` |
| 🟦 | Info | subdued | 중립적 설명/참고용 주석 | (220, 0.4, 0.6) | `#5C7099` |
| 🔷 | Tunable *(GoHome 추가)* | subdued | 밸런스 수치(무게/가치/소음 반경, 몬스터 감지 범위 등)처럼 기획 조정을 자주 받는 노출 변수 블록. 코드 담당이 아닌 사람이 그래프를 열었을 때 "여기는 값만 바꿔도 되는 곳"을 바로 찾게 하려는 목적 | (180, 0.4, 0.6) | `#5C9999` |
| 🟧 | Warn | 중간 | 주의해서 다뤄야 하는 블록(원인 불명확한 위험, 재확인이 필요하지만 아래 Network만큼 항상 그런 건 아닌 경우) | (40, 0.6, 0.6) | `#997A3D` |
| ⬛ | Debug *(GoHome 추가)* | subdued(무채색) | 확인·테스트용으로 일부러 넣은 임시 로직(치트, 강제 스폰, 로그 출력 등). Error(알려진 문제)와 성격이 달라 구분한다 — 채도 0이라 "이 블록은 정식 로직이 아니다"가 색만 봐도 드러난다 | (0, 0, 0.35) | `#595959` |
| 🟥 | Error | 고채도(눈에 띄게) | 알려진 문제·미완성·위험한 임시 코드(커밋 전 제거하거나 비활성화 여부 확인) — 그래프에서 정말 놓치면 안 되는 항목이라 원색을 유지 | (0, 1, 0.6) | `#990000` |
| 🟪 | Network *(GoHome 추가)* | 고채도(눈에 띄게) | 서버 권위 판정·리플리케이션이 걸린 블록. 이 프로젝트는 협동 멀티플레이가 핵심이라 "네트워크 관련"이 일반 Warn과 뭉뚱그려지면 정작 중요한 걸 놓친다 — Warn에서 분리하고 Error급으로 눈에 띄게 유지. `Replicated`/`RunOnServer` 계열이 섞인 블록엔 무조건 이 색을 쓴다 | (280, 1, 0.75) | `#8000BF` |

`색상` 열은 표 훑어보기용 이모지일 뿐(이모지는 원색 고정이라 subdued 톤을 표현 못함) — 실제 입력값은 항상 `HSV`/`Hex` 열 기준이다.

알파는 0.2(20% 불투명도)로 통일 — 코멘트 박스가 그 안의 노드를 가리면 안 되기 때문. 색상은 코멘트 박스 Details 패널에서 위 표 값을 그대로 입력하며, 이 표를 단일 출처로 삼는다(에디터 설정 파일로 공유하지 않음). 카테고리를 늘리기 전에 기존 7종으로 표현이 안 되는지 먼저 확인한다.

## Git / Git LFS

- 소스 관리는 Git + Git LFS ([../Design/02_GoHome_기술분석서.md 전제 조건](../Design/02_GoHome_기술분석서.md#전제-조건)에서 미션 가이드 명시로 확정)
- LFS 대상: `.uasset`, `.umap`, 이미지/오디오/비디오 원본 등 바이너리 애셋 전체 (`.gitattributes`에 이미 등록됨)
- 새 팀원은 clone 직후 `git lfs install`을 한 번 실행한다 — 이후 `.gitattributes`에 등록된 확장자는 자동으로 LFS를 통해 받아진다. 새 바이너리 확장자를 추가로 트래킹해야 하면 `.gitattributes`에 `<확장자> filter=lfs diff=lfs merge=lfs -text` 패턴을 추가한다.

## 커밋 전 체크

- 새 클래스를 추가했으면 [ARCHITECTURE.md](ARCHITECTURE.md)의 매핑 표가 여전히 맞는지 확인
- 기획 문서(`Docs/Design/`)를 여는 것은 예외 상황에서만: 유저에게 보이는 동작·밸런스 수치·시스템 범위 자체를 바꾸는 변경일 때만 해당 절을 읽고 어긋나지 않는지 확인한다 — 어긋나면 코드가 아니라 기획서 쪽을 먼저 팀과 조율. ARCHITECTURE.md 스펙대로의 순수 구현 작업(리팩터링, 버그 수정, 이미 확정된 클래스/인터페이스 구현)에는 기획 문서를 열 필요가 없다
- 블루프린트를 수정했으면 경고·오류 없이 컴파일되는지 확인
