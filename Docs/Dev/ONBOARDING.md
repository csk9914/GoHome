# GoHome 온보딩 / 빌드 가이드 (개발용)

> `Docs/Dev/` 문서군 — `Docs/Design/`·Notion과는 별개 체계. 새 팀원이 처음 이 프로젝트를 열 때 따라갈 순서만 안내한다.

## 0. 문서 지도부터

이 저장소에는 세 종류의 문서가 있다:

- **`Docs/Design/`** — 게임 디자인 본체 (01 기획서, 02 기술분석서, 03 리스크/미결정사항, 05 참고문서 교차매핑). 결정된 사항 전체 색인은 [../Design/03_GoHome_리스크_미결정사항.md](../Design/03_GoHome_리스크_미결정사항.md) 참고.
- **Notion** — 담당자 배정, 착수 로드맵, DoD처럼 자주 바뀌는 진행 트래커. 문서명만 기억해두면 됨 ("GoHome 착수 로드맵", "GoHome 담당자 배정").
- **`Docs/Dev/`**(이 폴더) — 코드와 함께 바뀌는 개발자용 문서: [ARCHITECTURE.md](ARCHITECTURE.md)(클래스/폴더 매핑), [CODING_CONVENTIONS.md](CODING_CONVENTIONS.md)(코딩 규칙), 이 문서(온보딩).

## 1. 엔진 설치

- Unreal Engine **5.7** 설치 (Epic Games Launcher 경유, `.uproject`의 `EngineAssociation`이 5.7로 고정되어 있음)
- Visual Studio 2022 + "게임 개발용 C++" 워크로드 (`.vsconfig`가 저장소에 있으니 Visual Studio Installer에서 이 파일로 구성 불러오기 가능)

## 2. 저장소 초기화 (현재 미착수 — 처음 착수하는 사람이 진행)

이 폴더는 아직 git 저장소가 아니다. 처음 시작할 때:

1. `git init`
2. `.gitignore`에 최소한 다음을 제외: `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.vs/`, `.idea/`
3. Git LFS 설치 후 `git lfs install`, `.gitattributes`에 `*.uasset`, `*.umap` 등 바이너리 확장자 트래킹 등록 ([CODING_CONVENTIONS.md의 Git/LFS 절](CODING_CONVENTIONS.md#git--git-lfs) 참고)
4. 최초 커밋

## 3. 프로젝트 열기 / 빌드

1. `GoHome.uproject` 우클릭 → **Generate Visual Studio project files** (또는 UnrealBuildTool을 커맨드라인으로 직접 호출)
2. 생성된 `GoHome.sln`을 Visual Studio로 열기
3. 빌드 구성 선택:
   - **Debug**: 엔진까지 디버그 심볼 포함, 가장 느림 — 엔진 코드 자체를 디버깅할 때만
   - **DebugGame**: 엔진은 최적화된 채로, 게임 모듈만 디버그 심볼 — 평소 개발 시 기본값
   - **Development**: 최적화 빌드, 에디터에서 플레이 테스트용
4. 에디터에서 실행하거나, Standalone Game으로 실행해 멀티플레이 테스트 (리슨 서버 구조이므로 2개 이상 인스턴스 실행 필요)

## 4. Steam OnlineSubsystem 설정 (착수 시 진행 — 아직 미반영)

[../Design/02_GoHome_기술분석서.md 전제 조건](../Design/02_GoHome_기술분석서.md#전제-조건)에서 이미 팀 결정된 값:

- 개발 중 테스트용 App ID: **480 (Spacewar)**, 정식 App ID는 출시 단계에서 별도 신청
- `Config/DefaultEngine.ini`에 `[OnlineSubsystem]` 섹션과 `DefaultPlatformService=Steam` 추가 필요 (아직 미반영 — 담당자가 1번 시스템(온라인 서브시스템) 착수 시 진행)
- Steamworks SDK 연동은 `OnlineSubsystemSteam` 플러그인 활성화로 시작

## 5. 코드 작성 전에

- [CODING_CONVENTIONS.md](CODING_CONVENTIONS.md)를 먼저 읽는다 (폴더 규칙, C++/BP 경계, 네이밍)
- 담당 시스템이 [ARCHITECTURE.md](ARCHITECTURE.md)의 어느 폴더에 해당하는지 확인한다
- 스펙 자체(인일/복잡도 제외)는 [../Design/02_GoHome_기술분석서.md](../Design/02_GoHome_기술분석서.md)에서 해당 시스템 절을 찾아 읽는다
- 담당자·착수 시점·DoD는 Notion("GoHome 담당자 배정" DB, "GoHome 착수 로드맵")에서 확인한다 (이 저장소 md에는 복사되어 있지 않음)
