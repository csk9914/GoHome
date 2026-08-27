# NodeCaster 사용 가이드 (Blueprint 시각화)

> `Docs/Dev/` 문서군. [AI_AGENT_GUIDE.md](AI_AGENT_GUIDE.md)의 라우팅표 "Blueprint 노드 그래프 어떻게 만들어?" 행에서 연결된다.

## 1. 설치 확인

**선택 사항 — 팀 전체 배포 전 단계.** NodeCaster는 아직 공식 배포되지 않았고, 설치한 사람만 쓸 수 있다. 아래로 확인부터 한다 — Node 프로세스를 안 띄우고 결과만 바로 나온다.

- PowerShell: `Get-Command nodecaster -ErrorAction SilentlyContinue`
- bash: `command -v nodecaster`

**못 찾으면 "2. 사용 절차"를 건너뛰고**, 지금까지 해온 방식대로 Blueprint 그래프를 사람이 읽을 설명(노드 목록 + 연결 관계를 텍스트/의사코드로)으로 안내한다 — 설치를 강요하거나 작업을 멈추지 않는다. **찾으면** 2절로 진행한다.

## 2. 사용 절차 (설치된 경우만)

이 프로젝트에서 Blueprint 그래프를 제안할 때는, 사람이 에디터에서 수동으로 노드를 배치하기 전에 먼저 NodeCaster로 시각화하라.

- 명령어: `nodecaster` (전역 등록되어 있음 — 안 되면 아래 "설치" 참고)
- 절차:
  1. 그래프 스펙을 JSON 파일로 작성한다 (스키마는 아래 참고). 프리셋에 없는 노드는:
     - **원칙**: 소스를 직접 읽을 수 있는 것(이 프로젝트 자신의 C++ 클래스/함수)은 항상 그 소스를 직접 읽어서 채운다. `lookup`/리플렉션 덤프는 소스가 없는(엔진에 컴파일되어 있는) 라이브러리 함수 전용 보조 수단이지, 프로젝트 코드를 대체하는 게 아니다.
     - 언리얼 기본 함수(엔진 라이브러리 함수 등 프로젝트에 소스가 없는 것)라면 먼저 `nodecaster lookup <함수명>`으로 실제 파라미터 이름/타입/기본값을 조회해서 `inputs`/`outputs`를 채운다 — 추측하지 않는다.
     - 프로젝트 고유 C++ 함수(이 프로젝트 자신의 액터/컴포넌트 등 클래스에 선언된 `UFUNCTION`)는, 그래프가 그 클래스(또는 그 자식 BP) 자신을 대상으로 호출하는 거라면 실제 UFUNCTION 선언을 직접 읽고 채우되 `functionOwner`는 지정하지 않는다(self-context로 해석됨) — `lookup`에 그 함수가 나오더라도 무시하고 이 원칙을 따른다.
     - **다른 오브젝트에 대고 멤버 함수를 호출**하는 경우(예: 어떤 컴포넌트 참조 변수에 대고 그 컴포넌트의 함수 호출)는 `functionOwner`를 명시하고, `inputs`에 `{"name":"Target","type":"object"}` 핀을 추가해서 대상 오브젝트에 연결한다. 프로젝트 자체 클래스(엔진 라이브러리가 아닌)도 지원된다.
     - `CastTo` 노드의 `targetClass`(`/Script/<Module>.<ClassName>` 형식)도 자동조회 대상이 아니다 — 대상 클래스 헤더 파일과 `Source/GoHome/` 폴더 구조를 직접 읽어서 클래스명(`A`/`U` 접두어 제거)을 조합해 채운다.
  2. `nodecaster visualize <그래프.json 경로>` 실행 — 다이어그램이 브라우저로 자동으로 열린다.
  3. 그래프가 지원되는 노드로만 구성됐다면 미리보기 페이지에 "Copy for Unreal paste" 버튼이 있다. 사람에게 그 버튼으로 복사해서 UE 에디터에 Ctrl+V 하라고 안내한다. 지원 안 되는 노드가 섞여 있으면 사람이 그 부분만 수동으로 만들어야 한다.
  4. 브라우저 없이 클립보드에 바로 복사만 하고 싶으면 `nodecaster copy <그래프.json 경로>`를 대신 쓸 수 있다 — 단, visualize로 먼저 사람이 검토하게 하는 게 원칙이다.

**설치(팀원별 1회)**: `C:\CSK\NodeCaster` 저장소를 clone한 뒤 그 폴더에서 `npm install && npm run build && npm link` — 이후 이 프로젝트를 포함해 어느 폴더에서든 `nodecaster` 명령을 그대로 쓸 수 있다.

그래프 JSON 스키마, 지원 노드 종류(프리셋), 클립보드 붙여넣기가 검증된 노드 목록은 NodeCaster 저장소(`C:\CSK\NodeCaster`)의 `README.md`를 참고하라.
