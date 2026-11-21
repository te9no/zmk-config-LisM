import assert from "node:assert/strict";
import net from "node:net";
import {Duplex} from "node:stream";
import {once} from "node:events";
import {createInterface} from "node:readline/promises";
import {setTimeout as delay} from "node:timers/promises";
const sharedClient = process.env.ZEROSHIFT_SHARED_RPC;
if (!sharedClient) throw new Error("Set ZEROSHIFT_SHARED_RPC to the built ZeroShift rpc.mjs path.");
const {moduleSession} = await import(sharedClient);

const socket = net.connect(Number(process.argv[2]), "127.0.0.1");
await once(socket, "connect");
const transport = {...Duplex.toWeb(socket), abortController: new AbortController()};
const client = moduleSession(transport);
const monitor = createInterface({input: process.stdin, output: process.stdout});
async function reset(side) {
  await monitor.question("RESET " + side + "\n");
  await delay(1800);
}

const modes = [0, 1, 2, 3];
async function read(side) {
  for (let attempt = 0; ; attempt++) {
    try { return await client.profile(side); }
    catch (error) {
      if (side !== "left" || !error.message.includes("左側の応答") || attempt >= 4) throw error;
      await delay(500);
    }
  }
}
function check(state, side, saved, active) {
  assert.equal(state.model, "LisM");
  assert.equal(state.side, side);
  assert.deepEqual(state.slots[0].modes.map(([id]) => id), modes);
  assert.equal(state.slots[0].saved, saved);
  assert.equal(state.slots[0].active, active);
}
try {
  await delay(1200);
  check(await read(), "right", 0, 0);
  const active = {left: 0, right: 0};
  for (const left of modes) {
    for (const right of modes) {
      for (const [side, next] of [["left", left], ["right", right]]) {
        if (active[side] === next) continue;
        check(await client.profile(side, [next]), side, next, active[side]);
        await reset(side);
        check(await read(side), side, next, next);
        active[side] = next;
      }
      check(await read("left"), "left", left, left);
      check(await read("right"), "right", right, right);
      console.log(left, right, "PASS");
    }
  }
} finally {
  monitor.close();
  process.stdin.pause();
  await client.close();
  socket.destroy();
}
