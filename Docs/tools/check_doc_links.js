// GoHome 문서(Docs/Design/01~05, Docs/Dev/*.md) 상호 링크 검증 스크립트
// 사용법: node Docs/tools/check_doc_links.js  (프로젝트 루트 C:\CSK\Unreal\GoHome 에서 실행)
//
// 문서들은 서로를 [텍스트](파일.md#앵커) 형태의 상대경로 링크로 참조한다. 문서 헤더 제목을 바꾸면
// 그 헤더를 가리키던 다른 문서의 앵커가 조용히 깨진다 (렌더링 시 그냥 링크가 안 눌리기만 함,
// 에러가 나지 않으므로 사람이 눈으로 못 찾는다). 이 스크립트가 GitHub의 앵커 생성 규칙을
// 그대로 재현해 모든 헤더의 앵커를 계산하고, 실제 링크가 그 앵커 목록에 있는지 대조한다.
//
// 검사 대상: 저장소 루트 CLAUDE.md + Docs/Design/01,02,03,05·DOC_MANAGEMENT.md (기획 문서) +
// Docs/Dev/*.md (개발 문서, ARCHITECTURE/CODING_CONVENTIONS/AI_AGENT_GUIDE 등). 링크는 절대경로가
// 아니라 파일 기준 상대경로로 해석하므로 Docs/Dev/ 문서가 ../Design/...md#앵커 형태로 기획서를
// 참조하는 링크, CLAUDE.md와 Docs/Design/·Docs/Dev/ 사이를 오가는 링크도 함께 검증된다.
// Notion 링크(URL)는 md 파일이 아니므로 애초에 이 스크립트의 검사 대상이 아니다 — 깨졌는지는 수동 확인.
//
// 문서 내용을 고친 뒤에는 습관적으로 이 스크립트를 돌려서 0 errors를 확인할 것.

const fs = require('fs');
const path = require('path');

// 04_GoHome_착수로드맵.md는 Notion으로 완전히 이전되어 파일 자체가 저장소에 없다. designFiles
// 필터가 01/02/03/05·DOC_MANAGEMENT만 골라 검증 대상 헤더 목록에서 자연히 제외한다 —
// 이렇게 해두면 누군가 실수로 04에 앵커 링크(#...)를 다시 걸어도
// anchorSets에 그 파일이 없으므로 FILE_NOT_FOUND로 정확히 잡힌다.
const projectRoot = path.resolve(__dirname, '..');
const designDir = path.join(projectRoot, 'Design');
const devDir = path.join(projectRoot, 'Dev');

const designFiles = fs.readdirSync(designDir)
  .filter(f => f === 'DOC_MANAGEMENT.md' || /^0[1235]_GoHome_.*\.md$/.test(f))
  .map(f => path.join(designDir, f));
const devFiles = fs.readdirSync(devDir)
  .filter(f => /\.md$/.test(f))
  .map(f => path.join(devDir, f));
const rootFiles = [path.join(projectRoot, '..', 'CLAUDE.md')];

const targetFiles = [...designFiles, ...devFiles, ...rootFiles];

function githubAnchor(header) {
  let h = header.toLowerCase();
  let out = '';
  for (const ch of h) {
    // GitHub은 문자/숫자/밑줄/하이픈/공백만 남기고 나머지(이모지, 괄호, 콜론, 쉼표 등)는 제거한다.
    // 한글 음절 범위(가~힣, 자모 포함)도 문자로 취급된다.
    if (/[a-z0-9_\- ㄱ-힝]/.test(ch)) {
      out += ch;
    }
  }
  h = out.replace(/ /g, '-');
  return h;
}

// 절대경로 기준으로 앵커 집합을 관리한다 — Dev -> ../Design/... 처럼 폴더를 넘나드는
// 상대경로 링크도 같은 기준으로 정규화해서 대조할 수 있게 하기 위함.
const anchorSets = {};
for (const absPath of targetFiles) {
  const content = fs.readFileSync(absPath, 'utf-8');
  const lines = content.split('\n');
  const seen = {};
  const set = new Set();
  for (const line of lines) {
    const m = line.match(/^(#{1,6})\s+(.*)/);
    if (m) {
      const anchor = githubAnchor(m[2]);
      // GitHub은 같은 앵커가 중복되면 두 번째부터 -1, -2 ... 를 붙인다.
      if (seen[anchor] !== undefined) {
        seen[anchor]++;
        set.add(`${anchor}-${seen[anchor]}`);
      } else {
        seen[anchor] = 0;
        set.add(anchor);
      }
    }
  }
  anchorSets[absPath] = set;
}

const linkRe = /\[[^\]]*\]\(([^)]+\.md)(#[^)]+)?\)/g;
const errors = [];
for (const absPath of targetFiles) {
  const content = fs.readFileSync(absPath, 'utf-8');
  const fromDir = path.dirname(absPath);
  const displayName = path.relative(projectRoot, absPath).replace(/\\/g, '/');
  let m;
  while ((m = linkRe.exec(content)) !== null) {
    const targetAbs = path.resolve(fromDir, m[1]);
    let anchor = m[2];
    if (!anchor) continue; // 앵커 없는 순수 파일 링크는 검사 대상 아님
    anchor = anchor.slice(1);
    const targetDisplay = path.relative(projectRoot, targetAbs).replace(/\\/g, '/');
    if (!anchorSets[targetAbs]) {
      errors.push({ from: displayName, targetFile: targetDisplay, anchor, reason: 'FILE_NOT_FOUND' });
      continue;
    }
    if (!anchorSets[targetAbs].has(anchor)) {
      errors.push({ from: displayName, targetFile: targetDisplay, anchor, reason: 'ANCHOR_NOT_FOUND' });
    }
  }
}

if (errors.length === 0) {
  console.log(`OK: ${targetFiles.length}개 문서, 링크 오류 0건`);
} else {
  console.log(`FAIL: 링크 오류 ${errors.length}건\n`);
  for (const e of errors) {
    console.log(`  [${e.reason}] ${e.from} -> ${e.targetFile}#${e.anchor}`);
  }
  process.exitCode = 1;
}
