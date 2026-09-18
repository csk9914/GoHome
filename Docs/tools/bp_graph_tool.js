// mcp-unreal blueprint_query get_graph 결과(JSON)를 다루는 CLI 유틸.
// 사용법: node Docs/tools/bp_graph_tool.js <subcommand> ...
//
// 배경: 200+ 노드짜리 Blueprint 그래프를 get_graph로 조회하면 응답이 토큰 한도를 넘어
// 파일로 저장됨(경로는 매번 tool-results/... 에 타임스탬프로 바뀜). 그 파일을 열어
// exec 체인 추적/데이터 의존성 추적/노드 검색/전후 diff를 할 때마다 즉석 `node -e "..."`
// 한줄 스크립트를 새로 짜는 대신, 이 스크립트 하나를 재사용한다
// (`settlement-table-graph-split` task, 2026-09-18 — 같은 작업을 4~5번 즉석으로 다시 짬).
//
// 서브커맨드:
//   dump <get_graph.json> [<out_cache.json>]
//     get_graph 결과 파일을 읽어 클래스별 개수를 출력하고, 이후 서브커맨드가 쓸 압축
//     캐시 파일(기본: 입력파일명.cache.json)을 만든다. 다른 서브커맨드는 전부 이 캐시를 읽는다.
//   find <cache.json> <substring>
//     title/class/comment에 substring(대소문자무시)이 포함된 노드를 나열(id/class/title).
//   trace <cache.json> <node_id_prefix> [--depth N]
//     exec 출력 핀을 재귀로 따라가며 트리 출력. 이미 방문한 노드는 재수렴 표시만 하고 멈춤.
//   datatrace <cache.json> <node_id_prefix,node_id_prefix,...>
//     주어진(exec) 노드 집합의 모든 data 입력 핀을 역추적해 의존 노드 목록을 출력.
//   diff <cacheBefore.json> <cacheAfter.json>
//     두 캐시의 노드 id 집합을 비교해 삭제/추가된 노드 수와 목록을 출력(삭제+추가 산수 검증용).
//
// 각 서브커맨드는 node_id를 8자 접두어만 줘도 매칭됨(get_graph가 매번 전체 32자 GUID를
// 주지만, 사람이 다루기엔 앞 8자로 충분 — mcp-unreal 자체도 접두어 매칭을 지원함).

const fs = require('fs');

function loadRaw(file) {
  const data = JSON.parse(fs.readFileSync(file, 'utf-8'));
  return data.result ? data.result.nodes : data;
}

function loadCache(file) {
  return JSON.parse(fs.readFileSync(file, 'utf-8'));
}

function findNode(nodes, prefix) {
  return nodes.find(n => n.id.startsWith(prefix));
}

function short(id) {
  return id ? id.slice(0, 8) : id;
}

function cmdDump(args) {
  const [inFile, outFile] = args;
  if (!inFile) throw new Error('usage: dump <get_graph.json> [<out_cache.json>]');
  const nodes = loadRaw(inFile);
  const out = outFile || inFile.replace(/\.json$/, '') + '.cache.json';
  fs.writeFileSync(out, JSON.stringify(nodes));
  const byClass = {};
  for (const n of nodes) byClass[n.class] = (byClass[n.class] || 0) + 1;
  console.log('total nodes:', nodes.length);
  console.log('by class:', JSON.stringify(byClass, null, 2));
  console.log('cache written to:', out);
}

function cmdFind(args) {
  const [cacheFile, substr] = args;
  if (!cacheFile || !substr) throw new Error('usage: find <cache.json> <substring>');
  const nodes = loadCache(cacheFile);
  const needle = substr.toLowerCase();
  for (const n of nodes) {
    const hay = `${n.class} ${n.title || ''} ${n.comment || ''}`.toLowerCase();
    if (hay.includes(needle)) {
      console.log(`${n.id} [${n.class}] ${JSON.stringify(n.title || '')}`);
    }
  }
}

function cmdTrace(args) {
  const cacheFile = args[0];
  const startPrefix = args[1];
  let maxDepth = 30;
  const depthIdx = args.indexOf('--depth');
  if (depthIdx !== -1) maxDepth = parseInt(args[depthIdx + 1], 10);
  if (!cacheFile || !startPrefix) throw new Error('usage: trace <cache.json> <node_id_prefix> [--depth N]');

  const nodes = loadCache(cacheFile);
  const byId = {};
  for (const n of nodes) byId[n.id] = n;

  function describe(n) {
    if (!n) return '???';
    return `${short(n.id)} [${n.class}] ${JSON.stringify(n.title || '').slice(0, 60)}`;
  }

  const visited = new Set();
  function walk(id, depth) {
    const n = byId[id];
    if (!n) { console.log('  '.repeat(depth) + `MISSING ${short(id)}`); return; }
    console.log('  '.repeat(depth) + describe(n));
    if (visited.has(id)) { console.log('  '.repeat(depth + 1) + '^^ already visited'); return; }
    visited.add(id);
    if (depth > maxDepth) { console.log('  '.repeat(depth + 1) + '... max depth reached'); return; }
    for (const pin of (n.pins || [])) {
      if (pin.type === 'exec' && pin.direction === 'output') {
        for (const link of (pin.links || [])) {
          console.log('  '.repeat(depth + 1) + `-> exec[${pin.name}]`);
          walk(link.node_id, depth + 2);
        }
      }
    }
  }

  const start = findNode(nodes, startPrefix);
  if (!start) throw new Error('start node not found for prefix ' + startPrefix);
  walk(start.id, 0);
}

function cmdDatatrace(args) {
  const cacheFile = args[0];
  const idsArg = args[1];
  if (!cacheFile || !idsArg) throw new Error('usage: datatrace <cache.json> <prefix1,prefix2,...>');

  const nodes = loadCache(cacheFile);
  const byId = {};
  for (const n of nodes) byId[n.id] = n;

  function describe(n) {
    if (!n) return '???';
    return `${short(n.id)} [${n.class}] ${JSON.stringify(n.title || '').slice(0, 60)}`;
  }

  const execIds = idsArg.split(',').map(p => {
    const n = findNode(nodes, p);
    if (!n) throw new Error('exec node not found for prefix ' + p);
    return n.id;
  });
  const execSet = new Set(execIds);

  const dataVisited = new Set();
  const queue = [];
  for (const id of execIds) {
    const n = byId[id];
    for (const pin of (n.pins || [])) {
      if (pin.type !== 'exec' && pin.direction === 'input') {
        for (const link of (pin.links || [])) queue.push(link.node_id);
      }
    }
  }
  while (queue.length) {
    const id = queue.pop();
    if (dataVisited.has(id) || execSet.has(id)) continue;
    dataVisited.add(id);
    const n = byId[id];
    if (!n) { console.log('MISSING data node', short(id)); continue; }
    for (const pin of (n.pins || [])) {
      if (pin.direction === 'input') {
        for (const link of (pin.links || [])) queue.push(link.node_id);
      }
    }
  }

  console.log('=== data-only dependency nodes ===');
  for (const id of dataVisited) console.log(describe(byId[id]));
  console.log('total data nodes:', dataVisited.size);
  console.log('total exec nodes:', execSet.size);
}

function cmdDiff(args) {
  const [beforeFile, afterFile] = args;
  if (!beforeFile || !afterFile) throw new Error('usage: diff <cacheBefore.json> <cacheAfter.json>');
  const before = loadCache(beforeFile);
  const after = loadCache(afterFile);
  const beforeIds = new Set(before.map(n => n.id));
  const afterIds = new Set(after.map(n => n.id));

  const removed = before.filter(n => !afterIds.has(n.id));
  const added = after.filter(n => !beforeIds.has(n.id));

  console.log(`before: ${before.length} nodes, after: ${after.length} nodes`);
  console.log(`removed: ${removed.length}, added: ${added.length}`);
  console.log(`arithmetic check: ${before.length} - ${removed.length} + ${added.length} = ${before.length - removed.length + added.length} (expect ${after.length})`);
  console.log('--- removed ---');
  for (const n of removed) console.log(`${short(n.id)} [${n.class}] ${JSON.stringify(n.title || '')}`);
  console.log('--- added ---');
  for (const n of added) console.log(`${short(n.id)} [${n.class}] ${JSON.stringify(n.title || '')}`);
}

const SUBCOMMANDS = { dump: cmdDump, find: cmdFind, trace: cmdTrace, datatrace: cmdDatatrace, diff: cmdDiff };

const [, , sub, ...rest] = process.argv;
if (!SUBCOMMANDS[sub]) {
  console.error('usage: node Docs/tools/bp_graph_tool.js <dump|find|trace|datatrace|diff> ...');
  process.exit(1);
}
try {
  SUBCOMMANDS[sub](rest);
} catch (e) {
  console.error('error:', e.message);
  process.exit(1);
}
