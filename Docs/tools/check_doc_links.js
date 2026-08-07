// GoHome 기획 문서(Docs/Design/01~05_GoHome_*.md) 상호 링크 검증 스크립트
// 사용법: node Docs/tools/check_doc_links.js  (프로젝트 루트 C:\CSK\Unreal\GoHome 에서 실행)
//
// 5개 문서는 서로를 [텍스트](파일.md#앵커) 형태로 참조한다. 문서 헤더 제목을 바꾸면
// 그 헤더를 가리키던 다른 문서의 앵커가 조용히 깨진다 (렌더링 시 그냥 링크가 안 눌리기만 함,
// 에러가 나지 않으므로 사람이 눈으로 못 찾는다). 이 스크립트가 GitHub의 앵커 생성 규칙을
// 그대로 재현해 모든 헤더의 앵커를 계산하고, 실제 링크가 그 앵커 목록에 있는지 대조한다.
//
// 문서 내용을 고친 뒤에는 습관적으로 이 스크립트를 돌려서 0 errors를 확인할 것.

const fs = require('fs');
const path = require('path');

// 04_GoHome_착수로드맵.md는 Notion으로 이전되어 스텁만 남아 있다. 검증 대상 헤더 목록에서 제외한다 —
// 이렇게 해두면 01/02/03/05 중 누군가 실수로 04에 앵커 링크(#...)를 다시 걸어도
// anchorSets['04_GoHome_착수로드맵.md']가 없으므로 FILE_NOT_FOUND로 정확히 잡힌다.
const projectRoot = path.resolve(__dirname, '..');
const docsDir = path.join(projectRoot, 'Design');
const files = fs.readdirSync(docsDir).filter(f => /^0[1235]_GoHome_.*\.md$/.test(f));

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

const anchorSets = {};
for (const f of files) {
  const content = fs.readFileSync(path.join(docsDir, f), 'utf-8');
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
  anchorSets[f] = set;
}

const linkRe = /\[[^\]]*\]\(([^)]+\.md)(#[^)]+)?\)/g;
const errors = [];
for (const f of files) {
  const content = fs.readFileSync(path.join(docsDir, f), 'utf-8');
  let m;
  while ((m = linkRe.exec(content)) !== null) {
    const targetFile = m[1];
    let anchor = m[2];
    if (!anchor) continue; // 앵커 없는 순수 파일 링크는 검사 대상 아님
    anchor = anchor.slice(1);
    if (!anchorSets[targetFile]) {
      errors.push({ from: f, targetFile, anchor, reason: 'FILE_NOT_FOUND' });
      continue;
    }
    if (!anchorSets[targetFile].has(anchor)) {
      errors.push({ from: f, targetFile, anchor, reason: 'ANCHOR_NOT_FOUND' });
    }
  }
}

if (errors.length === 0) {
  console.log(`OK: ${files.length}개 문서, 링크 오류 0건`);
} else {
  console.log(`FAIL: 링크 오류 ${errors.length}건\n`);
  for (const e of errors) {
    console.log(`  [${e.reason}] ${e.from} -> ${e.targetFile}#${e.anchor}`);
  }
  process.exitCode = 1;
}
