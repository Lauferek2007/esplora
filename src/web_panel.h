#pragma once

namespace {

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Esplora LoRa Panel</title>
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
      gap: 14px;
    }

    .table-wrap {
      overflow: auto;
      border-radius: 18px;
      border: 1px solid rgba(255, 255, 255, 0.06);
      background: rgba(5, 13, 18, 0.52);
    }

    .data-table {
      width: 100%;
      border-collapse: collapse;
      min-width: 720px;
      font-size: 13px;
    }

    .data-table th,
    .data-table td {
      padding: 12px 14px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.06);
      text-align: left;
      vertical-align: middle;
    }

    .data-table th {
      color: var(--accent-soft);
      font-size: 11px;
      letter-spacing: 0.12em;
      text-transform: uppercase;
      background: rgba(255, 255, 255, 0.03);
    }

    .data-table tr:last-child td {
      border-bottom: 0;
    }

    .data-table tbody tr:hover {
      background: rgba(255, 255, 255, 0.03);
    }

    .row-title {
      font-weight: 700;
      letter-spacing: -0.01em;
    }

    .row-sub {
      margin-top: 4px;
      color: var(--muted);
      font-size: 12px;
      font-family: var(--mono);
    }

    .row-badge {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      min-width: 52px;
      padding: 6px 10px;
      border-radius: 999px;
      font-size: 11px;
      font-weight: 700;
      letter-spacing: 0.08em;
      text-transform: uppercase;
      border: 1px solid rgba(255, 255, 255, 0.08);
      background: rgba(255, 255, 255, 0.05);
    }

    .row-badge.raw {
      color: var(--warn);
      border-color: rgba(255, 209, 102, 0.28);
      background: rgba(255, 209, 102, 0.10);
    }

    .row-badge.cad {
      color: var(--teal);
      border-color: rgba(55, 212, 197, 0.28);
      background: rgba(55, 212, 197, 0.10);
    }

    .row-badge.node {
      color: var(--green);
      border-color: rgba(155, 229, 100, 0.28);
      background: rgba(155, 229, 100, 0.10);
    }

    .empty-state {
      padding: 18px;
      border-radius: 18px;
      color: var(--muted);
      line-height: 1.6;
      border: 1px solid rgba(255, 255, 255, 0.06);
      background: rgba(255, 255, 255, 0.03);
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
          <p class="eyebrow">Węzeł LoRa</p>
          <h1 id="hero-title">Esplora Panel Sterowania</h1>
          <p class="subtitle">
            Panel na żywo dla XIAO ESP32S3 + Wio-SX1262. Możesz tu nadawać wiadomości LoRa,
            sprawdzać sąsiednie moduły, podglądać aktywność w eterze i mostkować telemetrię do Home Assistant po MQTT.
          </p>
          <div class="hero-badges">
            <div class="badge"><strong>Node ID</strong> <span id="badge-id">-</span></div>
            <div class="badge"><strong>IP</strong> <span id="badge-ip">-</span></div>
            <div class="badge"><strong>Profil</strong> <span id="badge-profile">-</span></div>
            <div class="badge"><strong>Eter</strong> <span id="badge-air">868.100 / SF9</span></div>
          </div>
        </div>
        <div class="chip-row">
          <div class="pill"><span class="dot" id="dot-radio"></span> <span id="pill-radio">Radio offline</span></div>
          <div class="pill"><span class="dot" id="dot-wifi"></span> <span id="pill-wifi">Wi‑Fi bez połączenia</span></div>
          <div class="pill"><span class="dot" id="dot-mqtt"></span> <span id="pill-mqtt">MQTT nieaktywne</span></div>
        </div>
      </div>
      <div class="stats">
        <article class="stat">
          <div class="stat-label">Sąsiedzi</div>
          <div class="stat-value" id="stat-neighbors">0</div>
          <div class="stat-meta" id="stat-neighbors-meta">Brak kompatybilnych modułów</div>
        </article>
        <article class="stat">
          <div class="stat-label">Wi‑Fi RSSI</div>
          <div class="stat-value" id="stat-wifi-rssi">--</div>
          <div class="stat-meta" id="stat-wifi-meta">Czekam na połączenie STA</div>
        </article>
        <article class="stat">
          <div class="stat-label">Ostatni RX</div>
          <div class="stat-value" id="stat-last-rx">--</div>
          <div class="stat-meta" id="stat-last-rx-meta">Nic jeszcze nie zdekodowano</div>
        </article>
        <article class="stat">
          <div class="stat-label">Uptime</div>
          <div class="stat-value" id="stat-uptime">--</div>
          <div class="stat-meta" id="stat-uptime-meta">Świeży start</div>
        </article>
      </div>
    </section>

    <section class="grid">
      <div class="column">
        <section class="panel">
          <div class="panel-head">
            <div>
              <h2>LoRa Chat</h2>
              <div class="panel-sub">Wyślij krótką wiadomość LoRa albo sprawdź, czy odpowiada inny moduł Esplora.</div>
            </div>
            <div class="inline-actions">
              <button class="secondary" id="ping-btn" type="button">Ping</button>
              <button class="ghost" id="beacon-now-btn" type="button">Nadaj Beacon</button>
            </div>
          </div>
          <div class="panel-body">
            <div class="chat-layout">
              <textarea id="chat-text" maxlength="80" placeholder="Wpisz krótką wiadomość LoRa i wyślij ją w eter."></textarea>
              <button id="send-btn" type="button">Wyślij</button>
            </div>
            <p class="mini-note">Wiadomości są celowo krótkie. Im mniejszy pakiet LoRa, tym krótszy czas zajęcia eteru i większa szansa, że inni też się przebiją.</p>
          </div>
        </section>

        <section class="panel">
          <div class="panel-head">
            <div>
              <h2>Scanner</h2>
              <div class="panel-sub">Tu masz dwie rzeczy: zdekodowane węzły Esplora oraz ogólną aktywność LoRa wykrytą na popularnych ustawieniach EU868.</div>
            </div>
            <div class="inline-actions">
              <button class="secondary" id="cad-btn" type="button">CAD (Kanał)</button>
              <button class="ghost" id="sweep-btn" type="button">Szybki Sweep</button>
            </div>
          </div>
          <div class="panel-body">
            <div class="mini-note" id="scanner-summary">Węzły Esplora zdekodowane na dokładnie tym samym profilu radiowym.</div>
            <div class="scanner" id="scanner"></div>
            <div class="mini-note" id="sightings-summary" style="margin-top:14px;">Tu pojawią się surowe wykrycia i trafienia sweepu, nawet jeśli obcy moduł nie używa protokołu Esplora.</div>
            <div class="scanner" id="sightings"></div>
          </div>
        </section>

        <section class="panel">
          <div class="panel-head">
            <div>
              <h2>Live Tape</h2>
              <div class="panel-sub">Ostatnie zdarzenia z radia, Wi‑Fi, MQTT i komend.</div>
            </div>
            <div class="inline-actions">
              <button class="ghost" id="clear-log-btn" type="button">Wyczyść Widok</button>
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
              <div class="panel-sub">Szybka zmiana profilu LoRa i przełączników bez wchodzenia na port szeregowy.</div>
            </div>
          </div>
          <div class="panel-body">
            <div class="chip-row" id="profile-row">
              <button class="chip" data-profile="fast" type="button">Szybki</button>
              <button class="chip" data-profile="balanced" type="button">Zrównoważony</button>
              <button class="chip" data-profile="long" type="button">Daleki Zasięg</button>
            </div>
            <div class="config-grid" style="margin-top:14px;">
              <div class="mini-note">Dla laika: profil zmienia szybkość i zasięg. Częstotliwość to kanał. Sieć prywatna/publiczna zmienia sync word, czyli to z jakim "językiem radiowym" próbujesz gadać.</div>
              <select id="air-freq">
                <option value="867.1">867.100 MHz</option>
                <option value="867.3">867.300 MHz</option>
                <option value="867.5">867.500 MHz</option>
                <option value="867.7">867.700 MHz</option>
                <option value="867.9">867.900 MHz</option>
                <option value="868.1" selected>868.100 MHz</option>
                <option value="868.3">868.300 MHz</option>
                <option value="868.5">868.500 MHz</option>
                <option value="869.525">869.525 MHz</option>
              </select>
              <select id="air-network">
                <option value="private" selected>Prywatna / DIY (sync 0x12)</option>
                <option value="public">Publiczna / LoRaWAN (sync 0x34)</option>
              </select>
              <button class="secondary" id="air-apply-btn" type="button">Ustaw Kanał i Sieć</button>
              <div class="mini-note" id="air-summary">Prywatna = własne moduły i eksperymenty. Publiczna = częściej spotykane ramki LoRaWAN/public.</div>
            </div>
            <div class="control-grid" style="margin-top:14px;">
              <div class="inline-actions">
                <button class="secondary" id="beacon-toggle-btn" type="button">Przełącz Beacon</button>
                <button class="ghost" id="raw-toggle-btn" type="button">Przełącz RAW</button>
              </div>
              <div class="mini-note" id="radio-summary">Czekam na stan radia.</div>
            </div>
          </div>
        </section>

        <section class="panel">
          <div class="panel-head">
            <div>
              <h2>Wi-Fi</h2>
              <div class="panel-sub">Stałe IP jest wpisane w firmware: <span class="mono">192.168.1.201</span>.</div>
            </div>
          </div>
          <div class="panel-body">
            <div class="config-grid">
              <input id="wifi-ssid" placeholder="Nazwa sieci Wi‑Fi (SSID)">
              <input id="wifi-pass" placeholder="Hasło Wi‑Fi" type="password">
              <div class="inline-actions">
                <button id="wifi-save-btn" type="button">Zapisz i Połącz</button>
                <button class="ghost" id="wifi-off-btn" type="button">Wyłącz Wi‑Fi</button>
              </div>
              <div class="mini-note" id="wifi-summary">Brak połączenia STA.</div>
            </div>
          </div>
        </section>

        <section class="panel">
          <div class="panel-head">
            <div>
              <h2>MQTT / Home Assistant</h2>
              <div class="panel-sub">Ustaw broker, a moduł zacznie publikować telemetrię i discovery dla Home Assistant.</div>
            </div>
          </div>
          <div class="panel-body">
            <div class="config-grid">
              <input id="mqtt-host" placeholder="Host lub IP brokera MQTT">
              <div class="chat-layout">
                <input id="mqtt-port" placeholder="Port" type="number" min="1" max="65535">
                <button class="secondary" id="mqtt-connect-btn" type="button">Połącz</button>
              </div>
              <input id="mqtt-user" placeholder="Użytkownik MQTT">
              <input id="mqtt-pass" placeholder="Hasło MQTT" type="password">
              <input id="mqtt-topic" placeholder="Temat bazowy, np. esplora/61AE3D98">
              <div class="inline-actions">
                <button id="mqtt-save-btn" type="button">Zapisz MQTT</button>
                <button class="ghost" id="mqtt-toggle-btn" type="button">Włącz / Wyłącz</button>
                <button class="ghost" id="mqtt-ha-btn" type="button">Discovery HA</button>
              </div>
              <div class="mini-note" id="mqtt-summary">Broker nie jest jeszcze ustawiony.</div>
            </div>
          </div>
        </section>
      </div>
    </section>
    <div class="footer-note">Panel łączy się bezpośrednio z modułem. Bez chmury, bez dodatkowego backendu, tylko urządzenie pod Twoim stałym IP.</div>
  </main>

  <script>
    const model = {
      lastLogSeq: 0,
      logs: [],
      status: {},
      wifi: {},
      mqtt: {},
      neighbors: [],
      sightings: []
    };

    const ids = [
      "hero-title", "badge-id", "badge-ip", "badge-profile", "badge-air",
      "stat-neighbors", "stat-neighbors-meta", "stat-wifi-rssi", "stat-wifi-meta",
      "stat-last-rx", "stat-last-rx-meta", "stat-uptime", "stat-uptime-meta",
      "dot-radio", "dot-wifi", "dot-mqtt", "pill-radio", "pill-wifi", "pill-mqtt",
      "scanner", "scanner-summary", "sightings", "sightings-summary", "logs", "chat-text", "radio-summary", "air-summary", "wifi-summary", "mqtt-summary",
      "wifi-ssid", "wifi-pass", "mqtt-host", "mqtt-port", "mqtt-user", "mqtt-pass", "mqtt-topic", "air-freq", "air-network"
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

    async function sendCommands(commands) {
      for (const cmd of commands) {
        await postForm("/api/command", { cmd });
      }
      await refresh(true);
    }

    function renderProfiles(profile) {
      document.querySelectorAll("[data-profile]").forEach((button) => {
        button.classList.toggle("active", button.dataset.profile === profile);
      });
    }

    function renderLogs(logs) {
      if (!logs.length && !model.logs.length) {
        el.logs.innerHTML = '<div class="log-line log">Brak logów. Spróbuj ping, beacon albo wiadomość LoRa.</div>';
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
      const sorted = [...neighbors].sort((a, b) => {
        const ageDiff = Number(a.ageSec || 999999) - Number(b.ageSec || 999999);
        if (ageDiff !== 0) return ageDiff;
        return Number(b.rssi || -999) - Number(a.rssi || -999);
      });

      if (!sorted.length) {
        el.scanner.innerHTML = '<div class="empty-state">Brak zdekodowanych węzłów Esplora. Ta tabela pokazuje tylko moduły mówiące tym samym formatem pakietów, na tej samej częstotliwości i z tym samym profilem LoRa.</div>';
        return;
      }

      el.scanner.innerHTML = `
        <div class="table-wrap">
          <table class="data-table">
            <thead>
              <tr>
                <th>Moduł</th>
                <th>Ostatni</th>
                <th>RSSI</th>
                <th>SNR</th>
                <th>Dystans</th>
                <th>Wiek</th>
              </tr>
            </thead>
            <tbody>
              ${sorted.map((node) => `
                <tr>
                  <td>
                    <div class="row-title">${node.name || "Nieznany"}</div>
                    <div class="row-sub">${node.id || "?"}</div>
                  </td>
                  <td><span class="row-badge node">${node.lastKind || "?"}</span></td>
                  <td>${node.rssi} dBm</td>
                  <td>${node.snr} dB</td>
                  <td>${node.distanceM} m</td>
                  <td>${fmtSeconds(node.ageSec || 0)}</td>
                </tr>`).join("")}
            </tbody>
          </table>
        </div>`;
    }

    function renderSightings(sightings) {
      const sorted = [...sightings].sort((a, b) => {
        const ageDiff = Number(a.ageSec || 999999) - Number(b.ageSec || 999999);
        if (ageDiff !== 0) return ageDiff;
        return Number(b.hits || 0) - Number(a.hits || 0);
      });

      if (!sorted.length) {
        el.sightings.innerHTML = '<div class="empty-state">Na razie nic ogólnego nie wykryto. Użyj <strong>Szybki Sweep</strong>, żeby przeskanować popularne kanały EU868 i wykryć preambuły LoRa, nawet jeśli obce urządzenie nie jest modułem Esplora.</div>';
        return;
      }

      el.sightings.innerHTML = `
        <div class="table-wrap">
          <table class="data-table">
            <thead>
              <tr>
                <th>Typ</th>
                <th>Eter</th>
                <th>Sygnał</th>
                <th>Trafienia</th>
                <th>Wiek</th>
                <th>Opis</th>
              </tr>
            </thead>
            <tbody>
              ${sorted.map((item) => {
                const typeCls = (item.type || "").toLowerCase();
                const rssi = item.rssi === null || item.rssi === undefined ? "--" : `${item.rssi} dBm`;
                const snr = item.snr === null || item.snr === undefined ? "--" : `${item.snr} dB`;
                return `
                  <tr>
                    <td><span class="row-badge ${typeCls}">${item.type || "?"}</span></td>
                    <td>
                      <div class="row-title">${item.frequencyMhz} MHz / SF${item.spreadingFactor}</div>
                      <div class="row-sub">BW ${item.bandwidthKhz} kHz / CR ${item.codingRate}</div>
                    </td>
                    <td>${rssi}<div class="row-sub">${snr}</div></td>
                    <td>${item.hits || 0}</td>
                    <td>${fmtSeconds(item.ageSec || 0)}</td>
                    <td>${item.label || ""}<div class="row-sub">${item.note || ""}</div></td>
                  </tr>`;
              }).join("")}
            </tbody>
          </table>
        </div>`;
    }

    function sweepSummary(status) {
      if (status.lastSweepAgeSec === null || status.lastSweepAgeSec === undefined) {
        return "Użyj Szybki Sweep, żeby sprawdzić popularne kanały EU868 i zobaczyć, czy w ogóle coś LoRa nadaje w pobliżu.";
      }
      if (Number(status.lastSweepHits || 0) > 0) {
        return `Ostatni sweep znalazł ${status.lastSweepHits} trafień w ${status.lastSweepChecks} sprawdzeniach, ${fmtSeconds(status.lastSweepAgeSec)} temu.`;
      }
      return `Ostatni sweep sprawdził ${status.lastSweepChecks} popularnych presetów EU868 i nie wykrył żadnej preambuły LoRa, ${fmtSeconds(status.lastSweepAgeSec)} temu.`;
    }

    function networkFromSync(syncWord) {
      const sync = String(syncWord || "").toLowerCase();
      return sync === "34" ? "public" : "private";
    }

    function frequencyOptionValue(freq) {
      const num = Number(freq);
      return Number.isFinite(num) ? String(num) : "868.1";
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
      model.sightings = data.sightings || [];

      el["hero-title"].textContent = (status.name || "Esplora") + " / " + (status.id || "--");
      el["badge-id"].textContent = status.id || "--";
      el["badge-ip"].textContent = wifi.ip || wifi.targetIp || "--";
      el["badge-profile"].textContent = status.profile || "--";
      el["badge-air"].textContent = `${status.frequencyMhz || "--"} MHz / SF${status.spreadingFactor || "--"}`;

      dotState(el["dot-radio"], status.radioReady ? "live" : "off");
      dotState(el["dot-wifi"], wifi.connected ? "live" : "warn");
      dotState(el["dot-mqtt"], mqtt.connected ? "live" : (mqtt.enabled ? "warn" : "off"));
      el["pill-radio"].textContent = status.radioReady ? `LoRa gotowe na ${status.pinmap}` : "Radio offline";
      el["pill-wifi"].textContent = wifi.connected ? `Wi‑Fi ${wifi.ip}` : "Wi‑Fi bez połączenia";
      el["pill-mqtt"].textContent = mqtt.connected ? `MQTT ${mqtt.host}:${mqtt.port}` : (mqtt.enabled ? "MQTT ponawia połączenie" : "MQTT wyłączone");

      el["stat-neighbors"].textContent = String(data.neighborCount || 0);
      el["stat-neighbors-meta"].textContent = data.neighborCount
        ? "Kompatybilne moduły na dokładnie tym profilu"
        : (status.lastSweepHits ? `Brak dekodowania, ale sweep widział ${status.lastSweepHits} trafień` : "Brak kompatybilnych modułów");
      el["stat-wifi-rssi"].textContent = wifi.connected ? `${wifi.rssi} dBm` : "--";
      el["stat-wifi-meta"].textContent = wifi.connected ? `SSID ${wifi.ssid}` : "Stałe IP czeka na połączenie STA";
      el["stat-last-rx"].textContent = hasLastRx ? `${lastRx.kind} / ${lastRx.from || "--"}` : "--";
      el["stat-last-rx-meta"].textContent = hasLastRx ? `${lastRx.rssi} dBm, ${lastRx.snr} dB` : "Nic jeszcze nie zdekodowano";
      el["stat-uptime"].textContent = fmtSeconds(status.uptimeSec || 0);
      el["stat-uptime-meta"].textContent = status.radioReady ? "Moduł pracuje" : "Radio nie jest zainicjalizowane";

      el["radio-summary"].textContent = `Pinmap ${status.pinmap || "--"}, sync 0x${status.syncWord || "--"}, beacon ${status.beaconEnabled ? "włączony" : "wyłączony"}, logi RAW ${status.rawLoggingEnabled ? "włączone" : "wyłączone"}.`;
      el["wifi-summary"].textContent = wifi.connected
        ? `Połączono z ${wifi.ssid}, adres ${wifi.ip}, RSSI ${wifi.rssi} dBm.`
        : `Docelowy stały adres to ${wifi.targetIp || "192.168.1.201"}. Zapisz dane sieci, aby panel wracał po restarcie.`;
      el["mqtt-summary"].textContent = mqtt.connected
        ? `Publikacja na ${mqtt.baseTopic}. Discovery dla Home Assistant jest ${mqtt.haDiscovery ? "włączone" : "wyłączone"}.`
        : (mqtt.enabled ? `Broker ${mqtt.host}:${mqtt.port} ustawiony, czekam na połączenie.` : "MQTT jest wyłączone albo jeszcze nieustawione.");
      el["scanner-summary"].textContent = data.neighborCount
        ? `Zdekodowane węzły Esplora na ${status.frequencyMhz} MHz / SF${status.spreadingFactor}, posortowane od najświeższych.`
        : `Na ${status.frequencyMhz} MHz / SF${status.spreadingFactor} nie ma jeszcze zdekodowanych sąsiadów Esplora.`;
      el["sightings-summary"].textContent = sweepSummary(status);
      el["air-summary"].textContent = networkFromSync(status.syncWord) === "public"
        ? "Tryb publiczny: sync 0x34. Spróbuj go, jeśli chcesz podsłuchiwać bardziej publiczne / LoRaWAN-owe ramki."
        : "Tryb prywatny: sync 0x12. Najlepszy do własnych modułów, eksperymentów i pakietów Esplora.";

      syncInputValue(el["wifi-ssid"], wifi.ssid || "");
      syncInputValue(el["mqtt-host"], mqtt.host || "");
      syncInputValue(el["mqtt-port"], mqtt.port || 1883);
      syncInputValue(el["mqtt-user"], mqtt.user || "");
      syncInputValue(el["mqtt-topic"], mqtt.baseTopic || "");
      syncInputValue(el["air-freq"], frequencyOptionValue(status.frequencyMhz));
      syncInputValue(el["air-network"], networkFromSync(status.syncWord));

      renderProfiles(status.profile || "");
      renderNeighbors(model.neighbors);
      renderSightings(model.sightings);
      renderLogs(data.logs || []);

      document.getElementById("beacon-toggle-btn").textContent = status.beaconEnabled ? "Wyłącz Beacon" : "Włącz Beacon";
      document.getElementById("raw-toggle-btn").textContent = status.rawLoggingEnabled ? "Wyłącz RAW" : "Włącz RAW";
      document.getElementById("mqtt-toggle-btn").textContent = mqtt.enabled ? "Wyłącz MQTT" : "Włącz MQTT";
      document.getElementById("mqtt-ha-btn").textContent = mqtt.haDiscovery ? "Wyłącz Discovery HA" : "Włącz Discovery HA";
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
        model.logs.push({ seq: model.lastLogSeq + 1, line: `ERR|odswiezanie panelu nieudane: ${error.message}` });
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
    document.getElementById("sweep-btn").addEventListener("click", () => sendCommand("sweep"));
    document.getElementById("air-apply-btn").addEventListener("click", async () => {
      const freq = el["air-freq"].value;
      const sync = el["air-network"].value === "public" ? "34" : "12";
      await sendCommands([
        `set freq ${freq}`,
        `set sync ${sync}`
      ]);
    });
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
