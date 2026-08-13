// ARCHITECTURE.md에 백틱으로 적힌 UE 클래스/인터페이스/열거형/파일명이 실제 Source/GoHome/에
// 존재하는지 검사한다.
// 사용법: node Docs/tools/check_architecture_symbols.js  (프로젝트 루트에서 실행)
//
// ARCHITECTURE.md는 "폴더별 클래스/인터페이스는 Source/GoHome/<폴더>/가 유일한 출처"라고 명시하면서도
// 문서 본문에 구체적 클래스명을 나열한다(시스템 간 인터페이스 계약 표 등). 코드에서 그 이름이
// 리네임/삭제되면 문서가 조용히 거짓말을 하게 된다 — 렌더링 에러가 없어 사람이 눈으로 못 찾는다.
// 이 스크립트는 UE 네이밍 컨벤션(A/U/I/E/F/S 접두사 + PascalCase)에 맞는 백틱 토큰과 .h/.cpp
// 파일명만 뽑아 Source/GoHome/ 전체에서 텍스트로 존재하는지 대조한다.
//
// 주의: "(신설 제안)"으로 표시된 항목은 이 문서가 제안하는 아직 없는 심볼이라 NOT_FOUND가 정상이다 —
// 이 스크립트는 그 태그를 자동으로 걸러내지 않으므로, 결과는 사람이 한 번 더 판단해야 한다.
// 함수/델리게이트명(GenerateNoise, OnDeath 등)은 UE 클래스 접두사 규칙이 없어 검사 대상에서 뺐다.

const fs = require('fs');
const path = require('path');

const projectRoot = path.resolve(__dirname, '..', '..');
const archFile = path.join(projectRoot, 'Docs', 'Dev', 'ARCHITECTURE.md');
const sourceDir = path.join(projectRoot, 'Source', 'GoHome');

const content = fs.readFileSync(archFile, 'utf-8');

// UE 클래스류: A/U/I/E/F/S 접두사 다음 대문자로 시작하는 PascalCase (AGoHomeGameState, IWeightProvider 등)
const classTokenRe = /`([AUIEFS][A-Z][A-Za-z0-9_]*)`/g;
// 파일명: `ItemActorBase.cpp` 형태
const fileTokenRe = /`([A-Za-z0-9_]+\.(h|cpp))`/g;

const classTokens = new Set();
const fileTokens = new Set();
let m;
while ((m = classTokenRe.exec(content)) !== null) classTokens.add(m[1]);
while ((m = fileTokenRe.exec(content)) !== null) fileTokens.add(m[1]);

function walk(dir, out) {
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const abs = path.join(dir, entry.name);
    if (entry.isDirectory()) walk(abs, out);
    else out.push(abs);
  }
}

const sourceFiles = [];
walk(sourceDir, sourceFiles);

const sourceText = sourceFiles
  .filter(f => /\.(h|cpp)$/.test(f))
  .map(f => fs.readFileSync(f, 'utf-8'))
  .join('\n');

const sourceBasenames = new Set(sourceFiles.map(f => path.basename(f)));

const missingClasses = [...classTokens].filter(t => !new RegExp(`\\b${t}\\b`).test(sourceText));
const missingFiles = [...fileTokens].filter(t => !sourceBasenames.has(t));

if (missingClasses.length === 0 && missingFiles.length === 0) {
  console.log(`OK: 클래스류 ${classTokens.size}개, 파일 ${fileTokens.size}개 모두 Source/GoHome/에서 확인됨`);
} else {
  console.log(`FAIL: Source/GoHome/에서 확인 안 되는 심볼 ${missingClasses.length + missingFiles.length}건`);
  console.log(`("신설 제안" 항목이면 정상 — 아니라면 리네임/삭제 여부 확인)\n`);
  for (const t of missingClasses) console.log(`  [CLASS_NOT_FOUND] ${t}`);
  for (const t of missingFiles) console.log(`  [FILE_NOT_FOUND] ${t}`);
  process.exitCode = 1;
}
