// LAN join through the local UDP/WebSocket relay.
//
// Starts a passworded bitfighterd on 127.0.0.1:28000 (master pointed at an
// unroutable address), relay.py, and a static server for the wasm page.
// Headless Chromium opens ?relay=ws://127.0.0.1:28080, rejects a wrong
// password, then joins with the real one.
//
//   BF_ROOT          repo root (default: two directories up from this file)
//   BF_SITE          directory with bitfighter.html/.js/.wasm/.data
//   BF_SERVER        native bitfighterd binary
//   BF_SCREENSHOT    where to write the in-game screenshot
//   PLAYWRIGHT_MODULE  absolute path to the playwright package, if it is
//                    not installed next to this script

import { spawn } from 'node:child_process';
import dgram from 'node:dgram';
import { createWriteStream, mkdirSync, readFileSync, symlinkSync, rmSync, readdirSync, existsSync, cpSync } from 'node:fs';
import { tmpdir } from 'node:os';
import net from 'node:net';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { setTimeout as delay } from 'node:timers/promises';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = process.env.BF_ROOT || path.resolve(here, '../..');
const site = process.env.BF_SITE || path.join(root, 'exe');
const serverBin = process.env.BF_SERVER || path.join(root, 'exe', 'bitfighterd');
const screenshotPath = process.env.BF_SCREENSHOT || path.join(root, 'lan-join.png');
const logDir = process.env.BF_LOG_DIR || path.join(root, 'lan-logs');
const httpPort = Number(process.env.BF_HTTP_PORT || 8091);
const relayPort = 28080;
const gamePort = 28123;
const password = 'lan-secret';
const wrongPassword = 'not-the-password';

const children = [];

function spawnLogged(cmd, args, logName) {
  mkdirSync(logDir, { recursive: true });
  const log = createWriteStream(path.join(logDir, logName));
  const child = spawn(cmd, args, {
    cwd: root,
    stdio: ['ignore', 'pipe', 'pipe'],
  });
  child.stdout.pipe(log);
  child.stderr.pipe(log);
  children.push(child);
  return child;
}

function stopAll() {
  for (const child of children) {
    if (child.exitCode === null && child.signalCode === null)
      child.kill('SIGTERM');
  }
}

function relayBoundPort() {
  const text = readText(path.join(logDir, 'relay.out'));
  const found = [...text.matchAll(/actual=(\d+)/g)];
  return found.length ? Number(found[found.length - 1][1]) : 0;
}

// Tell the relay the master named this host:port. Source port 25955 is what
// the relay treats as the master. The payload is the clear announcement.
function announceGamePort(boundPort) {
  return new Promise((resolve, reject) => {
    const sock = dgram.createSocket('udp4');
    const payload = Buffer.alloc(11);
    payload.writeUInt8(0xbf, 0);
    payload.writeUInt8(0xbf, 1);
    payload.writeUInt8(1, 2);
    payload.writeUInt16BE(1, 3);
    payload.writeUInt32BE(0x7f000001, 5);
    payload.writeUInt16BE(gamePort, 9);
    sock.once('error', reject);
    sock.bind(25955, '127.0.0.1', () => {
      sock.send(payload, boundPort, '127.0.0.1', (err) => {
        sock.close();
        if (err) reject(err);
        else resolve();
      });
    });
  });
}

function portOpen(port) {
  return new Promise((resolve) => {
    const sock = net.connect({ port, host: '127.0.0.1' }, () => {
      sock.end();
      resolve(true);
    });
    sock.on('error', () => resolve(false));
  });
}

function readText(file) {
  try {
    return readFileSync(file, 'utf8');
  } catch {
    return '';
  }
}

async function waitFor(label, predicate, timeoutMs) {
  const start = Date.now();
  while (Date.now() - start < timeoutMs) {
    if (await predicate())
      return;
    await delay(250);
  }
  throw new Error('timed out waiting for ' + label);
}

function prepareDataDir() {
  const dir = path.join(tmpdir(), 'bitfighter-lan-' + process.pid);
  rmSync(dir, { recursive: true, force: true });
  mkdirSync(dir);
  const resource = path.join(root, 'resource');
  for (const name of readdirSync(resource)) {
    if (name.endsWith('.ini') || name.endsWith('.log'))
      continue;
    symlinkSync(path.join(resource, name), path.join(dir, name));
  }
  return dir;
}

const playwrightMod = await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const chromium = playwrightMod.chromium || playwrightMod.default.chromium;

const dataDir = prepareDataDir();
const serverLog = path.join(dataDir, 'bitfighter_server.log');
const gameLog = path.join(dataDir, 'bitfighter.log');
let browser;

try {
  if (!existsSync(serverBin))
    throw new Error('bitfighterd not found at ' + serverBin);
  if (!existsSync(path.join(site, 'bitfighter.html')))
    throw new Error('wasm page not found in ' + site);

  spawnLogged(serverBin, [
    '-dedicated',
    '-rootdatadir', dataDir,
    '-serverpassword', password,
    '-hostaddr', 'IP:127.0.0.1:' + gamePort,
    '-master', 'IP:127.0.0.1:9',
    '-hostname', 'LanTest',
  ], 'bitfighterd.out');

  await waitFor('dedicated server', () => readText(serverLog).includes('Done.'), 30000);

  spawnLogged('python3', [path.join(root, 'tools/udp-ws-relay/relay.py'), String(relayPort)], 'relay.out');
  await waitFor('relay', () => portOpen(relayPort), 10000);

  spawnLogged('python3', ['-m', 'http.server', String(httpPort), '--bind', '127.0.0.1', '--directory', site], 'http.out');
  await waitFor('web server', () => portOpen(httpPort), 10000);

  // headless: false selects Playwright's full Chromium. --headless=new then
  // puts that binary in headless mode. The headless shell drops canvas
  // keyboard focus, so menu shortcuts never arrive.
  browser = await chromium.launch({
    headless: false,
    args: [
      '--headless=new',
      '--no-sandbox',
      '--disable-dev-shm-usage',
      '--use-gl=angle',
      '--use-angle=swiftshader',
      '--enable-webgl',
      '--ignore-gpu-blocklist',
      '--enable-unsafe-swiftshader',
    ],
  });
  const page = await browser.newPage({ viewport: { width: 1280, height: 900 } });
  page.on('console', (msg) => {
    const text = msg.text();
    if (text.includes('drawArrays') || text.includes('wall tile') || text.includes('music decoder') || text.includes('AudioContext'))
      return;
    console.log('[page]', text.slice(0, 240));
  });

  const url = 'http://127.0.0.1:' + httpPort + '/bitfighter.html'
    + '?relay=ws://127.0.0.1:' + relayPort
    + '&master=bitfighter.invalid:25955'
    + '&name=WebPilot';
  await page.goto(url, { waitUntil: 'domcontentloaded', timeout: 120000 });

  const output = page.locator('#output');
  await waitFor('client startup', async () => (await output.inputValue()).includes('Welcome to Bitfighter'), 90000);

  const canvas = page.locator('#canvas');
  // J is the JOIN shortcut. The main menu may not be taking keys on the
  // first frame, and a typed "o" on that menu opens OPTIONS, so retry J
  // until the server browser actually logs that it opened.
  await waitFor('server browser', async () => {
    await canvas.focus();
    await page.keyboard.press('j');
    await delay(400);
    return (await output.inputValue()).includes('Server browser');
  }, 20000);
  await waitFor('relay socket', () => relayBoundPort() > 0, 10000);
  await announceGamePort(relayBoundPort());
  await delay(200);
  await page.keyboard.type('/connect IP:127.0.0.1:' + gamePort, { delay: 20 });
  await page.keyboard.press('Enter');

  async function passwordPrompts() {
    const text = await output.inputValue();
    return text.split('Server requires a password').length - 1;
  }

  await waitFor('password prompt', async () => (await passwordPrompts()) >= 1, 30000);
  await canvas.focus();
  await page.keyboard.type(wrongPassword, { delay: 20 });
  await page.keyboard.press('Enter');
  await waitFor('wrong password rejected', async () => (await passwordPrompts()) >= 2, 30000);

  const midway = readText(gameLog);
  if (midway.includes('client "WebPilot" connected'))
    throw new Error('wrong password was accepted:\n' + midway);

  await canvas.focus();
  await page.keyboard.type(password, { delay: 20 });
  await page.keyboard.press('Enter');

  await waitFor('client joined', () => {
    const server = readText(serverLog);
    const game = readText(gameLog);
    return server.includes('WebPilot') && server.includes('joined')
      && game.includes('client "WebPilot" connected');
  }, 30000);

  await delay(1500);
  await page.screenshot({ path: screenshotPath, fullPage: true });
  console.log('joined; screenshot ' + screenshotPath);
} catch (err) {
  console.error(err);
  if (browser) {
    const pages = browser.contexts?.() || [];
    try {
      const page = browser.contexts()[0]?.pages()[0];
      if (page)
        await page.screenshot({ path: screenshotPath, fullPage: true });
    } catch {
      // The failure is already reported.
    }
    void pages;
  }
  process.exitCode = 1;
} finally {
  if (browser)
    await browser.close().catch(() => {});
  stopAll();
  await delay(300);
  try {
    cpSync(serverLog, path.join(logDir, 'bitfighter_server.log'));
    cpSync(gameLog, path.join(logDir, 'bitfighter.log'));
  } catch {
    // Logs are still in the data dir if the server never opened them.
  }
  rmSync(dataDir, { recursive: true, force: true });
}
