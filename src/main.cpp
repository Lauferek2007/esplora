#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>
#include <RadioLib.h>
#include <WiFi.h>

#include <ctype.h>
#include <math.h>

namespace {

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t SERIAL_WAIT_MS = 2500;
constexpr uint32_t DEFAULT_BEACON_INTERVAL_MS = 30000;
constexpr size_t MAX_NEIGHBORS = 16;
constexpr size_t MAX_NAME_LEN = 16;
constexpr size_t MAX_MSG_LEN = 80;
constexpr uint8_t PROTOCOL_VERSION = 1;

struct PinMap {
  const char* key;
  const char* label;
  int8_t cs;
  int8_t dio1;
  int8_t rst;
  int8_t busy;
  int8_t sck;
  int8_t miso;
  int8_t mosi;
  int8_t antSwitch;
  bool hasAntSwitch;
};

struct RadioConfig {
  float frequencyMhz = 868.1f;
  float bandwidthKhz = 125.0f;
  uint8_t spreadingFactor = 9;
  uint8_t codingRate = 7;
  uint8_t syncWord = RADIOLIB_SX126X_SYNC_WORD_PRIVATE;
  int8_t txPower = 14;
  uint16_t preambleLength = 8;
  float tcxoVoltage = 3.0f;
  bool useRegulatorLDO = true;
  bool beaconEnabled = true;
  bool rawLoggingEnabled = true;
  uint32_t beaconIntervalMs = DEFAULT_BEACON_INTERVAL_MS;
  char profile[12] = "balanced";
};

struct Neighbor {
  bool used = false;
  char id[9] = {0};
  char name[MAX_NAME_LEN + 1] = {0};
  uint32_t lastSeenMs = 0;
  uint32_t lastSeq = 0;
  uint32_t rxCount = 0;
  uint32_t uptimeSec = 0;
  float rssi = 0.0f;
  float snr = 0.0f;
  float distanceM = 0.0f;
  long freqErrHz = 0;
  int8_t txPower = 0;
  char lastKind = '?';
};

struct ProbeConfig {
  const char* key;
  float tcxoVoltage;
  bool useRegulatorLDO;
};

struct WifiConfig {
  char ssid[33] = {0};
  char password[65] = {0};
  IPAddress localIp = IPAddress(192, 168, 1, 201);
  IPAddress gateway = IPAddress(192, 168, 1, 1);
  IPAddress subnet = IPAddress(255, 255, 255, 0);
  IPAddress dns1 = IPAddress(1, 1, 1, 1);
  IPAddress dns2 = IPAddress(8, 8, 8, 8);
};

const PinMap PINMAPS[] = {
  {
    "b2b",
    "Seeed XIAO ESP32S3 + Wio-SX1262 via B2B",
    41,
    39,
    42,
    40,
    7,
    8,
    9,
    38,
    true,
  },
  {
    "header",
    "Wio-SX1262 on header pins",
    5,
    2,
    3,
    4,
    7,
    8,
    9,
    -1,
    false,
  },
};

const ProbeConfig PROBES[] = {
  {"seeed-official", 3.0f, true},
  {"radiolib-forum-18v", 1.8f, true},
  {"radiolib-default-16v", 1.6f, false},
};

RadioConfig gConfig;
Neighbor gNeighbors[MAX_NEIGHBORS];
WifiConfig gWifiConfig;
Preferences gPrefs;

Module* gModule = nullptr;
SX1262* gRadio = nullptr;
const PinMap* gActivePinMap = nullptr;
String gSerialLine;

volatile bool gReceivedFlag = false;
bool gRadioReady = false;
uint32_t gLastBeaconAt = 0;
uint32_t gNextSequence = 1;

uint32_t gNodeId = 0;
char gNodeIdText[9] = {0};
char gNodeName[MAX_NAME_LEN + 1] = {0};

bool timeReached(uint32_t since, uint32_t waitMs) {
  return static_cast<uint32_t>(millis() - since) >= waitMs;
}

String previewText(const String& text, size_t maxLen = 48) {
  String out;
  out.reserve(maxLen);
  for (size_t i = 0; i < text.length() && out.length() < maxLen; ++i) {
    const char ch = text.charAt(i);
    out += isprint(static_cast<unsigned char>(ch)) ? ch : '.';
  }
  return out;
}

void copyString(char* dst, size_t dstLen, const String& src) {
  if (dstLen == 0) {
    return;
  }
  size_t len = src.length();
  if (len >= dstLen) {
    len = dstLen - 1;
  }
  memcpy(dst, src.c_str(), len);
  dst[len] = '\0';
}

String ipToString(const IPAddress& ip) {
  return ip.toString();
}

String sanitizeName(const String& raw) {
  String out;
  out.reserve(MAX_NAME_LEN);
  for (size_t i = 0; i < raw.length() && out.length() < MAX_NAME_LEN; ++i) {
    const char ch = raw.charAt(i);
    if (isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '_') {
      out += ch;
    } else if (ch == ' ') {
      out += '_';
    }
  }
  if (out.isEmpty()) {
    out = "ESPLORA";
  }
  return out;
}

String hexEncode(const uint8_t* data, size_t len) {
  static const char* HEX_DIGITS = "0123456789ABCDEF";
  String out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; ++i) {
    out += HEX_DIGITS[(data[i] >> 4) & 0x0F];
    out += HEX_DIGITS[data[i] & 0x0F];
  }
  return out;
}

String hexEncode(const String& text) {
  return hexEncode(reinterpret_cast<const uint8_t*>(text.c_str()), text.length());
}

bool hexToByte(char high, char low, uint8_t& out) {
  auto decode = [](char ch) -> int {
    if (ch >= '0' && ch <= '9') {
      return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
      return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f') {
      return ch - 'a' + 10;
    }
    return -1;
  };

  int hi = decode(high);
  int lo = decode(low);
  if (hi < 0 || lo < 0) {
    return false;
  }
  out = static_cast<uint8_t>((hi << 4) | lo);
  return true;
}

bool hexDecode(const String& hex, String& text) {
  if ((hex.length() % 2) != 0) {
    return false;
  }
  text = "";
  text.reserve(hex.length() / 2);
  for (size_t i = 0; i < hex.length(); i += 2) {
    uint8_t value = 0;
    if (!hexToByte(hex.charAt(i), hex.charAt(i + 1), value)) {
      return false;
    }
    text += static_cast<char>(value);
  }
  return true;
}

String splitField(const String& input, size_t index, char separator = ',') {
  size_t current = 0;
  int start = 0;
  for (;;) {
    int next = input.indexOf(separator, start);
    if (current == index) {
      if (next < 0) {
        return input.substring(start);
      }
      return input.substring(start, next);
    }
    if (next < 0) {
      return "";
    }
    start = next + 1;
    ++current;
  }
}

size_t splitCount(const String& input, char separator = ',') {
  if (input.isEmpty()) {
    return 0;
  }
  size_t count = 1;
  for (size_t i = 0; i < input.length(); ++i) {
    if (input.charAt(i) == separator) {
      ++count;
    }
  }
  return count;
}

float estimateDistanceMeters(float rssi, int8_t txPower) {
  const float effectiveLossAt1m = 43.0f;
  const float pathLossExponent = 2.2f;
  float numerator = static_cast<float>(txPower) - rssi - effectiveLossAt1m;
  float distance = powf(10.0f, numerator / (10.0f * pathLossExponent));
  if (!isfinite(distance) || distance < 0.5f) {
    distance = 0.5f;
  }
  if (distance > 20000.0f) {
    distance = 20000.0f;
  }
  return distance;
}

void emitLine(const String& line) {
  Serial.println(line);
}

void emitLog(const String& message) {
  emitLine("LOG|" + message);
}

void emitWarn(const String& message) {
  emitLine("WARN|" + message);
}

void emitError(const String& message) {
  emitLine("ERR|" + message);
}

void emitOk(const String& message) {
  emitLine("OK|" + message);
}

void emitEvent(const String& name, const String& details) {
  emitLine("EVT|" + name + "|" + details);
}

void loadWifiPrefs() {
  String ssid = gPrefs.getString("wifi_ssid", "");
  String password = gPrefs.getString("wifi_pass", "");
  copyString(gWifiConfig.ssid, sizeof(gWifiConfig.ssid), ssid);
  copyString(gWifiConfig.password, sizeof(gWifiConfig.password), password);
}

void saveWifiPrefs() {
  gPrefs.putString("wifi_ssid", gWifiConfig.ssid);
  gPrefs.putString("wifi_pass", gWifiConfig.password);
}

void printWifiStatus() {
  String line = "WIFI|SSID=" + String(gWifiConfig.ssid);
  line += "|CONNECTED=" + String(WiFi.status() == WL_CONNECTED ? 1 : 0);
  line += "|TARGET_IP=" + ipToString(gWifiConfig.localIp);
  if (WiFi.status() == WL_CONNECTED) {
    line += "|IP=" + ipToString(WiFi.localIP());
    line += "|RSSI=" + String(WiFi.RSSI());
  }
  emitLine(line);
}

bool connectWifi(bool announce) {
  if (strlen(gWifiConfig.ssid) == 0) {
    emitWarn("wifi ssid is empty, use: wifi ssid <name>");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(100);

  if (!WiFi.config(gWifiConfig.localIp, gWifiConfig.gateway, gWifiConfig.subnet, gWifiConfig.dns1, gWifiConfig.dns2)) {
    emitWarn("wifi config failed");
  }

  WiFi.begin(gWifiConfig.ssid, gWifiConfig.password);
  uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && !timeReached(started, 12000)) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (announce) {
      emitEvent(
        "WIFI",
        "STATE=CONNECTED|SSID=" + String(gWifiConfig.ssid) +
        "|IP=" + ipToString(WiFi.localIP())
      );
    }
    return true;
  }

  emitWarn("wifi connect failed, status=" + String(WiFi.status()));
  return false;
}

void disconnectWifi() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  emitEvent("WIFI", "STATE=OFF|TARGET_IP=" + ipToString(gWifiConfig.localIp));
}

Neighbor* findNeighbor(const String& nodeId) {
  for (size_t i = 0; i < MAX_NEIGHBORS; ++i) {
    if (gNeighbors[i].used && nodeId.equalsIgnoreCase(gNeighbors[i].id)) {
      return &gNeighbors[i];
    }
  }
  return nullptr;
}

Neighbor* allocateNeighbor(const String& nodeId) {
  if (Neighbor* existing = findNeighbor(nodeId)) {
    return existing;
  }

  for (size_t i = 0; i < MAX_NEIGHBORS; ++i) {
    if (!gNeighbors[i].used) {
      gNeighbors[i].used = true;
      copyString(gNeighbors[i].id, sizeof(gNeighbors[i].id), nodeId);
      return &gNeighbors[i];
    }
  }

  size_t oldestIndex = 0;
  for (size_t i = 1; i < MAX_NEIGHBORS; ++i) {
    if (gNeighbors[i].lastSeenMs < gNeighbors[oldestIndex].lastSeenMs) {
      oldestIndex = i;
    }
  }
  gNeighbors[oldestIndex] = Neighbor{};
  gNeighbors[oldestIndex].used = true;
  copyString(gNeighbors[oldestIndex].id, sizeof(gNeighbors[oldestIndex].id), nodeId);
  return &gNeighbors[oldestIndex];
}

void printNeighbor(const Neighbor& neighbor) {
  String details = "ID=" + String(neighbor.id);
  details += "|NAME=" + String(neighbor.name);
  details += "|AGE_S=" + String((millis() - neighbor.lastSeenMs) / 1000U);
  details += "|RSSI=" + String(neighbor.rssi, 1);
  details += "|SNR=" + String(neighbor.snr, 1);
  details += "|DIST_M=" + String(neighbor.distanceM, 1);
  details += "|TX=" + String(neighbor.txPower);
  details += "|SEQ=" + String(neighbor.lastSeq);
  details += "|COUNT=" + String(neighbor.rxCount);
  details += "|LAST=" + String(neighbor.lastKind);
  emitLine("NODE|" + details);
}

void printNeighbors() {
  bool any = false;
  for (size_t i = 0; i < MAX_NEIGHBORS; ++i) {
    if (!gNeighbors[i].used) {
      continue;
    }
    any = true;
    printNeighbor(gNeighbors[i]);
  }
  if (!any) {
    emitLine("NODE|EMPTY=1");
  }
}

void disposeRadio() {
  gRadioReady = false;
  gReceivedFlag = false;
  if (gRadio != nullptr) {
    delete gRadio;
    gRadio = nullptr;
  }
  if (gModule != nullptr) {
    delete gModule;
    gModule = nullptr;
  }
}

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void onPacketReceived() {
  gReceivedFlag = true;
}

bool restartReceive() {
  if (gRadio == nullptr) {
    return false;
  }
  int16_t state = gRadio->startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    emitError("startReceive failed, code=" + String(state));
    return false;
  }
  return true;
}

bool configureRadioForPinMap(const PinMap& pinMap) {
  SPI.end();
  delay(5);
  SPI.begin(pinMap.sck, pinMap.miso, pinMap.mosi, pinMap.cs);

  if (pinMap.hasAntSwitch && pinMap.antSwitch >= 0) {
    pinMode(pinMap.antSwitch, OUTPUT);
    digitalWrite(pinMap.antSwitch, HIGH);
  }

  pinMode(pinMap.rst, OUTPUT);
  digitalWrite(pinMap.rst, LOW);
  delay(10);
  digitalWrite(pinMap.rst, HIGH);
  delay(25);

  for (const ProbeConfig& probe : PROBES) {
    disposeRadio();

    emitLog(
      "probe pinmap=" + String(pinMap.key) +
      " tcxo=" + String(probe.tcxoVoltage, 1) +
      " ldo=" + String(probe.useRegulatorLDO ? 1 : 0)
    );

    gModule = new Module(
      pinMap.cs,
      pinMap.dio1,
      pinMap.rst,
      pinMap.busy,
      SPI,
      SPISettings(500000, MSBFIRST, SPI_MODE0)
    );
    gRadio = new SX1262(gModule);

    int16_t state = gRadio->begin(
      gConfig.frequencyMhz,
      gConfig.bandwidthKhz,
      gConfig.spreadingFactor,
      gConfig.codingRate,
      gConfig.syncWord,
      gConfig.txPower,
      gConfig.preambleLength,
      probe.tcxoVoltage,
      probe.useRegulatorLDO
    );
    if (state != RADIOLIB_ERR_NONE) {
      emitWarn(
        "radio init failed on pinmap=" + String(pinMap.key) +
        " probe=" + String(probe.key) +
        " code=" + String(state)
      );
      continue;
    }

    state = gRadio->setDio2AsRfSwitch(true);
    if (state != RADIOLIB_ERR_NONE) {
      emitWarn(
        "setDio2AsRfSwitch failed on pinmap=" + String(pinMap.key) +
        " probe=" + String(probe.key) +
        " code=" + String(state)
      );
    }

    gRadio->setPacketReceivedAction(onPacketReceived);

    if (!restartReceive()) {
      continue;
    }

    gConfig.tcxoVoltage = probe.tcxoVoltage;
    gConfig.useRegulatorLDO = probe.useRegulatorLDO;
    gActivePinMap = &pinMap;
    gRadioReady = true;
    gLastBeaconAt = millis();
    emitEvent(
      "RADIO",
      "READY=1|PINMAP=" + String(pinMap.key) +
      "|PROBE=" + String(probe.key) +
      "|FREQ=" + String(gConfig.frequencyMhz, 3) +
      "|BW=" + String(gConfig.bandwidthKhz, 1) +
      "|SF=" + String(gConfig.spreadingFactor) +
      "|CR=" + String(gConfig.codingRate) +
      "|TX=" + String(gConfig.txPower)
    );
    return true;
  }

  disposeRadio();
  return false;
}

bool initRadioAutoDetect(bool fullRetry) {
  if (!fullRetry && gActivePinMap != nullptr) {
    if (configureRadioForPinMap(*gActivePinMap)) {
      return true;
    }
  }

  for (const PinMap& candidate : PINMAPS) {
    if (!fullRetry && gActivePinMap == &candidate) {
      continue;
    }
    if (configureRadioForPinMap(candidate)) {
      return true;
    }
  }

  emitError("unable to initialize SX1262 on any supported pinmap");
  return false;
}

String buildFrame(char kind, const String& payloadA = "", const String& payloadB = "") {
  String frame = "E";
  frame += String(PROTOCOL_VERSION);
  frame += ",";
  frame += kind;
  frame += ",";
  frame += String(gNodeIdText);
  frame += ",";
  frame += String(gNextSequence++);
  frame += ",";
  frame += String(gNodeName);
  frame += ",";
  frame += String(gConfig.txPower);
  if (!payloadA.isEmpty() || !payloadB.isEmpty()) {
    frame += ",";
    frame += payloadA;
  }
  if (!payloadB.isEmpty()) {
    frame += ",";
    frame += payloadB;
  }
  return frame;
}

bool transmitFrame(const String& frame, const char* label) {
  if (!gRadioReady || gRadio == nullptr) {
    emitError("radio not ready");
    return false;
  }

  RadioLibTime_t airtime = gRadio->getTimeOnAir(frame.length());
  String mutableFrame = frame;
  int16_t state = gRadio->transmit(mutableFrame);
  if (state != RADIOLIB_ERR_NONE) {
    emitError(String("tx failed, kind=") + label + ", code=" + String(state));
    restartReceive();
    return false;
  }

  emitEvent(
    "TX",
    "KIND=" + String(label) +
    "|BYTES=" + String(frame.length()) +
    "|AIRTIME_US=" + String(static_cast<uint32_t>(airtime)) +
    "|SEQ=" + String(gNextSequence - 1)
  );
  restartReceive();
  return true;
}

bool sendBeacon(const char* reason) {
  String frame = buildFrame('B', String(millis() / 1000U), String(gConfig.beaconIntervalMs / 1000U));
  bool ok = transmitFrame(frame, "BEACON");
  if (ok) {
    gLastBeaconAt = millis();
    emitEvent("BEACON", "WHY=" + String(reason));
  }
  return ok;
}

bool sendMessage(const String& text) {
  String trimmed = text;
  trimmed.trim();
  if (trimmed.isEmpty()) {
    emitError("message is empty");
    return false;
  }
  if (trimmed.length() > MAX_MSG_LEN) {
    emitError("message too long, max " + String(MAX_MSG_LEN) + " chars");
    return false;
  }
  return transmitFrame(buildFrame('M', hexEncode(trimmed)), "MSG");
}

bool sendPing(const String& nonce) {
  return transmitFrame(buildFrame('P', nonce), "PING");
}

bool sendPong(const String& nonce) {
  return transmitFrame(buildFrame('O', nonce), "PONG");
}

void updateNeighbor(
  const String& nodeId,
  const String& name,
  uint32_t seq,
  char kind,
  int8_t txPower,
  float rssi,
  float snr,
  long freqErrHz,
  uint32_t uptimeSec
) {
  Neighbor* neighbor = allocateNeighbor(nodeId);
  if (neighbor == nullptr) {
    return;
  }

  copyString(neighbor->name, sizeof(neighbor->name), sanitizeName(name));
  neighbor->lastSeenMs = millis();
  neighbor->lastSeq = seq;
  neighbor->rxCount += 1;
  neighbor->rssi = rssi;
  neighbor->snr = snr;
  neighbor->freqErrHz = freqErrHz;
  neighbor->txPower = txPower;
  neighbor->distanceM = estimateDistanceMeters(rssi, txPower);
  neighbor->uptimeSec = uptimeSec;
  neighbor->lastKind = kind;
}

void logKnownPacket(
  char kind,
  const String& nodeId,
  const String& name,
  uint32_t seq,
  int8_t txPower,
  float rssi,
  float snr,
  long freqErrHz,
  const String& extra
) {
  String details = "KIND=";
  details += kind;
  details += "|FROM=";
  details += nodeId;
  details += "|NAME=";
  details += sanitizeName(name);
  details += "|SEQ=";
  details += String(seq);
  details += "|RSSI=";
  details += String(rssi, 1);
  details += "|SNR=";
  details += String(snr, 1);
  details += "|FERR=";
  details += String(freqErrHz);
  details += "|TX=";
  details += String(txPower);
  if (!extra.isEmpty()) {
    details += "|";
    details += extra;
  }
  emitEvent("RX", details);
}

void handleKnownFrame(const String& packet, float rssi, float snr, long freqErrHz) {
  size_t fields = splitCount(packet);
  if (fields < 6) {
    emitWarn("known frame too short");
    return;
  }

  String kindField = splitField(packet, 1);
  if (kindField.length() != 1) {
    emitWarn("invalid frame kind");
    return;
  }

  const char kind = kindField.charAt(0);
  String nodeId = splitField(packet, 2);
  if (nodeId.equalsIgnoreCase(gNodeIdText)) {
    return;
  }

  String name = sanitizeName(splitField(packet, 4));
  uint32_t seq = static_cast<uint32_t>(splitField(packet, 3).toInt());
  int8_t txPower = static_cast<int8_t>(splitField(packet, 5).toInt());
  uint32_t uptimeSec = 0;

  if (kind == 'B' && fields >= 8) {
    uptimeSec = static_cast<uint32_t>(splitField(packet, 6).toInt());
    updateNeighbor(nodeId, name, seq, kind, txPower, rssi, snr, freqErrHz, uptimeSec);
    Neighbor* neighbor = findNeighbor(nodeId);
    if (neighbor != nullptr) {
      logKnownPacket(
        kind,
        nodeId,
        name,
        seq,
        txPower,
        rssi,
        snr,
        freqErrHz,
        "DIST_M=" + String(neighbor->distanceM, 1) + "|UPTIME_S=" + String(uptimeSec)
      );
    }
    return;
  }

  if (kind == 'M' && fields >= 7) {
    updateNeighbor(nodeId, name, seq, kind, txPower, rssi, snr, freqErrHz, 0);
    String text;
    if (!hexDecode(splitField(packet, 6), text)) {
      emitWarn("message hex decode failed");
      return;
    }
    Neighbor* neighbor = findNeighbor(nodeId);
    String extra = "TXTHEX=" + splitField(packet, 6);
    extra += "|TEXT=" + previewText(text);
    if (neighbor != nullptr) {
      extra += "|DIST_M=" + String(neighbor->distanceM, 1);
    }
    logKnownPacket(kind, nodeId, name, seq, txPower, rssi, snr, freqErrHz, extra);
    return;
  }

  if ((kind == 'P' || kind == 'O') && fields >= 7) {
    updateNeighbor(nodeId, name, seq, kind, txPower, rssi, snr, freqErrHz, 0);
    String nonce = splitField(packet, 6);
    Neighbor* neighbor = findNeighbor(nodeId);
    String extra = "NONCE=" + nonce;
    if (neighbor != nullptr) {
      extra += "|DIST_M=" + String(neighbor->distanceM, 1);
    }
    logKnownPacket(kind, nodeId, name, seq, txPower, rssi, snr, freqErrHz, extra);
    if (kind == 'P') {
      delay(random(30, 140));
      sendPong(nonce);
    }
    return;
  }

  emitWarn("unsupported known frame kind");
}

void handleUnknownFrame(const String& packet, float rssi, float snr, long freqErrHz) {
  if (!gConfig.rawLoggingEnabled) {
    return;
  }
  String details = "RSSI=" + String(rssi, 1);
  details += "|SNR=" + String(snr, 1);
  details += "|FERR=" + String(freqErrHz);
  details += "|BYTES=" + String(packet.length());
  details += "|ASCII=" + previewText(packet);
  details += "|HEX=" + hexEncode(packet);
  emitEvent("RAW", details);
}

void handleReceivedPacket() {
  if (!gRadioReady || gRadio == nullptr) {
    gReceivedFlag = false;
    return;
  }

  gReceivedFlag = false;

  String packet;
  int16_t state = gRadio->readData(packet);
  if (state == RADIOLIB_ERR_NONE) {
    if (packet.isEmpty()) {
      restartReceive();
      return;
    }
    float rssi = gRadio->getRSSI();
    float snr = gRadio->getSNR();
    long freqErrHz = lroundf(gRadio->getFrequencyError());
    if (packet.startsWith("E1,")) {
      handleKnownFrame(packet, rssi, snr, freqErrHz);
    } else {
      handleUnknownFrame(packet, rssi, snr, freqErrHz);
    }
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    emitWarn("crc mismatch");
  } else {
    emitError("readData failed, code=" + String(state));
  }

  restartReceive();
}

void printStatus() {
  String line = "STAT|ID=" + String(gNodeIdText);
  line += "|NAME=" + String(gNodeName);
  line += "|READY=" + String(gRadioReady ? 1 : 0);
  line += "|PINMAP=" + String(gActivePinMap == nullptr ? "none" : gActivePinMap->key);
  line += "|FREQ=" + String(gConfig.frequencyMhz, 3);
  line += "|BW=" + String(gConfig.bandwidthKhz, 1);
  line += "|SF=" + String(gConfig.spreadingFactor);
  line += "|CR=" + String(gConfig.codingRate);
  line += "|TX=" + String(gConfig.txPower);
  line += "|SYNC=0x" + String(gConfig.syncWord, HEX);
  line += "|BEACON=" + String(gConfig.beaconEnabled ? 1 : 0);
  line += "|INTERVAL_S=" + String(gConfig.beaconIntervalMs / 1000U);
  line += "|PROFILE=" + String(gConfig.profile);
  line += "|WIFI=" + String(WiFi.status() == WL_CONNECTED ? 1 : 0);
  line += "|IP=" + String(WiFi.status() == WL_CONNECTED ? ipToString(WiFi.localIP()) : ipToString(gWifiConfig.localIp));
  emitLine(line);
}

bool applyProfile(const String& profileName) {
  if (profileName.equalsIgnoreCase("fast")) {
    gConfig.bandwidthKhz = 250.0f;
    gConfig.spreadingFactor = 7;
    gConfig.codingRate = 5;
    gConfig.txPower = 14;
    copyString(gConfig.profile, sizeof(gConfig.profile), "fast");
    return true;
  }

  if (profileName.equalsIgnoreCase("balanced")) {
    gConfig.bandwidthKhz = 125.0f;
    gConfig.spreadingFactor = 9;
    gConfig.codingRate = 7;
    gConfig.txPower = 14;
    copyString(gConfig.profile, sizeof(gConfig.profile), "balanced");
    return true;
  }

  if (profileName.equalsIgnoreCase("long")) {
    gConfig.bandwidthKhz = 125.0f;
    gConfig.spreadingFactor = 11;
    gConfig.codingRate = 8;
    gConfig.txPower = 14;
    copyString(gConfig.profile, sizeof(gConfig.profile), "long");
    return true;
  }

  emitError("unknown profile, use fast|balanced|long");
  return false;
}

bool runCad() {
  if (!gRadioReady || gRadio == nullptr) {
    emitError("radio not ready");
    return false;
  }

  int16_t state = gRadio->scanChannel();
  if (state == RADIOLIB_LORA_DETECTED) {
    emitEvent("CAD", "BUSY=1");
    restartReceive();
    return true;
  }
  if (state == RADIOLIB_CHANNEL_FREE) {
    emitEvent("CAD", "BUSY=0");
    restartReceive();
    return true;
  }

  emitError("cad failed, code=" + String(state));
  restartReceive();
  return false;
}

void printHelp() {
  emitLine("HELP|status");
  emitLine("HELP|neighbors");
  emitLine("HELP|send <tekst>");
  emitLine("HELP|ping");
  emitLine("HELP|beacon on|off|now|<sekundy>");
  emitLine("HELP|profile fast|balanced|long");
  emitLine("HELP|set freq <MHz>");
  emitLine("HELP|set bw <kHz>");
  emitLine("HELP|set sf <7-12>");
  emitLine("HELP|set cr <5-8>");
  emitLine("HELP|set power <dBm>");
  emitLine("HELP|set name <nazwa>");
  emitLine("HELP|raw on|off");
  emitLine("HELP|wifi status");
  emitLine("HELP|wifi ssid <ssid>");
  emitLine("HELP|wifi pass <haslo>");
  emitLine("HELP|wifi connect");
  emitLine("HELP|wifi off");
  emitLine("HELP|cad");
  emitLine("HELP|restart radio");
}

bool reconfigureRadio() {
  return initRadioAutoDetect(false) || initRadioAutoDetect(true);
}

void handleCommand(const String& input) {
  String line = input;
  line.trim();
  if (line.isEmpty()) {
    return;
  }

  emitEvent("CMD", "TEXT=" + previewText(line, 80));

  if (line.equalsIgnoreCase("help")) {
    printHelp();
    return;
  }

  if (line.equalsIgnoreCase("status")) {
    printStatus();
    return;
  }

  if (line.equalsIgnoreCase("neighbors")) {
    printNeighbors();
    return;
  }

  if (line.equalsIgnoreCase("ping")) {
    String nonce = String(millis());
    sendPing(nonce);
    return;
  }

  if (line.equalsIgnoreCase("cad")) {
    runCad();
    return;
  }

  if (line.equalsIgnoreCase("wifi status")) {
    printWifiStatus();
    return;
  }

  if (line.equalsIgnoreCase("wifi connect")) {
    connectWifi(true);
    return;
  }

  if (line.equalsIgnoreCase("wifi off")) {
    disconnectWifi();
    return;
  }

  if (line.startsWith("wifi ssid ")) {
    String ssid = line.substring(10);
    ssid.trim();
    copyString(gWifiConfig.ssid, sizeof(gWifiConfig.ssid), ssid);
    saveWifiPrefs();
    emitOk("wifi ssid saved");
    return;
  }

  if (line.startsWith("wifi pass ")) {
    String password = line.substring(10);
    password.trim();
    copyString(gWifiConfig.password, sizeof(gWifiConfig.password), password);
    saveWifiPrefs();
    emitOk("wifi password saved");
    return;
  }

  if (line.equalsIgnoreCase("beacon on")) {
    gConfig.beaconEnabled = true;
    emitOk("beacon enabled");
    return;
  }

  if (line.equalsIgnoreCase("beacon off")) {
    gConfig.beaconEnabled = false;
    emitOk("beacon disabled");
    return;
  }

  if (line.equalsIgnoreCase("beacon now")) {
    sendBeacon("manual");
    return;
  }

  if (line.startsWith("beacon ")) {
    String value = line.substring(7);
    value.trim();
    uint32_t seconds = static_cast<uint32_t>(value.toInt());
    if (seconds == 0) {
      emitError("beacon expects on|off|now or integer seconds");
      return;
    }
    gConfig.beaconEnabled = true;
    gConfig.beaconIntervalMs = seconds * 1000UL;
    emitOk("beacon interval set to " + String(seconds) + " s");
    return;
  }

  if (line.startsWith("send ")) {
    sendMessage(line.substring(5));
    return;
  }

  if (line.startsWith("profile ")) {
    if (applyProfile(line.substring(8))) {
      if (reconfigureRadio()) {
        emitOk("profile applied");
      }
    }
    return;
  }

  if (line.startsWith("set name ")) {
    String name = sanitizeName(line.substring(9));
    copyString(gNodeName, sizeof(gNodeName), name);
    emitOk("name=" + String(gNodeName));
    return;
  }

  if (line.equalsIgnoreCase("raw on")) {
    gConfig.rawLoggingEnabled = true;
    emitOk("raw logging enabled");
    return;
  }

  if (line.equalsIgnoreCase("raw off")) {
    gConfig.rawLoggingEnabled = false;
    emitOk("raw logging disabled");
    return;
  }

  if (line.equalsIgnoreCase("restart radio")) {
    if (initRadioAutoDetect(true)) {
      emitOk("radio restarted");
    }
    return;
  }

  if (line.startsWith("set freq ")) {
    float value = line.substring(9).toFloat();
    if (value < 150.0f || value > 960.0f) {
      emitError("freq out of range");
      return;
    }
    gConfig.frequencyMhz = value;
    if (reconfigureRadio()) {
      emitOk("freq=" + String(gConfig.frequencyMhz, 3));
    }
    return;
  }

  if (line.startsWith("set bw ")) {
    float value = line.substring(7).toFloat();
    if (value < 7.8f || value > 500.0f) {
      emitError("bw out of range");
      return;
    }
    gConfig.bandwidthKhz = value;
    if (reconfigureRadio()) {
      emitOk("bw=" + String(gConfig.bandwidthKhz, 1));
    }
    return;
  }

  if (line.startsWith("set sf ")) {
    int value = line.substring(7).toInt();
    if (value < 5 || value > 12) {
      emitError("sf out of range");
      return;
    }
    gConfig.spreadingFactor = static_cast<uint8_t>(value);
    if (reconfigureRadio()) {
      emitOk("sf=" + String(gConfig.spreadingFactor));
    }
    return;
  }

  if (line.startsWith("set cr ")) {
    int value = line.substring(7).toInt();
    if (value < 5 || value > 8) {
      emitError("cr out of range, use 5..8");
      return;
    }
    gConfig.codingRate = static_cast<uint8_t>(value);
    if (reconfigureRadio()) {
      emitOk("cr=" + String(gConfig.codingRate));
    }
    return;
  }

  if (line.startsWith("set power ")) {
    int value = line.substring(10).toInt();
    if (value < -9 || value > 22) {
      emitError("power out of range");
      return;
    }
    gConfig.txPower = static_cast<int8_t>(value);
    if (gConfig.txPower > 14) {
      emitWarn("power >14 dBm may be illegal for some EU868 uses");
    }
    if (reconfigureRadio()) {
      emitOk("power=" + String(gConfig.txPower));
    }
    return;
  }

  emitError("unknown command, use help");
}

void pollSerial() {
  while (Serial.available() > 0) {
    char ch = static_cast<char>(Serial.read());
    if (ch == '\r') {
      continue;
    }
    if (ch == '\n') {
      handleCommand(gSerialLine);
      gSerialLine = "";
      continue;
    }
    if (gSerialLine.length() < 160) {
      gSerialLine += ch;
    }
  }
}

void setupIdentity() {
  uint64_t mac = ESP.getEfuseMac();
  gNodeId = static_cast<uint32_t>(mac & 0xFFFFFFFFULL);
  snprintf(gNodeIdText, sizeof(gNodeIdText), "%08lX", static_cast<unsigned long>(gNodeId));

  String defaultName = "ESP-" + String(gNodeIdText).substring(2);
  copyString(gNodeName, sizeof(gNodeName), sanitizeName(defaultName));
}

}  // namespace

void setup() {
  Serial.begin(SERIAL_BAUD);
  uint32_t started = millis();
  while (!Serial && !timeReached(started, SERIAL_WAIT_MS)) {
    delay(10);
  }

  randomSeed(ESP.getEfuseMac());
  gPrefs.begin("esplora", false);
  loadWifiPrefs();
  setupIdentity();
  applyProfile("balanced");

  emitLog("esplora boot");
  emitEvent(
    "BOOT",
    "ID=" + String(gNodeIdText) +
    "|NAME=" + String(gNodeName) +
    "|BOARD=seeed_xiao_esp32s3" +
    "|DEFAULT_FREQ=" + String(gConfig.frequencyMhz, 3)
  );

  printHelp();
  initRadioAutoDetect(true);
  if (strlen(gWifiConfig.ssid) > 0) {
    connectWifi(true);
  } else {
    emitEvent("WIFI", "STATE=SKIPPED|TARGET_IP=" + ipToString(gWifiConfig.localIp));
  }
  printStatus();
}

void loop() {
  pollSerial();

  if (gReceivedFlag) {
    handleReceivedPacket();
  }

  if (gConfig.beaconEnabled && gRadioReady && timeReached(gLastBeaconAt, gConfig.beaconIntervalMs)) {
    sendBeacon("interval");
  }
}
