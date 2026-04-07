#pragma once

namespace {

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Esplora LoRa Lab</title>
  <style>
    :root {
      --bg: #061017;
      --bg-soft: #0c1d27;
      --card: rgba(15, 36, 48, 0.88);
      --card-strong: rgba(21, 49, 64, 0.96);
      --line: rgba(122, 167, 184, 0.18);
      --text: #f2f7f5;
      --muted: #8faab7;
      --accent: #ff8c42;
      --accent-soft: #ffc36a;
      --teal: #37d4c5;
      --green: #9be564;
      --warn: #ffd166;
      --danger: #ff6b6b;
      --shadow: 0 24px 80px rgba(0, 0, 0, 0.28);
      --radius: 22px;
      --mono: "Consolas", "Courier New", monospace;
      --sans: "Trebuchet MS", "Segoe UI", sans-serif;
    }

    * {
      box-sizing: border-box;
    }

    html, body {
      margin: 0;
      padding: 0;
      min-height: 100%;
      font-family: var(--sans);
      color: var(--text);
      background:
        radial-gradient(circle at 12% 18%, rgba(55, 212, 197, 0.16), transparent 22%),
        radial-gradient(circle at 88% 14%, rgba(255, 140, 66, 0.18), transparent 24%),
        radial-gradient(circle at 80% 78%, rgba(155, 229, 100, 0.12), transparent 20%),
        linear-gradient(160deg, #051016 0%, #0b1b24 40%, #08121b 100%);
    }

    body {
      padding: 24px;
    }

    .shell {
      width: min(1380px, 100%);
      margin: 0 auto;
      display: grid;
      gap: 18px;
      animation: rise 0.75s ease-out both;
    }

    .hero {
      position: relative;
      overflow: hidden;
      border: 1px solid var(--line);
      border-radius: 30px;
      padding: 26px;
      background:
        linear-gradient(135deg, rgba(55, 212, 197, 0.14), transparent 38%),
        linear-gradient(135deg, rgba(255, 140, 66, 0.16), transparent 55%),
        linear-gradient(180deg, rgba(13, 33, 45, 0.96), rgba(8, 19, 27, 0.96));
      box-shadow: var(--shadow);
    }

    .hero::after {
      content: "";
      position: absolute;
      inset: auto -60px -80px auto;
      width: 240px;
      height: 240px;
      border-radius: 50%;
      background: radial-gradient(circle, rgba(255, 195, 106, 0.2), transparent 65%);
      pointer-events: none;
    }

    .hero-top {
      display: flex;
      justify-content: space-between;
      align-items: flex-start;
      gap: 18px;
      flex-wrap: wrap;
    }

    .eyebrow {
      margin: 0 0 10px;
      letter-spacing: 0.18em;
      text-transform: uppercase;
      color: var(--accent-soft);
      font-size: 12px;
    }

    h1 {
      margin: 0;
      font-size: clamp(34px, 5vw, 64px);
      line-height: 0.92;
      letter-spacing: -0.04em;
      max-width: 620px;
    }

    .subtitle {
      max-width: 720px;
      margin: 14px 0 0;
      color: var(--muted);
      font-size: 15px;
      line-height: 1.65;
    }

    .hero-badges,
    .inline-actions,
    .chip-row {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
    }

    .hero-badges {
      margin-top: 22px;
    }

    .badge,
    .chip,
    button,
    .button-link {
      border-radius: 999px;
      border: 1px solid transparent;
    }

    .badge {
      padding: 10px 14px;
      font-size: 13px;
      color: var(--text);
      background: rgba(255, 255, 255, 0.06);
      border-color: rgba(255, 255, 255, 0.08);
    }

    .badge strong {
      color: var(--accent-soft);
    }

    .grid {
      display: grid;
      gap: 18px;
      grid-template-columns: 1.1fr 0.9fr;
    }

    .column {
      display: grid;
      gap: 18px;
    }

    .panel {
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: var(--radius);
      box-shadow: var(--shadow);
      overflow: hidden;
    }

    .panel-head {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 12px;
      padding: 18px 20px 12px;
    }

    .panel h2 {
      margin: 0;
      font-size: 18px;
      letter-spacing: -0.02em;
    }

    .panel-sub {
      color: var(--muted);
      font-size: 13px;
    }

    .panel-body {
      padding: 0 20px 20px;
    }

    .stats {
      display: grid;
      grid-template-columns: repeat(4, minmax(0, 1fr));
      gap: 12px;
      margin-top: 18px;
    }

    .stat {
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid rgba(255, 255, 255, 0.06);
      border-radius: 20px;
      padding: 16px;
    }

    .stat-label {
      color: var(--muted);
      font-size: 12px;
      text-transform: uppercase;
      letter-spacing: 0.12em;
      margin-bottom: 10px;
    }

    .stat-value {
      font-size: clamp(22px, 3vw, 34px);
      line-height: 0.95;
      letter-spacing: -0.04em;
    }

    .stat-meta {
      margin-top: 10px;
      color: var(--muted);
      font-size: 13px;
    }

    .chat-layout,
    .control-grid,
    .config-grid {
      display: grid;
      gap: 14px;
    }

    .chat-layout {
      grid-template-columns: 1fr auto;
      align-items: end;
    }

    textarea,
    input,
    select {
      width: 100%;
      padding: 14px 16px;
      color: var(--text);
      background: rgba(5, 13, 18, 0.65);
      border: 1px solid rgba(143, 170, 183, 0.18);
      border-radius: 18px;
      outline: none;
      font: inherit;
      transition: border-color 0.18s ease, transform 0.18s ease, background 0.18s ease;
    }

    textarea {
      min-height: 104px;
      resize: vertical;
    }

    textarea:focus,
    input:focus,
    select:focus {
      border-color: rgba(55, 212, 197, 0.7);
      background: rgba(8, 20, 28, 0.9);
      transform: translateY(-1px);
    }

    button,
    .button-link {
      cursor: pointer;
      padding: 12px 16px;
      color: var(--text);
      background: linear-gradient(135deg, var(--accent), #ff6f61);
      border-color: rgba(255, 255, 255, 0.08);
      font: inherit;
      font-weight: 700;
      transition: transform 0.16s ease, filter 0.16s ease, opacity 0.16s ease;
      text-decoration: none;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
    }

    button:hover,
    .button-link:hover {
      transform: translateY(-1px);
      filter: brightness(1.03);
    }

    button.secondary {
      background: linear-gradient(135deg, rgba(55, 212, 197, 0.28), rgba(55, 212, 197, 0.12));
      color: var(--text);
    }

    button.ghost {
      background: rgba(255, 255, 255, 0.04);
      color: var(--muted);
    }

    .chip {
      padding: 10px 14px;
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid rgba(255, 255, 255, 0.08);
      color: var(--muted);
      cursor: pointer;
      transition: all 0.16s ease;
    }

    .chip.active {
      color: var(--text);
      border-color: rgba(255, 195, 106, 0.45);
      background: rgba(255, 140, 66, 0.16);
    }

    .scanner {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(240px, 1fr));
      gap: 14px;
    }

    .node-card {
      position: relative;
      border-radius: 20px;
      padding: 16px;
      background:
        linear-gradient(180deg, rgba(255, 255, 255, 0.04), rgba(255, 255, 255, 0.02)),
        rgba(8, 21, 29, 0.85);
      border: 1px solid rgba(255, 255, 255, 0.07);
      min-height: 164px;
      overflow: hidden;
    }

    .node-card::before {
      content: "";
      position: absolute;
      inset: auto -40px -50px auto;
      width: 160px;
      height: 160px;
      border-radius: 50%;
      background: radial-gradient(circle, rgba(55, 212, 197, 0.16), transparent 68%);
    }

    .node-top,
    .line-item,
    .metric-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      gap: 10px;
    }

    .node-name {
      font-size: 18px;
      font-weight: 700;
      letter-spacing: -0.02em;
    }

    .node-id,
    .mono,
    .log-line {
      font-family: var(--mono);
    }

    .node-id {
      color: var(--muted);
      font-size: 12px;
    }

    .signal-bar {
      position: relative;
      width: 100%;
      height: 8px;
      margin: 12px 0 16px;
      border-radius: 999px;
      overflow: hidden;
      background: rgba(255, 255, 255, 0.06);
    }

    .signal-fill {
      height: 100%;
      border-radius: inherit;
      background: linear-gradient(90deg, var(--danger), var(--warn), var(--green), var(--teal));
      transform-origin: left center;
    }

    .metric-grid {
      display: grid;
      gap: 8px;
    }

    .metric-key {
      color: var(--muted);
      font-size: 12px;
      text-transform: uppercase;
      letter-spacing: 0.1em;
    }

    .metric-value {
      font-weight: 700;
    }

    .logs {
      background: rgba(2, 8, 12, 0.5);
      border-radius: 18px;
      border: 1px solid rgba(255, 255, 255, 0.06);
      min-height: 360px;
      max-height: 520px;
      overflow: auto;
      padding: 14px;
    }

    .log-line {
      padding: 10px 12px;
      margin-bottom: 8px;
      border-radius: 14px;
      font-size: 12px;
      line-height: 1.55;
      white-space: pre-wrap;
      word-break: break-word;
      background: rgba(255, 255, 255, 0.03);
      border: 1px solid rgba(255, 255, 255, 0.05);
      color: #e8f5f0;
    }

    .log-line.evt { border-left: 4px solid var(--teal); }
    .log-line.err { border-left: 4px solid var(--danger); }
    .log-line.warn { border-left: 4px solid var(--warn); }
    .log-line.ok { border-left: 4px solid var(--green); }
    .log-line.log { border-left: 4px solid var(--accent); }

    .mini-note {
      color: var(--muted);
      font-size: 12px;
      line-height: 1.5;
    }

    .dot {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: var(--muted);
      box-shadow: 0 0 0 rgba(255, 255, 255, 0);
    }

    .dot.live {
      background: var(--green);
      box-shadow: 0 0 20px rgba(155, 229, 100, 0.45);
    }

    .dot.warn {
      background: var(--warn);
      box-shadow: 0 0 20px rgba(255, 209, 102, 0.35);
    }

    .dot.off {
      background: var(--danger);
      box-shadow: 0 0 20px rgba(255, 107, 107, 0.24);
    }

    .pill {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 9px 12px;
      border-radius: 999px;
      font-size: 12px;
      border: 1px solid rgba(255, 255, 255, 0.08);
      background: rgba(255, 255, 255, 0.04);
    }

    .footer-note {
      padding: 4px 2px 0;
      color: var(--muted);
      font-size: 12px;
      text-align: right;
    }

    @keyframes rise {
      from {
        opacity: 0;
        transform: translateY(18px);
      }
      to {
        opacity: 1;
        transform: translateY(0);
      }
    }

    @media (max-width: 1080px) {
      .grid {
        grid-template-columns: 1fr;
      }

      .stats {
        grid-template-columns: repeat(2, minmax(0, 1fr));
      }
    }

    @media (max-width: 720px) {
      body {
        padding: 16px;
      }

      .hero {
        padding: 20px;
      }

      .stats {
        grid-template-columns: 1fr;
      }

      .chat-layout {
        grid-template-columns: 1fr;
      }

      .panel-head,
      .panel-body {
        padding-left: 16px;
        padding-right: 16px;
      }
    }
  </style>
</head>
<body>
  <main class="shell">
    <section class="hero">
      <div class="hero-top">
        <div>
          <p class="eyebrow">LoRa Lab Node</p>
          <h1 id="hero-title">Esplora Control Deck</h1>
          <p class="subtitle">
            Live control surface for your XIAO ESP32S3 + Wio-SX1262 node. Watch the ether,
            chat over LoRa, inspect nearby nodes, and bridge the telemetry into Home Assistant over MQTT.
          </p>
          <div class="hero-badges">
            <div class="badge"><strong>Node</strong> <span id="badge-id">-</span></div>
            <div class="badge"><strong>IP</strong> <span id="badge-ip">-</span></div>
            <div class="badge"><strong>Profile</strong> <span id="badge-profile">-</span></div>
            <div class="badge"><strong>Air</strong> <span id="badge-air">868.100 / SF9</span></div>
          </div>
        </div>
        <div class="chip-row">
          <div class="pill"><span class="dot" id="dot-radio"></span> <span id="pill-radio">Radio offline</span></div>
          <div class="pill"><span class="dot" id="dot-wifi"></span> <span id="pill-wifi">Wi-Fi idle</span></div>
          <div class="pill"><span class="dot" id="dot-mqtt"></span> <span id="pill-mqtt">MQTT idle</span></div>
        </div>
      </div>
      <div class="stats">
        <article class="stat">
          <div class="stat-label">Neighbors</div>
          <div class="stat-value" id="stat-neighbors">0</div>
          <div class="stat-meta" id="stat-neighbors-meta">No compatible nodes yet</div>
        </article>
        <article class="stat">
          <div class="stat-label">Wi-Fi RSSI</div>
          <div class="stat-value" id="stat-wifi-rssi">--</div>
          <div class="stat-meta" id="stat-wifi-meta">Waiting for STA link</div>
        </article>
        <article class="stat">
          <div class="stat-label">Last RX</div>
          <div class="stat-value" id="stat-last-rx">--</div>
          <div class="stat-meta" id="stat-last-rx-meta">Nothing decoded yet</div>
        </article>
        <article class="stat">
          <div class="stat-label">Uptime</div>
          <div class="stat-value" id="stat-uptime">--</div>
          <div class="stat-meta" id="stat-uptime-meta">Fresh boot</div>
        </article>
      </div>
    </section>

    <section class="grid">
      <div class="column">
        <section class="panel">
          <div class="panel-head">
            <div>
              <h2>LoRa Chat</h2>
              <div class="panel-sub">Send a short message into the ether or probe for another Esplora node.</div>
            </div>
            <div class="inline-actions">
              <button class="secondary" id="ping-btn" type="button">Ping</button>
              <button class="ghost" id="beacon-now-btn" type="button">Beacon Now</button>
            </div>
          </div>
          <div class="panel-body">
            <div class="chat-layout">
              <textarea id="chat-text" maxlength="80" placeholder="Type a short LoRa message, then send it."></textarea>
              <button id="send-btn" type="button">Send Message</button>
            </div>
            <p class="mini-note">Messages are short by design. LoRa rewards discipline: lower payload size means less airtime and better coexistence.</p>
          </div>
        </section>

        <section class="panel">
          <div class="panel-head">
            <div>
              <h2>Scanner</h2>
              <div class="panel-sub">Compatible nodes detected on the current LoRa settings.</div>
            </div>
            <button class="secondary" id="cad-btn" type="button">CAD Scan</button>
          </div>
          <div class="panel-body">
            <div class="scanner" id="scanner"></div>
          </div>
        </section>

        <section class="panel">
          <div class="panel-head">
            <div>
              <h2>Live Tape</h2>
              <div class="panel-sub">Recent radio, Wi-Fi and command events from the node.</div>
            </div>
            <div class="inline-actions">
              <button class="ghost" id="clear-log-btn" type="button">Clear View</button>
            </div>
          </div>
          <div class="panel-body">
            <div class="logs" id="logs"></div>
          </div>
        </section>
      </div>

      <div class="column">
        <section class="panel">
          <div class="panel-head">
            <div>
              <h2>Radio Controls</h2>
              <div class="panel-sub">Fast profile shifts and radio toggles without touching serial.</div>
            </div>
          </div>
          <div class="panel-body">
            <div class="chip-row" id="profile-row">
              <button class="chip" data-profile="fast" type="button">Fast</button>
              <button class="chip" data-profile="balanced" type="button">Balanced</button>
              <button class="chip" data-profile="long" type="button">Long Range</button>
            </div>
            <div class="control-grid" style="margin-top:14px;">
              <div class="inline-actions">
                <button class="secondary" id="beacon-toggle-btn" type="button">Toggle Beacon</button>
                <button class="ghost" id="raw-toggle-btn" type="button">Toggle Raw Logs</button>
              </div>
              <div class="mini-note" id="radio-summary">Waiting for state.</div>
            </div>
          </div>
        </section>

        <section class="panel">
          <div class="panel-head">
            <div>
              <h2>Wi-Fi</h2>
              <div class="panel-sub">Static IP target is fixed in firmware at <span class="mono">192.168.1.201</span>.</div>
            </div>
          </div>
          <div class="panel-body">
            <div class="config-grid">
              <input id="wifi-ssid" placeholder="SSID">
              <input id="wifi-pass" placeholder="Password" type="password">
              <div class="inline-actions">
                <button id="wifi-save-btn" type="button">Save + Connect</button>
                <button class="ghost" id="wifi-off-btn" type="button">Wi-Fi Off</button>
              </div>
              <div class="mini-note" id="wifi-summary">No STA link yet.</div>
            </div>
          </div>
        </section>

        <section class="panel">
          <div class="panel-head">
            <div>
              <h2>MQTT / Home Assistant</h2>
              <div class="panel-sub">Configure the broker, then let the node publish telemetry and discovery topics for HA.</div>
            </div>
          </div>
          <div class="panel-body">
            <div class="config-grid">
              <input id="mqtt-host" placeholder="Broker host or IP">
              <div class="chat-layout">
                <input id="mqtt-port" placeholder="Port" type="number" min="1" max="65535">
                <button class="secondary" id="mqtt-connect-btn" type="button">Connect</button>
              </div>
              <input id="mqtt-user" placeholder="Username">
              <input id="mqtt-pass" placeholder="Password" type="password">
              <input id="mqtt-topic" placeholder="Base topic, e.g. esplora/61AE3D98">
              <div class="inline-actions">
                <button id="mqtt-save-btn" type="button">Save MQTT</button>
                <button class="ghost" id="mqtt-toggle-btn" type="button">Enable / Disable</button>
                <button class="ghost" id="mqtt-ha-btn" type="button">Toggle HA Discovery</button>
              </div>
              <div class="mini-note" id="mqtt-summary">Broker not configured.</div>
            </div>
          </div>
        </section>
      </div>
    </section>
    <div class="footer-note">Esplora panel polls the node directly. No cloud, no extra backend, just the device at your static IP.</div>
  </main>

  <script>
    const model = {
      lastLogSeq: 0,
      logs: [],
      status: {},
      wifi: {},
      mqtt: {},
      neighbors: []
    };

    const ids = [
      "hero-title", "badge-id", "badge-ip", "badge-profile", "badge-air",
      "stat-neighbors", "stat-neighbors-meta", "stat-wifi-rssi", "stat-wifi-meta",
      "stat-last-rx", "stat-last-rx-meta", "stat-uptime", "stat-uptime-meta",
      "dot-radio", "dot-wifi", "dot-mqtt", "pill-radio", "pill-wifi", "pill-mqtt",
      "scanner", "logs", "chat-text", "radio-summary", "wifi-summary", "mqtt-summary",
      "wifi-ssid", "wifi-pass", "mqtt-host", "mqtt-port", "mqtt-user", "mqtt-pass", "mqtt-topic"
    ];
    const el = Object.fromEntries(ids.map((id) => [id, document.getElementById(id)]));

    function fmtSeconds(total) {
      const sec = Number(total || 0);
      const h = Math.floor(sec / 3600);
      const m = Math.floor((sec % 3600) / 60);
      const s = sec % 60;
      if (h > 0) return `${h}h ${m}m`;
      if (m > 0) return `${m}m ${s}s`;
      return `${s}s`;
    }

    function clsForLine(line) {
      if (line.startsWith("ERR|")) return "err";
      if (line.startsWith("WARN|")) return "warn";
      if (line.startsWith("OK|")) return "ok";
      if (line.startsWith("LOG|")) return "log";
      return "evt";
    }

    function dotState(target, state) {
      target.className = "dot " + state;
    }

    function syncInputValue(target, value) {
      if (document.activeElement === target) return;
      target.value = value ?? "";
    }

    async function postForm(url, fields) {
      const body = new URLSearchParams();
      Object.entries(fields).forEach(([key, value]) => body.set(key, value));
      const res = await fetch(url, {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body
      });
      if (!res.ok) {
        throw new Error(await res.text());
      }
      return res.json();
    }

    async function sendCommand(cmd) {
      await postForm("/api/command", { cmd });
      await refresh(true);
    }

    function renderProfiles(profile) {
      document.querySelectorAll("[data-profile]").forEach((button) => {
        button.classList.toggle("active", button.dataset.profile === profile);
      });
    }

    function renderLogs(logs) {
      if (!logs.length && !model.logs.length) {
        el.logs.innerHTML = '<div class="log-line log">No logs yet. Try ping, beacon, or a chat message.</div>';
        return;
      }

      logs.forEach((entry) => {
        model.lastLogSeq = Math.max(model.lastLogSeq, Number(entry.seq || 0));
        model.logs.push(entry);
      });

      model.logs = model.logs.slice(-120);
      el.logs.innerHTML = model.logs.map((entry) => {
        const line = entry.line || "";
        return `<div class="log-line ${clsForLine(line)}">#${entry.seq} ${line}</div>`;
      }).join("");
      el.logs.scrollTop = el.logs.scrollHeight;
    }

    function renderNeighbors(neighbors) {
      if (!neighbors.length) {
        el.scanner.innerHTML = '<div class="node-card"><div class="node-name">No nearby Esplora nodes</div><p class="mini-note">The node is listening and beaconing. If another module speaks the same profile, it will appear here automatically.</p></div>';
        return;
      }

      el.scanner.innerHTML = neighbors.map((node) => {
        const rssi = Number(node.rssi || -140);
        const quality = Math.max(0, Math.min(100, Math.round((rssi + 120) * 1.4)));
        return `
          <article class="node-card">
            <div class="node-top">
              <div>
                <div class="node-name">${node.name || "Unknown"}</div>
                <div class="node-id">${node.id || "?"}</div>
              </div>
              <div class="pill"><span class="dot live"></span>${node.lastKind || "?"}</div>
            </div>
            <div class="signal-bar"><div class="signal-fill" style="width:${quality}%"></div></div>
            <div class="metric-grid">
              <div class="metric-row"><span class="metric-key">Distance</span><span class="metric-value">${node.distanceM} m</span></div>
              <div class="metric-row"><span class="metric-key">RSSI</span><span class="metric-value">${node.rssi} dBm</span></div>
              <div class="metric-row"><span class="metric-key">SNR</span><span class="metric-value">${node.snr} dB</span></div>
              <div class="metric-row"><span class="metric-key">Age</span><span class="metric-value">${node.ageSec} s</span></div>
            </div>
          </article>`;
      }).join("");
    }

    function render(data) {
      const payload = data.payload || {};
      const status = payload.status || {};
      const wifi = payload.wifi || {};
      const mqtt = payload.mqtt || {};
      const lastRx = payload.lastRx || {};
      const hasLastRx = lastRx.ageSec !== null && lastRx.ageSec !== undefined;

      model.status = status;
      model.wifi = wifi;
      model.mqtt = mqtt;
      model.neighbors = data.neighbors || [];

      el["hero-title"].textContent = (status.name || "Esplora") + " / " + (status.id || "--");
      el["badge-id"].textContent = status.id || "--";
      el["badge-ip"].textContent = wifi.ip || wifi.targetIp || "--";
      el["badge-profile"].textContent = status.profile || "--";
      el["badge-air"].textContent = `${status.frequencyMhz || "--"} MHz / SF${status.spreadingFactor || "--"}`;

      dotState(el["dot-radio"], status.radioReady ? "live" : "off");
      dotState(el["dot-wifi"], wifi.connected ? "live" : "warn");
      dotState(el["dot-mqtt"], mqtt.connected ? "live" : (mqtt.enabled ? "warn" : "off"));
      el["pill-radio"].textContent = status.radioReady ? `LoRa ready on ${status.pinmap}` : "Radio offline";
      el["pill-wifi"].textContent = wifi.connected ? `Wi-Fi ${wifi.ip}` : "Wi-Fi idle";
      el["pill-mqtt"].textContent = mqtt.connected ? `MQTT ${mqtt.host}:${mqtt.port}` : (mqtt.enabled ? "MQTT retrying" : "MQTT disabled");

      el["stat-neighbors"].textContent = String(data.neighborCount || 0);
      el["stat-neighbors-meta"].textContent = data.neighborCount ? "Live compatible nodes in range" : "No compatible nodes yet";
      el["stat-wifi-rssi"].textContent = wifi.connected ? `${wifi.rssi} dBm` : "--";
      el["stat-wifi-meta"].textContent = wifi.connected ? `SSID ${wifi.ssid}` : "Static IP reserved, waiting for STA link";
      el["stat-last-rx"].textContent = hasLastRx ? `${lastRx.kind} / ${lastRx.from || "--"}` : "--";
      el["stat-last-rx-meta"].textContent = hasLastRx ? `${lastRx.rssi} dBm, ${lastRx.snr} dB` : "Nothing decoded yet";
      el["stat-uptime"].textContent = fmtSeconds(status.uptimeSec || 0);
      el["stat-uptime-meta"].textContent = status.radioReady ? "Node is active" : "Radio not initialized";

      el["radio-summary"].textContent = `Pinmap ${status.pinmap || "--"}, sync 0x${status.syncWord || "--"}, beacon ${status.beaconEnabled ? "on" : "off"}, raw logs ${status.rawLoggingEnabled ? "on" : "off"}.`;
      el["wifi-summary"].textContent = wifi.connected
        ? `Connected to ${wifi.ssid} with ${wifi.ip} (RSSI ${wifi.rssi} dBm).`
        : `Static target IP is ${wifi.targetIp || "192.168.1.201"}. Save credentials to bring the panel back after reboot.`;
      el["mqtt-summary"].textContent = mqtt.connected
        ? `Publishing on ${mqtt.baseTopic}. Home Assistant discovery is ${mqtt.haDiscovery ? "enabled" : "disabled"}.`
        : (mqtt.enabled ? `Broker ${mqtt.host}:${mqtt.port} configured, waiting to connect.` : "MQTT disabled or not configured yet.");

      syncInputValue(el["wifi-ssid"], wifi.ssid || "");
      syncInputValue(el["mqtt-host"], mqtt.host || "");
      syncInputValue(el["mqtt-port"], mqtt.port || 1883);
      syncInputValue(el["mqtt-user"], mqtt.user || "");
      syncInputValue(el["mqtt-topic"], mqtt.baseTopic || "");

      renderProfiles(status.profile || "");
      renderNeighbors(model.neighbors);
      renderLogs(data.logs || []);

      document.getElementById("beacon-toggle-btn").textContent = status.beaconEnabled ? "Disable Beacon" : "Enable Beacon";
      document.getElementById("raw-toggle-btn").textContent = status.rawLoggingEnabled ? "Disable Raw Logs" : "Enable Raw Logs";
      document.getElementById("mqtt-toggle-btn").textContent = mqtt.enabled ? "Disable MQTT" : "Enable MQTT";
      document.getElementById("mqtt-ha-btn").textContent = mqtt.haDiscovery ? "Disable HA Discovery" : "Enable HA Discovery";
    }

    async function refresh(force) {
      try {
        if (force) {
          model.logs = [];
          model.lastLogSeq = 0;
        }
        const since = force ? 0 : model.lastLogSeq;
        const res = await fetch(`/api/state?since=${since}`);
        const data = await res.json();
        render(data);
      } catch (error) {
        model.logs.push({ seq: model.lastLogSeq + 1, line: `ERR|panel refresh failed: ${error.message}` });
        renderLogs([]);
      }
    }

    document.getElementById("send-btn").addEventListener("click", async () => {
      const text = el["chat-text"].value.trim();
      if (!text) return;
      await postForm("/api/send", { text });
      el["chat-text"].value = "";
      await refresh(true);
    });

    document.getElementById("ping-btn").addEventListener("click", () => sendCommand("ping"));
    document.getElementById("beacon-now-btn").addEventListener("click", () => sendCommand("beacon now"));
    document.getElementById("cad-btn").addEventListener("click", () => sendCommand("cad"));
    document.getElementById("beacon-toggle-btn").addEventListener("click", () => sendCommand("beacon toggle"));
    document.getElementById("raw-toggle-btn").addEventListener("click", () => sendCommand("raw toggle"));
    document.getElementById("clear-log-btn").addEventListener("click", () => {
      model.logs = [];
      el.logs.innerHTML = "";
    });

    document.querySelectorAll("[data-profile]").forEach((button) => {
      button.addEventListener("click", () => sendCommand(`profile ${button.dataset.profile}`));
    });

    document.getElementById("wifi-save-btn").addEventListener("click", async () => {
      await postForm("/api/wifi", {
        ssid: el["wifi-ssid"].value.trim(),
        password: el["wifi-pass"].value,
        connect: "1"
      });
      el["wifi-pass"].value = "";
      await refresh(true);
    });

    document.getElementById("wifi-off-btn").addEventListener("click", async () => {
      await postForm("/api/wifi", { mode: "off" });
      await refresh(true);
    });

    document.getElementById("mqtt-save-btn").addEventListener("click", async () => {
      await postForm("/api/mqtt", {
        host: el["mqtt-host"].value.trim(),
        port: el["mqtt-port"].value.trim(),
        user: el["mqtt-user"].value.trim(),
        password: el["mqtt-pass"].value,
        topic: el["mqtt-topic"].value.trim()
      });
      el["mqtt-pass"].value = "";
      await refresh(true);
    });

    document.getElementById("mqtt-connect-btn").addEventListener("click", async () => {
      await postForm("/api/mqtt", {
        host: el["mqtt-host"].value.trim(),
        port: el["mqtt-port"].value.trim(),
        user: el["mqtt-user"].value.trim(),
        password: el["mqtt-pass"].value,
        topic: el["mqtt-topic"].value.trim()
      });
      el["mqtt-pass"].value = "";
      await sendCommand("mqtt on");
      await sendCommand("mqtt connect");
    });
    document.getElementById("mqtt-toggle-btn").addEventListener("click", () => sendCommand("mqtt toggle"));
    document.getElementById("mqtt-ha-btn").addEventListener("click", () => sendCommand("mqtt ha toggle"));

    setInterval(() => refresh(false), 1500);
    refresh(true);
  </script>
</body>
</html>
)rawliteral";

}  // namespace
