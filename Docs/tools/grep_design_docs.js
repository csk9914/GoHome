// Docs/Design/01,02,03,05 문서에서 키워드를 검색한다.
// 사용법: node Docs/tools/grep_design_docs.js <키워드>  (프로젝트 루트에서 실행)

const fs = require('fs');
const path = require('path');

const keyword = process.argv[2];
if (!keyword) {
  console.error('사용법: node Docs/tools/grep_design_docs.js <키워드>');
  process.exit(1);
}

const designDir = path.join(__dirname, '..', 'Design');
const files = fs.readdirSync(designDir)
  .filter(f => /^0[1235]_GoHome_.*\.md$/.test(f))
  .map(f => path.join(designDir, f));

const needle = keyword.toLowerCase();
let hits = 0;
for (const absPath of files) {
  const displayName = path.relative(path.join(__dirname, '..', '..'), absPath).replace(/\\/g, '/');
  const lines = fs.readFileSync(absPath, 'utf-8').split('\n');
  lines.forEach((line, i) => {
    if (line.toLowerCase().includes(needle)) {
      console.log(`${displayName}:${i + 1}: ${line.trim()}`);
      hits++;
    }
  });
}

if (hits === 0) {
  console.log(`"${keyword}" 관련 언급 없음`);
}
