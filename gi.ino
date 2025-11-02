#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// AP Configuration
const char* ssid = "bara";
const char* password = "A7med@Elshab7";

WebServer server(80);

String getHackerStylePage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>BARA - Advanced Wi-Fi Scanner</title>
  <style>
    body {
      background-color: #000;
      color: #0f0;
      font-family: 'Courier New', monospace;
      margin: 0;
      padding: 20px;
      overflow-x: hidden;
    }
    h1 {
      text-align: center;
      text-shadow: 0 0 10px #0f0, 0 0 20px #0f0;
      letter-spacing: 3px;
      font-size: 32px;
    }
    .header {
      border-bottom: 1px solid #0f0;
      padding-bottom: 10px;
      margin-bottom: 20px;
    }
    .scan-btn {
      background: transparent;
      color: #0f0;
      border: 1px solid #0f0;
      padding: 10px 20px;
      font-family: 'Courier New', monospace;
      font-size: 18px;
      cursor: pointer;
      margin: 20px auto;
      display: block;
      transition: all 0.3s;
    }
    .scan-btn:hover {
      background: #0f0;
      color: #000;
      box-shadow: 0 0 15px #0f0;
    }
    .networks {
      margin-top: 20px;
    }
    .net-item {
      border: 1px solid #0a0;
      margin: 10px 0;
      padding: 10px;
      background: rgba(0, 30, 0, 0.3);
    }
    .footer {
      margin-top: 40px;
      text-align: center;
      font-size: 14px;
      color: #0a0;
    }
    .blink {
      animation: blink 1s infinite;
    }
    @keyframes blink {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.3; }
    }
  </style>
</head>
<body>
  <div class="header">
    <h1 class="blink">[ BARA Wi-Fi SCANNER ]</h1>
    <p style="text-align:center; color:#0a0;">Developer: Ahmed Noor Ahmed - Qena</p>
  </div>

  <button class="scan-btn" onclick="scan()">[ SCAN NEARBY NETWORKS ]</button>

  <div id="results" class="networks"></div>

  <div class="footer">
    <p>BARA - Ethical Wi-Fi Analysis Tool | For Authorized Use Only</p>
  </div>

  <script>
    function scan() {
      document.getElementById('results').innerHTML = '<p style="text-align:center;">Scanning... Please wait...</p>';
      fetch('/scan')
        .then(response => response.json())
        .then(data => {
          let html = '';
          if (data.networks && data.networks.length > 0) {
            data.networks.forEach(net => {
              html += `<div class="net-item">` +
                      `<strong>SSID:</strong> ${net.ssid} <br>` +
                      `<strong>RSSI:</strong> ${net.rssi} dBm <br>` +
                      `<strong>Channel:</strong> ${net.channel} <br>` +
                      `<strong>Encryption:</strong> ${net.encryption}` +
                      `</div>`;
            });
          } else {
            html = '<p style="text-align:center; color:#f00;">No networks found.</p>';
          }
          document.getElementById('results').innerHTML = html;
        })
        .catch(err => {
          document.getElementById('results').innerHTML = '<p style="text-align:center; color:#f00;">Scan failed!</p>';
        });
    }
  </script>
</body>
</html>
  )rawliteral";
  return html;
}

void handleRoot() {
  server.send(200, "text/html", getHackerStylePage());
}

void handleScan() {
  int n = WiFi.scanNetworks();
  String json = "{\"networks\":[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"channel\":" + String(WiFi.channel(i)) + ",";
    json += "\"encryption\":\"";
    switch (WiFi.encryptionType(i)) {
      case WIFI_AUTH_OPEN:          json += "Open"; break;
      case WIFI_AUTH_WEP:           json += "WEP"; break;
      case WIFI_AUTH_WPA_PSK:       json += "WPA"; break;
      case WIFI_AUTH_WPA2_PSK:      json += "WPA2"; break;
      case WIFI_AUTH_WPA_WPA2_PSK:  json += "WPA/WPA2"; break;
      case WIFI_AUTH_WPA2_ENTERPRISE: json += "WPA2-EAP"; break;
      default:                      json += "Unknown";
    }
    json += "\"}";
  }
  json += "]}";
  server.send(200, "application/json", json);
  WiFi.scanDelete(); // Free memory
}

void setup() {
  Serial.begin(115200);

  // Start AP
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started:");
  Serial.print("SSID: "); Serial.println(ssid);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  // Web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/scan", HTTP_GET, handleScan);
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}
