import assert from 'node:assert/strict';
import { readFileSync, readdirSync } from 'node:fs';
const source = readFileSync(new URL('../web/solver-core.js', import.meta.url), 'utf8');
const core = await import('data:text/javascript;base64,' + Buffer.from(source).toString('base64'));

for (const data of [null, [], {}, { pieces: [] },
    { pieces: [{ vectors: [[100,0]] }] },
    { pieces: [{ vectors: [[0.5,0]] }] },
    { pieces: [{ vectors: [[0,0]] }] },
    { pieces: [{ vectors: [[2,0]] }] },
    { pieces: [{ vectors: [] }] }]) {
    assert.throws(() => core.parsePieceSet(JSON.stringify(data)));
}
const directory = new URL('../gui_cpp/pieces/', import.meta.url);
for (const name of readdirSync(directory).filter(n => n.endsWith('.json')))
    assert.ok(core.parsePieceSet(readFileSync(new URL(name, directory), 'utf8')).pieces.length);

const short = { pieces: [new core.Piece([], 1, [core.UP])] };
assert.equal(core.solveDateCustom(1,1,1,false,short).solutions.length, 0);
const singles = core.parsePieceSet(JSON.stringify({
    pieces: Array.from({ length: 47 }, () => ({ vectors: [] }))
}));
const full = core.solveDateCustom(1,1,1,false,singles);
assert.equal(full.solutions.length, 1);
assert.ok(!full.solutions[0].includes(0));
const original = core.solveDate(1,27,3,true);
assert.equal(original.solutions.length, 1);
assert.equal(original.tries, 3026228);
console.log('Web solver regressions passed');
