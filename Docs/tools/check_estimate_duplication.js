// Docs/Design/ 문서에 "인일" 추정치 숫자가 유일 출처(02문서) 밖에 다시 옮겨 적혔는지 검사한다.
// 사용법: node Docs/tools/check_estimate_duplication.js  (프로젝트 루트에서 실행)
//
// DOC_MANAGEMENT.md의 단일 출처 원칙: 인일 추정치의 유일한 출처는 02문서(시스템별 요약 표,
// 13. 2차 프로토타입 추가 시스템)이고, 다른 문서는 링크만 걸고 숫자를 다시 옮겨 적지 않는다.
// 이 규칙은 지금까지 사람이 기억해서 지키는 규칙이었다 — 이 스크립트가 기계적으로 검증한다.
//
// 검사 대상: Docs/Design/01,03,05,DOC_MANAGEMENT.md. "N인일" / "N~M인일" 형태(소수점 포함)의
// 숫자가 하나라도 나오면 위반으로 보고한다. 02문서는 출처이므로 검사하지 않는다.

const fs = require('fs');
const path = require('path');

const designDir = path.join(__dirname, '..', 'Design');
const sourceFile = '02_GoHome_기술분석서.md';

const targetFiles = fs.readdirSync(designDir)
  .filter(f => /\.md$/.test(f) && f !== sourceFile);

const estimateRe = /\d+(\.\d+)?(~\d+(\.\d+)?)?\s*인일/g;

const violations = [];
for (const file of targetFiles) {
  const absPath = path.join(designDir, file);
  const lines = fs.readFileSync(absPath, 'utf-8').split('\n');
  lines.forEach((line, i) => {
    const matches = line.match(estimateRe);
    if (matches) {
      for (const m of matches) {
        violations.push({ file: `Docs/Design/${file}`, lineNo: i + 1, match: m, text: line.trim() });
      }
    }
  });
}

if (violations.length === 0) {
  console.log(`OK: ${sourceFile} 밖에서 인일 숫자 중복 없음 (검사 대상 ${targetFiles.length}개 문서)`);
} else {
  console.log(`FAIL: 인일 숫자 중복 ${violations.length}건 — 유일 출처(${sourceFile}) 밖에서 숫자를 다시 적었다\n`);
  for (const v of violations) {
    console.log(`  ${v.file}:${v.lineNo}: "${v.match}"`);
    console.log(`    ${v.text}`);
  }
  process.exitCode = 1;
}
