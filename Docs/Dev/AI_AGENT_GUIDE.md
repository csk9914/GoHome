# GoHome AI 에이전트 활용 가이드 (개발용)

> `Docs/Dev/` 문서군. 팀원이 자기 AI 코딩 에이전트(Claude Code, Codex CLI 등)에게 담당 작업을 물어볼 때 따르는 절차를 다룬다. "사람이 뭘 물어야 하는가"를 다루는 문서이며, 에이전트가 실제 참조할 규칙 본문은 저장소 루트 [CLAUDE.md](../../CLAUDE.md)에 있다 — 여기서 복제하지 않는다. 아래 라우팅표의 단일 출처는 이 파일이다 — `CLAUDE.md`는 이 표를 복제하지 않고 [Document Map](../../CLAUDE.md#document-map)에서 이 파일을 가리키기만 한다.

## 1. 에이전트에게 처음 알려줄 것

에이전트는 아래 두 가지를 스스로 알아낼 수 없으므로 프롬프트에 직접 포함해야 한다:

- **본인 담당 시스템 이름**: GoHome 담당자 배정 DB(Notion)에서 본인이 직접 확인 후 전달.
- **작업 레포 루트**가 이 저장소(`GoHome.uproject`가 있는 폴더)라는 것.

## 2. 질문 유형 → 참조 문서 라우팅표

Dev가 1차, Notion은 필요 시만 부차적으로(에이전트가 직접 열지 않음). 표에 없는 질문은 먼저 Dev, 없으면 Notion. "Notion" 열 항목은 사람이 확인해 프롬프트에 전달한다.

| 질문 | 참조 문서 |
|---|---|
| (질문이 아니라 구현 작업 자체) 코드를 새로 짜거나 수정하는 모든 작업 — 착수 전에 항상 확인 | [OOP/SOLID 적용 방식](CODING_CONVENTIONS.md#oopsolid-적용-방식) + [C++ / Blueprint 경계](CODING_CONVENTIONS.md#c--blueprint-경계) + [구현 완료 시 자가검토](CODING_CONVENTIONS.md#구현-완료-시-자가검토-에이전트-기준) — 팀원이 코딩 규칙을 따로 묻지 않아도 구현 작업이면 항상 먼저 연다 |
| 어떤 클래스/인터페이스를 만들어야 해? 다른 시스템과 어떻게 연결돼? (**메인 개발 루프에서 가장 흔한 질문**) | 클래스 자체는 `Source/GoHome/<담당 폴더>/`가 출처. 경계 규칙·이유는 [시스템별 결정과 경계](ARCHITECTURE.md#시스템별-결정과-경계) + [시스템 간 인터페이스 계약](ARCHITECTURE.md#시스템-간-인터페이스-계약) — 안 풀리면 그때만 아래 "기획 의도/수치 근거" 행 참고 |
| 내 시스템의 기획 의도·밸런스 수치·유저 경험 흐름이 뭐야? (ARCHITECTURE.md에 없는 서사적 스펙) | GoHome 기술분석서(Notion) 해당 시스템 절 — 사람이 확인해 전달 |
| 이 시스템 지금 뭐가 이미 구현됐고 뭐가 남았어? | 문서가 아니라 `Source/GoHome/Public|Private/<담당 폴더>/`를 직접 읽고 위 두 문서와 대조 — 진행 상태의 단일 출처는 코드 자체이며 별도 문서는 두지 않는다 |
| 이거 팀이 이미 결정한 거야, 아직 미정이야? | 결정된 사항은 GoHome 기획서·기술분석서(Notion)의 해당 절. 아직 미정인 사안은 GoHome 착수 로드맵(Notion) — 둘 다 사람이 확인해 전달 |
| 코딩 규칙/폴더 규칙/네이밍이 뭐야? | [CODING_CONVENTIONS.md](CODING_CONVENTIONS.md) |
| 이 값(수치)이 어디서 온 결정인지 근거가 궁금해 | GoHome 기획서·기술분석서(Notion)의 해당 절 — 사람이 확인해 전달 |
| 담당자/목표 기간/DoD가 뭐야? | GoHome 담당자 배정 DB, GoHome 착수 로드맵(Notion) — 사람이 확인해 전달 |
| 브랜치 어떻게 만들어/머지해? PR은 언제부터 써? | GoHome Git 워크플로우(Notion) — 사람이 확인해 전달, 또는 사람이 직접 수행 |
| 바이너리(.uasset/.umap) 머지 충돌 CLI로 어떻게 풀어? | [CODING_CONVENTIONS.md "바이너리 충돌 시 CLI로 한쪽 선택하기"](CODING_CONVENTIONS.md#바이너리uassetumap-충돌-시-cli로-한쪽-선택하기) — 실시간 도움이 필요한 절이라 md로 직접 참조 |
| UI 화면 새로 만들거나 구조 바꿔야 해 (레이어·라우터·연출·네이밍) | [UI_GUIDE.md](UI_GUIDE.md) — 개별 위젯 필드는 `Content/GoHome/UI/`가 출처, 데이터 방향 원칙은 [ARCHITECTURE.md UI 절](ARCHITECTURE.md#ui) |
| Blueprint 노드 그래프 어떻게 만들어? 코드랑 같이 BP 가이드도 줘 | [NODECASTER_GUIDE.md](NODECASTER_GUIDE.md) — 설치 안 돼 있으면 "2. 사용 절차"는 읽지 않고 기존 방식(텍스트 설명)으로 안내 |
| 블루프린트 코멘트 박스 색상은 뭘 써야 해? | [BP_COMMENT_COLORS.md](BP_COMMENT_COLORS.md) |

## 3. 등록된 스킬 사용법 (Claude Code)

Claude Code를 쓰는 팀원은 매번 절차를 프롬프트로 다시 설명하는 대신, 저장소에 등록된 스킬을 슬래시 명령으로 바로 호출할 수 있다. 스킬은 `.claude/skills/<이름>/SKILL.md`에 정의되며, Claude Code가 아닌 다른 에이전트(Codex CLI 등)에는 이 메커니즘이 없다 — 그 경우 위 라우팅표를 프롬프트에 직접 풀어서 전달한다.

| 스킬 | 호출 | 용도 |
|---|---|---|
| `dev-doc-review` | `/dev-doc-review [파일명 또는 경로] [최대 루프 횟수]` | `Docs/Dev/`의 `ARCHITECTURE.md`·`CODING_CONVENTIONS.md`·`AI_AGENT_GUIDE.md`를 원칙급 리뷰어 서브에이전트로 반복 검증·반영. 문서명 생략 시 대상을 물어보고, 루프 횟수 생략 시 기본 3회 |

- 대상 파일명은 별칭으로도 인식된다: `아키텍처`/`ARCHITECTURE`, `컨벤션`/`CONVENTIONS`, `에이전트 가이드`/`AI_AGENT_GUIDE`. 예: `/dev-doc-review 아키텍처 2`
- 새 스킬이 추가되면 이 표에 한 줄만 추가한다(상세 절차의 유일한 출처는 각 `SKILL.md`).

## 4. 에이전트 답변 검증 원칙

- 인일 추정치·"이미 결정됐는지 여부" 같은 Notion 단일 출처 항목의 숫자를 에이전트가 스스로 지어내면 안 된다 — 반드시 사람이 전달한 Notion 인용을 근거로 답해야 하며, 인용 없이 답하면 재질문한다. 담당자/일정도 마찬가지로 사람이 전달한 값이 Notion과 일치하는지만 재확인시킨다.
- 에이전트가 새 인터페이스/클래스를 제안하면 [시스템 간 인터페이스 계약](ARCHITECTURE.md#시스템-간-인터페이스-계약)에 같은 접점이 이미 정의돼 있지 않은지 먼저 확인시킨다.

