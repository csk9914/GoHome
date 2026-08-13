    # GoHome GitHub Desktop 사용 가이드 & 워크플로우 (개발용)

> `Docs/Dev/` 문서군. Git 저장소 구조/네이밍 규칙 자체는 [CODING_CONVENTIONS.md "Git / Git LFS"](CODING_CONVENTIONS.md#git--git-lfs)를 따르며, 이 문서는 그 위에서 팀이 실제로 브랜치를 만들고 머지하는 **작업 절차**만 다룬다.

## 1. 브랜치 전략

- **원칙: 당일 작업 당일 머지.** 브랜치 수명은 하루를 넘기지 않는다 — 여러 날 걸리면 끝난 만큼 머지하고 다음 날 새 브랜치를 판다.
- 브랜치 흐름: `develop`에서 **작업자 이니셜 브랜치** 생성 → 작업 → **당일 머지** → **브랜치 삭제**.
- 브랜치 이름은 작업자 이니셜 기준(예: `csk`, `csk-inventory`) — 이니셜이 겹치면 뒤에 구분자를 붙인다.
- `develop` 브랜치가 없다면 최초 1회 `main`에서 만들고 이후 모든 작업 브랜치는 `develop`에서 분기한다. `main`은 배포/제출용 안정 버전만 유지, 직접 커밋하지 않는다.

## 2. 풀 리퀘스트(PR) 도입 시점

- **지금은 PR을 쓰지 않는다.** 브랜치에서 작업 후 바로 `develop`으로 머지한다.
- **도입 시점**: 착수 후 약 1개월, 팀원 전원이 GitHub 사용 방식에 익숙해졌을 때 팀 협의로 전환한다. 정확한 시점·조건은 Notion "GoHome 착수 로드맵"이 단일 출처.
- PR 도입 이후 절차(리뷰어 지정, 승인 규칙 등)는 전환 시점에 이 문서에 추가한다.

## 3. GitHub Desktop 절차 (클릭 단위)

### 3-1. 시작 전 1회만: `develop` 최신화

1. GitHub Desktop 상단 **Current Branch**를 클릭 → `develop` 선택.
2. 상단 **Fetch origin** 클릭 → 원격에 새 커밋이 있으면 **Pull origin** 클릭해 받아온다.

### 3-2. 작업자 이니셜 브랜치 생성

1. `develop`이 최신 상태인지 확인한 뒤, 상단 **Current Branch** → **New Branch** 클릭.
2. Name 입력창에 자신의 이니셜(필요하면 `-작업내용` 붙여서) 입력.
3. **Create Branch** 클릭 — 자동으로 이 새 브랜치로 전환된다. 이 브랜치는 반드시 `develop`을 base로 생성되어 있어야 한다(하단에 "based on develop"으로 표시되는지 확인).

### 3-3. 작업 & 커밋

1. Unreal Editor / 코드 에디터에서 작업한다.
2. 작업이 끝나면 GitHub Desktop 좌측 **Changes** 탭에서 변경된 파일 목록을 확인한다.
3. 커밋에 포함할 파일만 체크(임시 파일·개인 설정 파일은 해제).
4. 하단 Summary(필수)에 아래 "커밋 메시지 규칙"대로 짧게 적는다. 필요하면 Description에 상세 내용 추가.
5. **Commit to `<이니셜 브랜치>`** 클릭.
6. 하루 안에 여러 번 커밋해도 무방하다 — 마지막에 한 번에 머지한다.

#### 커밋 메시지 규칙

Summary는 `태그: 내용` 형식으로 적는다 (예: `기능: 인벤토리 UI 추가`, `수정: 산소 게이지 감소 버그 수정`). 태그는 아래 중 하나만 쓴다.

| 태그 | 의미 |
|---|---|
| `기능` | 새 기능/시스템 추가 |
| `수정` | 버그 수정 |
| `변경` | 기존 동작·수치 조정 (버그는 아님) |
| `에셋` | 아트/사운드 등 리소스 추가·교체 |
| `문서` | `Docs/` 문서 수정 |
| `기타` | 위에 안 맞는 잡일(정리, 설정 변경 등) |

- "왜"가 자명하지 않을 때만 Description에 이유를 덧붙인다.
- 새 태그가 필요해 보이면 임의로 만들지 말고 팀과 상의해 이 표에 추가한다.

### 3-4. 원격에 올리기

1. 상단 **Push origin** 클릭 (최초 push 시 버튼이 **Publish branch**로 표시됨 — 그대로 클릭).
2. Git LFS 대상 파일(`.uasset`, `.umap` 등)이 섞여 있으면 push가 오래 걸릴 수 있다 — 완료될 때까지 기다린다.

### 3-5. 당일 머지 (`develop`으로)

1. 상단 **Current Branch** → `develop`으로 전환.
2. 상단 **Fetch origin** → 그 사이 다른 팀원이 `develop`에 먼저 머지했다면 **Pull origin**으로 받아온다.
3. 메뉴 **Branch → Merge into current branch...** 선택.
4. 목록에서 자신의 이니셜 브랜치를 선택 → **Merge `<이니셜 브랜치>` into develop** 클릭.
5. 충돌(conflict)이 뜨면 안내된 파일을 열어 직접 해결한다 — 애매하면 임의로 지우지 말고 해당 파일 작업자와 먼저 상의한다. `.uasset`/`.umap` 등 바이너리 충돌은 병합이 불가능하므로, 같은 에셋을 여러 명이 동시에 건드리지 않는 것으로 예방한다.
6. 머지 완료 후 **Push origin**을 클릭해 원격에 올린다.

### 3-6. 브랜치 삭제

1. `develop`으로 정상 머지·push까지 끝난 것을 확인한다.
2. 상단 **Current Branch** 클릭 → 목록에서 자신의 이니셜 브랜치에 마우스를 올리면 나오는 휴지통 아이콘(또는 우클릭 → **Delete...**) 클릭.
3. "Delete branch on GitHub.com/원격" 체크박스가 있으면 함께 체크해 로컬·원격 브랜치를 한 번에 정리한다.
4. 다음 작업을 시작할 때는 3-1부터 다시 반복한다.

## 4. 바이너리(`.uasset`/`.umap`) 충돌 시 CLI로 한쪽 선택하기

3-5의 5번에서 말한 것처럼 `.uasset`/`.umap` 같은 Git LFS 바이너리는 텍스트 코드처럼 "부분 병합"이 안 된다. GitHub Desktop은 충돌 사실만 알려줄 뿐 자동으로 합쳐주지 못하므로, **터미널(명령 프롬프트/PowerShell)에서 어느 쪽 파일을 쓸지 직접 골라줘야** 한다.

git은 병합할 때 지금 서 있는 브랜치를 **ours(우리 것)**, 병합해 들어오는 브랜치를 **theirs(상대 것)**라고 부른다. "develop 걸로 고르고 싶다"고 해도 지금 어느 브랜치에 있느냐에 따라 명령어가 달라진다.

### 상황 A: 내 브랜치(`csk`)에서 `develop`을 병합할 때

```
git checkout csk
git merge develop
```
지금 서 있는 브랜치가 `csk`이므로 `csk`가 ours, `develop`이 theirs다. develop 쪽 파일을 쓰고 싶으면 **theirs**:

```
git checkout --theirs Content/GoHome/AI/Enemy/BP_UnderwaterMonster.uasset
git add Content/GoHome/AI/Enemy/BP_UnderwaterMonster.uasset
git commit
```

### 상황 B: `develop`에서 내 브랜치(`csk`)를 병합할 때

```
git checkout develop
git merge csk
```
이번엔 `develop`이 ours, `csk`가 theirs로 뒤바뀐다. 여전히 develop 쪽을 쓰고 싶으면 이번엔 **ours**:

```
git checkout --ours Content/GoHome/AI/Enemy/BP_UnderwaterMonster.uasset
git add Content/GoHome/AI/Enemy/BP_UnderwaterMonster.uasset
git commit
```

### 지켜야 할 것

- **무조건 develop을 고르는 게 정답이 아니다.** 두 파일 중 어느 쪽이 최신 작업인지 먼저 확인하고 고른다 — 확실하지 않으면 해당 파일 작업자에게 먼저 물어본다(3-5의 5번과 동일).
- 병합이 끝나면 결과를 반드시 확인한다:
  ```
  git diff develop HEAD -- "*.uasset" "*.umap"
  ```
  의도한 파일만, 의도한 방향으로 바뀌었는지 확인한 뒤에 push한다. 확인 없이 넘어가면 다른 사람 작업물이 조용히 사라질 수 있다.

## 5. 자주 발생하는 상황

- **머지 전에 `develop`이 이미 앞서 있는 경우**: 3-5의 2번(Fetch/Pull)을 건너뛰지 않는다(오래된 `develop` 기준 머지는 다른 팀원 변경을 되돌릴 수 있음).
- **당일 안에 작업이 안 끝난 경우**: 끝난 만큼 커밋 → push → 가능하면 그날치만 `develop`에 머지하고, 남은 작업은 다음 날 새 이니셜 브랜치를 다시 파서 이어간다. 하루를 넘긴 브랜치를 그대로 들고 있지 않는다.
- **Git LFS 관련 오류**: 최초 clone 후 `git lfs install`을 실행했는지 확인([CODING_CONVENTIONS.md "Git / Git LFS"](CODING_CONVENTIONS.md#git--git-lfs) 참고).