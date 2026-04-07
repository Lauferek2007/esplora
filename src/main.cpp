#include <Arduino.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <RadioLib.h>
#include <WebServer.h>
#include <WiFi.h>

#include <ctype.h>
#include <math.h>

#include "web_panel.h"

namespace {

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t SERIAL_WAIT_MS = 2500;
constexpr uint32_t DEFAULT_BEACON_INTERVAL_MS = 30000;
constexpr uint32_t MQTT_RECONNECT_INTERVAL_MS = 5000;
constexpr uint32_t MQTT_STATE_INTERVAL_MS = 10000;
constexpr size_t MAX_NEIGHBORS = 16;
constexpr size_t MAX_NAME_LEN = 16;
constexpr size_t MAX_MSG_LEN = 80;
constexpr size_t MAX_LOG_ENTRIES = 64;
constexpr size_t MAX_LOG_LINE_LEN = 192;
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

struct MqttConfig {
  bool enabled = false;
  bool haDiscoveryEnabled = true;
  char host[65] = {0};
  uint16_t port = 1883;
  char user[33] = {0};
  char password[65] = {0};
  char baseTopic[80] = {0};
};

struct LogEntry {
  bool used = false;
  uint32_t seq = 0;
  uint32_t tsMs = 0;
  char line[MAX_LOG_LINE_LEN + 1] = {0};
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
MqttConfig gMqttConfig;
LogEntry gLogEntries[MAX_LOG_ENTRIES];
Preferences gPrefs;

Module* gModule = nullptr;
SX1262* gRadio = nullptr;
const PinMap* gActivePinMap = nullptr;
String gSerialLine;
WebServer gWebServer(80);
WiFiClient gMqttSocket;
PubSubClient gMqttClient(gMqttSocket);

volatile bool gReceivedFlag = false;
bool gRadioReady = false;
bool gWebServerStarted = false;
bool gMqttDiscoveryPublished = false;
bool gMqttStateDirty = true;
bool gMqttNeighborsDirty = true;
uint32_t gLastBeaconAt = 0;
uint32_t gNextSequence = 1;
uint32_t gLastLogSequence = 0;
uint32_t gLastMqttReconnectAt = 0;
uint32_t gLastMqttStateAt = 0;
uint32_t gLastRxAt = 0;

uint32_t gNodeId = 0;
char gNodeIdText[9] = {0};
char gNodeName[MAX_NAME_LEN + 1] = {0};
char gLastRxFrom[9] = {0};
char gLastRxText[MAX_MSG_LEN + 1] = {0};
char gLastRxKind = '-';
float gLastRxRssi = 0.0f;
float gLastRxSnr = 0.0f;
long gLastRxFreqErrHz = 0;

bool timeReached(uint32_t since, uint32_t waitMs) {
  return static_cast<uint32_t>(millis() - since) >= waitMs;
}

size_t countNeighbors() {
  size_t count = 0;
  for (const Neighbor& neighbor : gNeighbors) {
    if (neighbor.used) {
      ++count;
    }
  }
  return count;
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

String jsonEscape(const String& raw) {
  String out;
  out.reserve(raw.length() + 8);
  for (size_t i = 0; i < raw.length(); ++i) {
    const char ch = raw.charAt(i);
    switch (ch) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (static_cast<uint8_t>(ch) < 0x20) {
          out += ' ';
        } else {
          out += ch;
        }
        break;
    }
  }
  return out;
}

void appendJsonQuoted(String& out, const String& value) {
  out += '"';
  out += jsonEscape(value);
  out += '"';
}

void markMqttStateDirty(bool neighborsToo = false) {
  gMqttStateDirty = true;
  if (neighborsToo) {
    gMqttNeighborsDirty = true;
  }
}

String mqttAvailabilityTopic();
void mqttPublishEventLine(const String& line);
void mqttEnsureConnected();
void mqttLoop();
void ensureWebServerState();

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

void rememberLogLine(const String& line) {
  const uint32_t seq = ++gLastLogSequence;
  LogEntry& entry = gLogEntries[seq % MAX_LOG_ENTRIES];
  entry.used = true;
  entry.seq = seq;
  entry.tsMs = millis();
  copyString(entry.line, sizeof(entry.line), line);
}

void emitLine(const String& line) {
  rememberLogLine(line);
  Serial.println(line);
  mqttPublishEventLine(line);
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

void loadMqttPrefs() {
  gMqttConfig.enabled = gPrefs.getBool("mqtt_en", false);
  gMqttConfig.haDiscoveryEnabled = gPrefs.getBool("mqtt_ha", true);
  copyString(gMqttConfig.host, sizeof(gMqttConfig.host), gPrefs.getString("mqtt_host", ""));
  gMqttConfig.port = static_cast<uint16_t>(gPrefs.getUInt("mqtt_port", 1883));
  copyString(gMqttConfig.user, sizeof(gMqttConfig.user), gPrefs.getString("mqtt_user", ""));
  copyString(gMqttConfig.password, sizeof(gMqttConfig.password), gPrefs.getString("mqtt_pass", ""));
  copyString(gMqttConfig.baseTopic, sizeof(gMqttConfig.baseTopic), gPrefs.getString("mqtt_topic", ""));
  if (strlen(gMqttConfig.baseTopic) == 0) {
    copyString(gMqttConfig.baseTopic, sizeof(gMqttConfig.baseTopic), "esplora/" + String(gNodeIdText));
  }
}

void saveMqttPrefs() {
  gPrefs.putBool("mqtt_en", gMqttConfig.enabled);
  gPrefs.putBool("mqtt_ha", gMqttConfig.haDiscoveryEnabled);
  gPrefs.putString("mqtt_host", gMqttConfig.host);
  gPrefs.putUInt("mqtt_port", gMqttConfig.port);
  gPrefs.putString("mqtt_user", gMqttConfig.user);
  gPrefs.putString("mqtt_pass", gMqttConfig.password);
  gPrefs.putString("mqtt_topic", gMqttConfig.baseTopic);
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

void printMqttStatus() {
  String line = "MQTT|ENABLED=" + String(gMqttConfig.enabled ? 1 : 0);
  line += "|CONNECTED=" + String(gMqttClient.connected() ? 1 : 0);
  line += "|HOST=" + String(gMqttConfig.host);
  line += "|PORT=" + String(gMqttConfig.port);
  line += "|TOPIC=" + String(gMqttConfig.baseTopic);
  line += "|HA=" + String(gMqttConfig.haDiscoveryEnabled ? 1 : 0);
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
    markMqttStateDirty();
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
  if (gMqttClient.connected()) {
    gMqttClient.publish(mqttAvailabilityTopic().c_str(), "offline", true);
    gMqttClient.disconnect();
  }
  gMqttDiscoveryPublished = false;
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  markMqttStateDirty();
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

String buildStatusJson() {
  String out;
  out.reserve(960);
  out += '{';
  out += "\"status\":{";
  out += "\"id\":";
  appendJsonQuoted(out, String(gNodeIdText));
  out += ",\"name\":";
  appendJsonQuoted(out, String(gNodeName));
  out += ",\"radioReady\":";
  out += gRadioReady ? "true" : "false";
  out += ",\"pinmap\":";
  appendJsonQuoted(out, gActivePinMap == nullptr ? "none" : String(gActivePinMap->key));
  out += ",\"frequencyMhz\":";
  out += String(gConfig.frequencyMhz, 3);
  out += ",\"bandwidthKhz\":";
  out += String(gConfig.bandwidthKhz, 1);
  out += ",\"spreadingFactor\":";
  out += String(gConfig.spreadingFactor);
  out += ",\"codingRate\":";
  out += String(gConfig.codingRate);
  out += ",\"txPower\":";
  out += String(gConfig.txPower);
  out += ",\"syncWord\":";
  appendJsonQuoted(out, String(gConfig.syncWord, HEX));
  out += ",\"beaconEnabled\":";
  out += gConfig.beaconEnabled ? "true" : "false";
  out += ",\"rawLoggingEnabled\":";
  out += gConfig.rawLoggingEnabled ? "true" : "false";
  out += ",\"beaconIntervalSec\":";
  out += String(gConfig.beaconIntervalMs / 1000U);
  out += ",\"profile\":";
  appendJsonQuoted(out, String(gConfig.profile));
  out += ",\"uptimeSec\":";
  out += String(millis() / 1000U);
  out += '}';

  out += ",\"wifi\":{";
  out += "\"ssid\":";
  appendJsonQuoted(out, String(gWifiConfig.ssid));
  out += ",\"connected\":";
  out += WiFi.status() == WL_CONNECTED ? "true" : "false";
  out += ",\"targetIp\":";
  appendJsonQuoted(out, ipToString(gWifiConfig.localIp));
  out += ",\"ip\":";
  appendJsonQuoted(out, WiFi.status() == WL_CONNECTED ? ipToString(WiFi.localIP()) : "");
  out += ",\"rssi\":";
  out += WiFi.status() == WL_CONNECTED ? String(WiFi.RSSI()) : "null";
  out += '}';

  out += ",\"mqtt\":{";
  out += "\"enabled\":";
  out += gMqttConfig.enabled ? "true" : "false";
  out += ",\"connected\":";
  out += gMqttClient.connected() ? "true" : "false";
  out += ",\"host\":";
  appendJsonQuoted(out, String(gMqttConfig.host));
  out += ",\"port\":";
  out += String(gMqttConfig.port);
  out += ",\"user\":";
  appendJsonQuoted(out, String(gMqttConfig.user));
  out += ",\"baseTopic\":";
  appendJsonQuoted(out, String(gMqttConfig.baseTopic));
  out += ",\"haDiscovery\":";
  out += gMqttConfig.haDiscoveryEnabled ? "true" : "false";
  out += '}';

  out += ",\"lastRx\":{";
  out += "\"kind\":";
  appendJsonQuoted(out, String(gLastRxKind));
  out += ",\"from\":";
  appendJsonQuoted(out, String(gLastRxFrom));
  out += ",\"text\":";
  appendJsonQuoted(out, String(gLastRxText));
  out += ",\"rssi\":";
  out += gLastRxAt > 0 ? String(gLastRxRssi, 1) : "null";
  out += ",\"snr\":";
  out += gLastRxAt > 0 ? String(gLastRxSnr, 1) : "null";
  out += ",\"freqErrHz\":";
  out += gLastRxAt > 0 ? String(gLastRxFreqErrHz) : "null";
  out += ",\"ageSec\":";
  out += gLastRxAt > 0 ? String((millis() - gLastRxAt) / 1000U) : "null";
  out += '}';

  out += ",\"neighborCount\":";
  out += String(countNeighbors());
  out += '}';
  return out;
}

String buildNeighborsJson() {
  String out;
  out.reserve(1600);
  out += '[';
  bool first = true;
  for (const Neighbor& neighbor : gNeighbors) {
    if (!neighbor.used) {
      continue;
    }
    if (!first) {
      out += ',';
    }
    first = false;
    out += '{';
    out += "\"id\":";
    appendJsonQuoted(out, String(neighbor.id));
    out += ",\"name\":";
    appendJsonQuoted(out, String(neighbor.name));
    out += ",\"ageSec\":";
    out += String((millis() - neighbor.lastSeenMs) / 1000U);
    out += ",\"rssi\":";
    out += String(neighbor.rssi, 1);
    out += ",\"snr\":";
    out += String(neighbor.snr, 1);
    out += ",\"distanceM\":";
    out += String(neighbor.distanceM, 1);
    out += ",\"txPower\":";
    out += String(neighbor.txPower);
    out += ",\"lastSeq\":";
    out += String(neighbor.lastSeq);
    out += ",\"rxCount\":";
    out += String(neighbor.rxCount);
    out += ",\"uptimeSec\":";
    out += String(neighbor.uptimeSec);
    out += ",\"lastKind\":";
    appendJsonQuoted(out, String(neighbor.lastKind));
    out += '}';
  }
  out += ']';
  return out;
}

String buildLogsJson(uint32_t since) {
  String out;
  out.reserve(4200);
  out += '[';
  bool first = true;
  const uint32_t earliest = (gLastLogSequence > MAX_LOG_ENTRIES) ? (gLastLogSequence - MAX_LOG_ENTRIES + 1) : 1;
  const uint32_t start = since + 1 > earliest ? since + 1 : earliest;
  for (uint32_t seq = start; seq <= gLastLogSequence; ++seq) {
    const LogEntry& entry = gLogEntries[seq % MAX_LOG_ENTRIES];
    if (!entry.used || entry.seq != seq) {
      continue;
    }
    if (!first) {
      out += ',';
    }
    first = false;
    out += '{';
    out += "\"seq\":";
    out += String(entry.seq);
    out += ",\"tsMs\":";
    out += String(entry.tsMs);
    out += ",\"line\":";
    appendJsonQuoted(out, String(entry.line));
    out += '}';
  }
  out += ']';
  return out;
}

String buildApiStateJson(uint32_t since) {
  String out;
  String statusJson = buildStatusJson();
  String neighborsJson = buildNeighborsJson();
  String logsJson = buildLogsJson(since);
  out.reserve(statusJson.length() + neighborsJson.length() + logsJson.length() + 128);
  out += '{';
  out += "\"nowMs\":";
  out += String(millis());
  out += ",\"neighborCount\":";
  out += String(countNeighbors());
  out += ",\"lastLogSeq\":";
  out += String(gLastLogSequence);
  out += ",\"payload\":";
  out += statusJson;
  out += ",\"neighbors\":";
  out += neighborsJson;
  out += ",\"logs\":";
  out += logsJson;
  out += '}';
  return out;
}

String mqttAvailabilityTopic() {
  return String(gMqttConfig.baseTopic) + "/availability";
}

String mqttStatusTopic() {
  return String(gMqttConfig.baseTopic) + "/status";
}

String mqttNeighborsTopic() {
  return String(gMqttConfig.baseTopic) + "/neighbors";
}

String mqttEventsTopic() {
  return String(gMqttConfig.baseTopic) + "/events";
}

void mqttPublishRetained(const String& topic, const String& payload) {
  if (!gMqttClient.connected()) {
    return;
  }
  gMqttClient.publish(topic.c_str(), payload.c_str(), true);
}

void mqttPublishTransient(const String& topic, const String& payload) {
  if (!gMqttClient.connected()) {
    return;
  }
  gMqttClient.publish(topic.c_str(), payload.c_str(), false);
}

String mqttDeviceJson() {
  String out;
  out.reserve(220);
  out += '{';
  out += "\"ids\":[";
  appendJsonQuoted(out, "esplora_" + String(gNodeIdText));
  out += "],\"name\":";
  appendJsonQuoted(out, String("Esplora ") + gNodeIdText);
  out += ",\"mdl\":";
  appendJsonQuoted(out, "XIAO ESP32S3 + Wio-SX1262");
  out += ",\"mf\":";
  appendJsonQuoted(out, "Seeed Studio");
  out += ",\"sw\":";
  appendJsonQuoted(out, "esplora");
  out += '}';
  return out;
}

void mqttPublishDiscovery() {
  if (!gMqttClient.connected() || !gMqttConfig.haDiscoveryEnabled || gMqttDiscoveryPublished) {
    return;
  }

  const String dev = mqttDeviceJson();
  const String avail = mqttAvailabilityTopic();
  const String statusTopic = mqttStatusTopic();
  const String prefix = "homeassistant";
  const String nodeKey = String("esplora_") + gNodeIdText;

  auto publishConfig = [&](const String& component, const String& objectId, const String& payload) {
    const String topic = prefix + "/" + component + "/" + nodeKey + "/" + objectId + "/config";
    gMqttClient.publish(topic.c_str(), payload.c_str(), true);
  };

  publishConfig(
    "sensor",
    "neighbor_count",
    "{\"name\":\"Esplora Neighbor Count\",\"uniq_id\":\"" + nodeKey + "_neighbor_count\",\"stat_t\":\"" + statusTopic +
      "\",\"val_tpl\":\"{{ value_json.neighborCount }}\",\"avty_t\":\"" + avail +
      "\",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\",\"dev\":" + dev + "}"
  );
  publishConfig(
    "sensor",
    "wifi_rssi",
    "{\"name\":\"Esplora WiFi RSSI\",\"uniq_id\":\"" + nodeKey + "_wifi_rssi\",\"stat_t\":\"" + statusTopic +
      "\",\"unit_of_meas\":\"dBm\",\"val_tpl\":\"{{ value_json.wifi.rssi }}\",\"avty_t\":\"" + avail +
      "\",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\",\"dev\":" + dev + "}"
  );
  publishConfig(
    "sensor",
    "last_rx_rssi",
    "{\"name\":\"Esplora Last RX RSSI\",\"uniq_id\":\"" + nodeKey + "_last_rx_rssi\",\"stat_t\":\"" + statusTopic +
      "\",\"unit_of_meas\":\"dBm\",\"val_tpl\":\"{{ value_json.lastRx.rssi }}\",\"avty_t\":\"" + avail +
      "\",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\",\"dev\":" + dev + "}"
  );
  publishConfig(
    "sensor",
    "last_rx_snr",
    "{\"name\":\"Esplora Last RX SNR\",\"uniq_id\":\"" + nodeKey + "_last_rx_snr\",\"stat_t\":\"" + statusTopic +
      "\",\"unit_of_meas\":\"dB\",\"val_tpl\":\"{{ value_json.lastRx.snr }}\",\"avty_t\":\"" + avail +
      "\",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\",\"dev\":" + dev + "}"
  );
  publishConfig(
    "binary_sensor",
    "wifi_connected",
    "{\"name\":\"Esplora WiFi Connected\",\"uniq_id\":\"" + nodeKey + "_wifi_connected\",\"stat_t\":\"" + statusTopic +
      "\",\"val_tpl\":\"{{ 'ON' if value_json.wifi.connected else 'OFF' }}\",\"pl_on\":\"ON\",\"pl_off\":\"OFF\",\"avty_t\":\"" + avail +
      "\",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\",\"dev\":" + dev + "}"
  );
  publishConfig(
    "switch",
    "beacon_enabled",
    "{\"name\":\"Esplora Beacon\",\"uniq_id\":\"" + nodeKey + "_beacon\",\"stat_t\":\"" + statusTopic +
      "\",\"cmd_t\":\"" + String(gMqttConfig.baseTopic) + "/ha/beacon_enabled/set\",\"val_tpl\":\"{{ 'ON' if value_json.status.beaconEnabled else 'OFF' }}\",\"pl_on\":\"ON\",\"pl_off\":\"OFF\",\"state_on\":\"ON\",\"state_off\":\"OFF\",\"avty_t\":\"" + avail +
      "\",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\",\"dev\":" + dev + "}"
  );
  publishConfig(
    "button",
    "ping",
    "{\"name\":\"Esplora Ping\",\"uniq_id\":\"" + nodeKey + "_ping\",\"cmd_t\":\"" + String(gMqttConfig.baseTopic) +
      "/ha/ping/command\",\"pl_prs\":\"PRESS\",\"dev\":" + dev + "}"
  );
  publishConfig(
    "button",
    "beacon_now",
    "{\"name\":\"Esplora Beacon Now\",\"uniq_id\":\"" + nodeKey + "_beacon_now\",\"cmd_t\":\"" + String(gMqttConfig.baseTopic) +
      "/ha/beacon_now/command\",\"pl_prs\":\"PRESS\",\"dev\":" + dev + "}"
  );

  gMqttDiscoveryPublished = true;
}

void mqttPublishStateSnapshot(bool force) {
  if (!gMqttClient.connected()) {
    return;
  }

  if (!force && !gMqttStateDirty && !gMqttNeighborsDirty && !timeReached(gLastMqttStateAt, MQTT_STATE_INTERVAL_MS)) {
    return;
  }

  mqttPublishRetained(mqttStatusTopic(), buildStatusJson());
  if (force || gMqttNeighborsDirty) {
    mqttPublishRetained(mqttNeighborsTopic(), buildNeighborsJson());
  }
  gLastMqttStateAt = millis();
  gMqttStateDirty = false;
  gMqttNeighborsDirty = false;
}

void mqttPublishEventLine(const String& line) {
  if (!gMqttClient.connected()) {
    return;
  }
  String payload;
  payload.reserve(line.length() + 48);
  payload += "{\"seq\":";
  payload += String(gLastLogSequence);
  payload += ",\"line\":";
  appendJsonQuoted(payload, line);
  payload += '}';
  mqttPublishTransient(mqttEventsTopic(), payload);
}

void mqttHandlePayload(const String& topic, const String& payload);

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String t = String(topic);
  String body;
  body.reserve(length);
  for (unsigned int i = 0; i < length; ++i) {
    body += static_cast<char>(payload[i]);
  }
  mqttHandlePayload(t, body);
}

void mqttConnectNow() {
  if (!gMqttConfig.enabled) {
    emitWarn("mqtt is disabled");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    emitWarn("wifi is not connected");
    return;
  }
  if (strlen(gMqttConfig.host) == 0) {
    emitWarn("mqtt host is empty");
    return;
  }

  gMqttClient.setServer(gMqttConfig.host, gMqttConfig.port);
  gMqttDiscoveryPublished = false;
  const String willTopic = mqttAvailabilityTopic();
  const String clientId = "esplora-" + String(gNodeIdText);
  bool ok = false;
  if (strlen(gMqttConfig.user) > 0) {
    ok = gMqttClient.connect(
      clientId.c_str(),
      gMqttConfig.user,
      gMqttConfig.password,
      willTopic.c_str(),
      0,
      true,
      "offline"
    );
  } else {
    ok = gMqttClient.connect(
      clientId.c_str(),
      willTopic.c_str(),
      0,
      true,
      "offline"
    );
  }

  if (!ok) {
    emitWarn("mqtt connect failed, state=" + String(gMqttClient.state()));
    return;
  }

  gMqttClient.publish(willTopic.c_str(), "online", true);
  gMqttClient.subscribe((String(gMqttConfig.baseTopic) + "/cmd").c_str());
  gMqttClient.subscribe((String(gMqttConfig.baseTopic) + "/send").c_str());
  gMqttClient.subscribe((String(gMqttConfig.baseTopic) + "/ha/ping/command").c_str());
  gMqttClient.subscribe((String(gMqttConfig.baseTopic) + "/ha/beacon_now/command").c_str());
  gMqttClient.subscribe((String(gMqttConfig.baseTopic) + "/ha/beacon_enabled/set").c_str());
  emitEvent("MQTT", "STATE=CONNECTED|HOST=" + String(gMqttConfig.host) + "|PORT=" + String(gMqttConfig.port));
  mqttPublishDiscovery();
  mqttPublishStateSnapshot(true);
}

void mqttEnsureConnected() {
  if (!gMqttConfig.enabled || WiFi.status() != WL_CONNECTED) {
    return;
  }
  if (gMqttClient.connected()) {
    return;
  }
  if (!timeReached(gLastMqttReconnectAt, MQTT_RECONNECT_INTERVAL_MS)) {
    return;
  }
  gLastMqttReconnectAt = millis();
  mqttConnectNow();
}

void mqttLoop() {
  if (gMqttClient.connected()) {
    gMqttClient.loop();
    mqttPublishDiscovery();
    mqttPublishStateSnapshot(false);
  } else {
    mqttEnsureConnected();
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
    markMqttStateDirty(true);
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
  markMqttStateDirty();
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
  markMqttStateDirty(true);
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

void updateLastRx(char kind, const String& from, const String& text, float rssi, float snr, long freqErrHz) {
  gLastRxAt = millis();
  gLastRxKind = kind;
  copyString(gLastRxFrom, sizeof(gLastRxFrom), from);
  copyString(gLastRxText, sizeof(gLastRxText), text);
  gLastRxRssi = rssi;
  gLastRxSnr = snr;
  gLastRxFreqErrHz = freqErrHz;
  markMqttStateDirty();
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
    updateLastRx(kind, nodeId, "beacon", rssi, snr, freqErrHz);
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
    updateLastRx(kind, nodeId, text, rssi, snr, freqErrHz);
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
    updateLastRx(kind, nodeId, nonce, rssi, snr, freqErrHz);
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
  updateLastRx('R', "RAW", previewText(packet), rssi, snr, freqErrHz);
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
  emitLine("HELP|mqtt status");
  emitLine("HELP|mqtt host <host>");
  emitLine("HELP|mqtt port <port>");
  emitLine("HELP|mqtt user <user>");
  emitLine("HELP|mqtt pass <haslo>");
  emitLine("HELP|mqtt topic <base/topic>");
  emitLine("HELP|mqtt on|off|toggle");
  emitLine("HELP|mqtt connect");
  emitLine("HELP|mqtt ha on|off|toggle");
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

  if (line.equalsIgnoreCase("mqtt status")) {
    printMqttStatus();
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

  if (line.equalsIgnoreCase("mqtt connect")) {
    mqttConnectNow();
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

  if (line.startsWith("mqtt host ")) {
    String host = line.substring(10);
    host.trim();
    copyString(gMqttConfig.host, sizeof(gMqttConfig.host), host);
    saveMqttPrefs();
    markMqttStateDirty();
    emitOk("mqtt host saved");
    return;
  }

  if (line.startsWith("mqtt port ")) {
    int port = line.substring(10).toInt();
    if (port < 1 || port > 65535) {
      emitError("mqtt port out of range");
      return;
    }
    gMqttConfig.port = static_cast<uint16_t>(port);
    saveMqttPrefs();
    markMqttStateDirty();
    emitOk("mqtt port saved");
    return;
  }

  if (line.startsWith("mqtt user ")) {
    String user = line.substring(10);
    user.trim();
    copyString(gMqttConfig.user, sizeof(gMqttConfig.user), user);
    saveMqttPrefs();
    markMqttStateDirty();
    emitOk("mqtt user saved");
    return;
  }

  if (line.startsWith("mqtt pass ")) {
    String password = line.substring(10);
    password.trim();
    copyString(gMqttConfig.password, sizeof(gMqttConfig.password), password);
    saveMqttPrefs();
    markMqttStateDirty();
    emitOk("mqtt password saved");
    return;
  }

  if (line.startsWith("mqtt topic ")) {
    String topic = line.substring(11);
    topic.trim();
    if (topic.isEmpty()) {
      emitError("mqtt topic cannot be empty");
      return;
    }
    copyString(gMqttConfig.baseTopic, sizeof(gMqttConfig.baseTopic), topic);
    saveMqttPrefs();
    markMqttStateDirty(true);
    emitOk("mqtt topic saved");
    return;
  }

  if (line.equalsIgnoreCase("mqtt on")) {
    gMqttConfig.enabled = true;
    saveMqttPrefs();
    markMqttStateDirty();
    emitOk("mqtt enabled");
    return;
  }

  if (line.equalsIgnoreCase("mqtt off")) {
    gMqttConfig.enabled = false;
    saveMqttPrefs();
    if (gMqttClient.connected()) {
      gMqttClient.publish(mqttAvailabilityTopic().c_str(), "offline", true);
      gMqttClient.disconnect();
    }
    gMqttDiscoveryPublished = false;
    markMqttStateDirty();
    emitOk("mqtt disabled");
    return;
  }

  if (line.equalsIgnoreCase("mqtt toggle")) {
    handleCommand(gMqttConfig.enabled ? "mqtt off" : "mqtt on");
    return;
  }

  if (line.equalsIgnoreCase("mqtt ha on")) {
    gMqttConfig.haDiscoveryEnabled = true;
    gMqttDiscoveryPublished = false;
    saveMqttPrefs();
    markMqttStateDirty();
    emitOk("mqtt ha discovery enabled");
    return;
  }

  if (line.equalsIgnoreCase("mqtt ha off")) {
    gMqttConfig.haDiscoveryEnabled = false;
    saveMqttPrefs();
    markMqttStateDirty();
    emitOk("mqtt ha discovery disabled");
    return;
  }

  if (line.equalsIgnoreCase("mqtt ha toggle")) {
    handleCommand(gMqttConfig.haDiscoveryEnabled ? "mqtt ha off" : "mqtt ha on");
    return;
  }

  if (line.equalsIgnoreCase("beacon on")) {
    gConfig.beaconEnabled = true;
    markMqttStateDirty();
    emitOk("beacon enabled");
    return;
  }

  if (line.equalsIgnoreCase("beacon off")) {
    gConfig.beaconEnabled = false;
    markMqttStateDirty();
    emitOk("beacon disabled");
    return;
  }

  if (line.equalsIgnoreCase("beacon toggle")) {
    handleCommand(gConfig.beaconEnabled ? "beacon off" : "beacon on");
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
    markMqttStateDirty();
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
    markMqttStateDirty();
    emitOk("name=" + String(gNodeName));
    return;
  }

  if (line.equalsIgnoreCase("raw on")) {
    gConfig.rawLoggingEnabled = true;
    markMqttStateDirty();
    emitOk("raw logging enabled");
    return;
  }

  if (line.equalsIgnoreCase("raw off")) {
    gConfig.rawLoggingEnabled = false;
    markMqttStateDirty();
    emitOk("raw logging disabled");
    return;
  }

  if (line.equalsIgnoreCase("raw toggle")) {
    handleCommand(gConfig.rawLoggingEnabled ? "raw off" : "raw on");
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

void mqttHandlePayload(const String& topic, const String& payload) {
  if (topic.endsWith("/cmd")) {
    handleCommand(payload);
    return;
  }
  if (topic.endsWith("/send")) {
    sendMessage(payload);
    return;
  }
  if (topic.endsWith("/ha/ping/command")) {
    sendPing(String(millis()));
    return;
  }
  if (topic.endsWith("/ha/beacon_now/command")) {
    sendBeacon("mqtt");
    return;
  }
  if (topic.endsWith("/ha/beacon_enabled/set")) {
    if (payload.equalsIgnoreCase("ON")) {
      handleCommand("beacon on");
    } else if (payload.equalsIgnoreCase("OFF")) {
      handleCommand("beacon off");
    }
    return;
  }
}

String simpleJsonOk(const String& message) {
  String out;
  out.reserve(message.length() + 32);
  out += "{\"ok\":true,\"message\":";
  appendJsonQuoted(out, message);
  out += '}';
  return out;
}

void handleApiRoot() {
  gWebServer.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleApiState() {
  uint32_t since = 0;
  if (gWebServer.hasArg("since")) {
    since = static_cast<uint32_t>(gWebServer.arg("since").toInt());
  }
  gWebServer.send(200, "application/json; charset=utf-8", buildApiStateJson(since));
}

void handleApiCommand() {
  if (!gWebServer.hasArg("cmd")) {
    gWebServer.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"message\":\"missing cmd\"}");
    return;
  }
  handleCommand(gWebServer.arg("cmd"));
  gWebServer.send(200, "application/json; charset=utf-8", simpleJsonOk("command accepted"));
}

void handleApiSend() {
  if (!gWebServer.hasArg("text")) {
    gWebServer.send(400, "application/json; charset=utf-8", "{\"ok\":false,\"message\":\"missing text\"}");
    return;
  }
  sendMessage(gWebServer.arg("text"));
  gWebServer.send(200, "application/json; charset=utf-8", simpleJsonOk("message queued"));
}

void handleApiWifi() {
  if (gWebServer.hasArg("mode") && gWebServer.arg("mode").equalsIgnoreCase("off")) {
    disconnectWifi();
    gWebServer.send(200, "application/json; charset=utf-8", simpleJsonOk("wifi off"));
    return;
  }

  if (gWebServer.hasArg("ssid")) {
    copyString(gWifiConfig.ssid, sizeof(gWifiConfig.ssid), gWebServer.arg("ssid"));
  }
  if (gWebServer.hasArg("password")) {
    copyString(gWifiConfig.password, sizeof(gWifiConfig.password), gWebServer.arg("password"));
  }
  saveWifiPrefs();

  if (gWebServer.hasArg("connect") && gWebServer.arg("connect") == "1") {
    connectWifi(true);
  }

  gWebServer.send(200, "application/json; charset=utf-8", simpleJsonOk("wifi updated"));
}

void handleApiMqtt() {
  if (gWebServer.hasArg("host")) {
    copyString(gMqttConfig.host, sizeof(gMqttConfig.host), gWebServer.arg("host"));
  }
  if (gWebServer.hasArg("port")) {
    int port = gWebServer.arg("port").toInt();
    if (port >= 1 && port <= 65535) {
      gMqttConfig.port = static_cast<uint16_t>(port);
    }
  }
  if (gWebServer.hasArg("user")) {
    copyString(gMqttConfig.user, sizeof(gMqttConfig.user), gWebServer.arg("user"));
  }
  if (gWebServer.hasArg("password")) {
    copyString(gMqttConfig.password, sizeof(gMqttConfig.password), gWebServer.arg("password"));
  }
  if (gWebServer.hasArg("topic") && !gWebServer.arg("topic").isEmpty()) {
    copyString(gMqttConfig.baseTopic, sizeof(gMqttConfig.baseTopic), gWebServer.arg("topic"));
  }
  saveMqttPrefs();
  gMqttDiscoveryPublished = false;
  if (gMqttClient.connected()) {
    gMqttClient.disconnect();
  }
  markMqttStateDirty(true);
  gWebServer.send(200, "application/json; charset=utf-8", simpleJsonOk("mqtt updated"));
}

void setupWebServerRoutes() {
  gWebServer.on("/", HTTP_GET, handleApiRoot);
  gWebServer.on("/api/state", HTTP_GET, handleApiState);
  gWebServer.on("/api/command", HTTP_POST, handleApiCommand);
  gWebServer.on("/api/send", HTTP_POST, handleApiSend);
  gWebServer.on("/api/wifi", HTTP_POST, handleApiWifi);
  gWebServer.on("/api/mqtt", HTTP_POST, handleApiMqtt);
  gWebServer.onNotFound([]() {
    gWebServer.send(404, "application/json; charset=utf-8", "{\"ok\":false,\"message\":\"not found\"}");
  });
}

void ensureWebServerState() {
  const bool wifiUp = WiFi.status() == WL_CONNECTED;
  if (wifiUp && !gWebServerStarted) {
    gWebServer.begin();
    gWebServerStarted = true;
    emitEvent("WEB", "STATE=STARTED|IP=" + ipToString(WiFi.localIP()));
  } else if (!wifiUp && gWebServerStarted) {
    gWebServer.stop();
    gWebServerStarted = false;
    emitEvent("WEB", "STATE=STOPPED");
  }
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
  loadMqttPrefs();
  applyProfile("balanced");
  gMqttClient.setBufferSize(4096);
  gMqttClient.setCallback(mqttCallback);
  setupWebServerRoutes();

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
  ensureWebServerState();
  printStatus();
  printMqttStatus();
}

void loop() {
  pollSerial();

  ensureWebServerState();
  if (gWebServerStarted) {
    gWebServer.handleClient();
  }
  mqttLoop();

  if (gReceivedFlag) {
    handleReceivedPacket();
  }

  if (gConfig.beaconEnabled && gRadioReady && timeReached(gLastBeaconAt, gConfig.beaconIntervalMs)) {
    sendBeacon("interval");
  }
}
