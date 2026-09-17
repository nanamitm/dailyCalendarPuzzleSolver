// Solve off the main thread. Cancellation is done by terminating the worker,
// so no cooperative cancel flag is needed here.
import { solveDate, solveDateCustom, parsePieceSet, BXL, BYL } from './solver-core.js';

self.onmessage = (e) => {
    const msg = e.data;
    if (msg.type !== 'solve') return;

    let out;
    try {
        if (msg.pieceSetText) {
            const pset = parsePieceSet(msg.pieceSetText, msg.pieceSetName);
            out = solveDateCustom(msg.weekday, msg.day, msg.month, msg.findAll, pset);
        } else {
            out = solveDate(msg.weekday, msg.day, msg.month, msg.findAll);
        }
    } catch (err) {
        self.postMessage({ type: 'error', reqId: msg.reqId, error: String(err && err.message || err) });
        return;
    }

    // Pack every solution board into one transferable buffer
    const stride = BYL * BXL;
    const packed = new Int8Array(out.solutions.length * stride);
    out.solutions.forEach((sol, i) => packed.set(sol, i * stride));

    self.postMessage({
        type: 'done',
        reqId: msg.reqId,
        count: out.solutions.length,
        stride,
        boards: packed,
        tries: out.tries,
        elapsedMs: out.elapsedMs,
    }, [packed.buffer]);
};
