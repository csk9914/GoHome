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
//   layout <cache.json> [--colspacing 260] [--rowspacing 160] [--startx 0] [--starty 0]
//     모든 핀 링크(exec+data 둘 다) 기준 위상정렬로 각 노드의 "열"(선행 노드가 없으면 0열,
//     있으면 그 선행 노드들 중 가장 큰 열+1)을 계산하고, 같은 열 안에서는 등장 순서대로 행을
//     배정해 (node_id, pos_x, pos_y) 배열을 stdout에 JSON으로 찍는다 —
//     `blueprint_modify(operation:"move_nodes", moves:<이 출력>)`에 그대로 넣으면 됨.
//     `EdGraphNode_Comment`는 레이아웃 대상에서 제외(원래 위치 유지) — 그래프 구조와 무관한
//     주석 상자를 위상정렬에 섞으면 전부 0열로 쏠려서 의미가 없음.
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

function getOpt(args, flag, def) {
  const idx = args.indexOf(flag);
  if (idx === -1) return def;
  return parseFloat(args[idx + 1]);
}

function cmdLayout(args) {
  const cacheFile = args[0];
  if (!cacheFile) throw new Error('usage: layout <cache.json> [--colspacing 260] [--rowspacing 160] [--startx 0] [--starty 0]');
  const colSpacing = getOpt(args, '--colspacing', 260);
  const rowSpacing = getOpt(args, '--rowspacing', 160);
  const startX = getOpt(args, '--startx', 0);
  const startY = getOpt(args, '--starty', 0);

  const nodes = loadCache(cacheFile);
  const byId = {};
  for (const n of nodes) byId[n.id] = n;

  // Predecessors: for each node, the set of node ids feeding into ANY of its input pins
  // (exec and data links both count — an exec predecessor and a data predecessor both
  // belong to the left of their consumer in a left-to-right flow reading).
  const preds = {};
  for (const n of nodes) preds[n.id] = new Set();
  for (const n of nodes) {
    for (const pin of (n.pins || [])) {
      if (pin.direction === 'input') {
        for (const link of (pin.links || [])) {
          if (preds[n.id]) preds[n.id].add(link.node_id);
        }
      }
    }
  }

  // Longest-path layering, memoized, with a cycle guard (BP graphs shouldn't have true
  // dependency cycles, but a defensive guard beats an infinite loop if one slips through).
  const layer = {};
  const visiting = new Set();
  function computeLayer(id) {
    if (layer[id] !== undefined) return layer[id];
    if (visiting.has(id)) return 0;
    visiting.add(id);
    let maxP = -1;
    for (const p of preds[id] || []) {
      if (!byId[p]) continue;
      maxP = Math.max(maxP, computeLayer(p));
    }
    visiting.delete(id);
    return (layer[id] = maxP + 1);
  }

  const layoutable = nodes.filter(n => n.class !== 'EdGraphNode_Comment');
  for (const n of layoutable) computeLayer(n.id);

  const byLayer = {};
  for (const n of layoutable) {
    const L = layer[n.id];
    (byLayer[L] = byLayer[L] || []).push(n);
  }

  const moves = [];
  const layers = Object.keys(byLayer).map(Number).sort((a, b) => a - b);
  for (const L of layers) {
    byLayer[L].forEach((n, i) => {
      moves.push({ node_id: n.id, pos_x: startX + L * colSpacing, pos_y: startY + i * rowSpacing });
    });
  }

  console.log(JSON.stringify(moves));
  console.error(`layout: ${moves.length} nodes across ${layers.length} columns (${nodes.length - layoutable.length} comment box(es) excluded, left in place)`);
}

const SUBCOMMANDS = { dump: cmdDump, find: cmdFind, trace: cmdTrace, datatrace: cmdDatatrace, diff: cmdDiff, layout: cmdLayout };

const [, , sub, ...rest] = process.argv;
if (!SUBCOMMANDS[sub]) {
  console.error('usage: node Docs/tools/bp_graph_tool.js <dump|find|trace|datatrace|diff|layout> ...');
  process.exit(1);
}
try {
  SUBCOMMANDS[sub](rest);
} catch (e) {
  console.error('error:', e.message);
  process.exit(1);
}
