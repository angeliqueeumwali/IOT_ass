#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "secrets.h" 

WebServer server(80);


const int LED_PIN = 2;


unsigned long lastBlinkTime = 0;
int blinkDelay = 500; 
bool ledState = false;


String getDashboardPage();


void handleRoot() {
    server.send(200, "text/html", getDashboardPage());
}


void handleScan() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"crypto\":\"" + String((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Protected") + "\"";
        json += "}";
    }
    json += "]";
    server.send(200, "application/json", json);
}


void handleSetDelay() {
    if (server.hasArg("val")) {
        int newVal = server.arg("val").toInt();
        if (newVal >= 0) {
            blinkDelay = newVal;
            server.send(200, "text/plain", "Delay updated to " + String(blinkDelay) + "ms");
            return;
        }
    }
    server.send(400, "text/plain", "Invalid input value");
}


void handleConnect() {
    if (server.hasArg("ssid")) {
        String target_ssid = server.arg("ssid");
        String target_pass = server.arg("pass");
        
        server.send(200, "text/plain", "Connecting...");
        delay(500); // Give the response time to transmit
        
        Serial.printf("Connecting to station: %s\n", target_ssid.c_str());
        WiFi.begin(target_ssid.c_str(), target_pass.c_str());
    } else {
        server.send(400, "text/plain", "SSID Missing");
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);

    
    WiFi.mode(WIFI_AP_STA); 

    Serial.print("Configuring Access Point: ");
    Serial.println(ap_ssid);
    WiFi.softAP(ap_ssid, ap_pass);

    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/scan", handleScan);
    server.on("/set_delay", handleSetDelay);
    server.on("/connect", handleConnect);
    
    server.begin();
    Serial.println("HTTP Server active and running.");
}


void loop() {
    server.handleClient();

    if (blinkDelay > 0) {
        unsigned long currentMillis = millis();
        if (currentMillis - lastBlinkTime >= (unsigned long)blinkDelay) {
            lastBlinkTime = currentMillis;
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
        }
    } else {
        digitalWrite(LED_PIN, LOW); 
    }
}


String getDashboardPage() {
    String html = "<!DOCTYPE html><html lang='en'><head>";
    html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>ESP32 Control Dashboard</title>";
    html += "<style>";
    html += "body { font-family: 'Segoe UI', Arial, sans-serif; background-color: #F0F4F8; margin: 0; padding: 20px; color: #333; }";
    html += ".container { max-width: 900px; margin: 0 auto; display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }";
    html += "@media (max-width: 768px) { .container { grid-template-columns: 1fr; } }";
    html += ".card { background: white; padding: 25px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.05); border-top: 5px solid #34754E; }";
    html += "h2 { color: #34754E; margin-top: 0; margin-bottom: 20px; }";
    html += "button { background-color: #34754E; color: white; border: none; padding: 12px 20px; font-size: 1rem; border-radius: 6px; cursor: pointer; width: 100%; transition: background 0.2s; }";
    html += "button:hover { background-color: #265639; }";
    html += "input[type='number'], input[type='text'], input[type='password'] { width: 100%; padding: 10px; margin: 10px 0 20px 0; border: 1px solid #ccc; border-radius: 6px; box-sizing: border-box; font-size: 1rem; }";
    html += ".wifi-item { display: flex; justify-content: space-between; align-items: center; padding: 10px; margin: 8px 0; background: #F8F9FA; border-radius: 6px; border: 1px solid #E9ECEF; cursor: pointer; }";
    html += ".wifi-item:hover { background: #E9ECEF; }";
    html += ".signal { font-weight: bold; color: #28A745; }";
    html += ".modal { display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.5); justify-content: center; align-items: center; }";
    html += ".modal-content { background: white; padding: 25px; border-radius: 12px; width: 90%; max-width: 400px; }";
    html += "</style></head><body>";

    html += "<h1 style='text-align:center; color:#34754E; margin-bottom:30px;'>ESP32 Device Dashboard</h1>";
    html += "<div class='container'>";

    html += "<div class='card'>";
    html += "<h2>Wi-Fi Networks</h2>";
    html += "<button onclick='scanWifi()'>Scan Nearby Networks</button>";
    html += "<div id='wifi-list' style='margin-top: 20px;'></div>";
    html += "</div>";

    html += "<div class='card'>";
    html += "<h2>LED Control</h2>";
    html += "<label for='delayInput'>Blink Delay (milliseconds):</label>";
    html += "<input type='number' id='delayInput' min='0' placeholder='e.g., 500' value='" + String(blinkDelay) + "'>";
    html += "<button onclick='updateDelay()'>Update Delay</button>";
    html += "<p id='status-msg' style='margin-top:15px; font-weight:bold; color:#34754E;'></p>";
    html += "</div>";

    html += "</div>";

    html += "<div id='wifiModal' class='modal'><div class='modal-content'>";
    html += "<h3>Connect to <span id='modal-ssid'></span></h3>";
    html += "<input type='hidden' id='selected-ssid'>";
    html += "<label>Password:</label><input type='password' id='wifi-pass' placeholder='Enter Wi-Fi password'>";
    html += "<button onclick='connectWifi()' style='margin-bottom:10px;'>Connect</button>";
    html += "<button onclick='closeModal()' style='background-color:#6c757d;'>Cancel</button>";
    html += "</div></div>";

    html += "<script>";
    html += "function scanWifi() {";
    html += "  const list = document.getElementById('wifi-list');";
    html += "  list.innerHTML = 'Scanning networks, please wait...';";
    html += "  fetch('/scan').then(res => res.json()).then(data => {";
    html += "    list.innerHTML = '';";
    html += "    if(data.length === 0) { list.innerHTML = 'No networks found.'; return; }";
    html += "    data.forEach(net => {";
    html += "      const div = document.createElement('div');";
    html += "      div.className = 'wifi-item';";
    html += "      div.onclick = () => openModal(net.ssid);";
    html += "      div.innerHTML = `<span>${net.ssid} (${net.crypto})</span><span class='signal'>${net.rssi} dBm</span>`;";
    html += "      list.appendChild(div);";
    html += "    });";
    html += "  });";
    html += "}";

    html += "function updateDelay() {";
    html += "  const delayVal = document.getElementById('delayInput').value;";
    html += "  if(!delayVal || delayVal < 0) { alert('Please enter a valid positive number'); return; }";
    html += "  fetch(`/set_delay?val=${delayVal}`).then(res => res.text()).then(msg => {";
    html += "    document.getElementById('status-msg').innerText = 'Status: ' + msg;";
    html += "  });";
    html += "}";

    html += "function openModal(ssid) {";
    html += "  document.getElementById('selected-ssid').value = ssid;";
    html += "  document.getElementById('modal-ssid').innerText = ssid;";
    html += "  document.getElementById('wifiModal').style.display = 'flex';";
    html += "}";
    html += "function closeModal() {";
    html += "  document.getElementById('wifiModal').style.display = 'none';";
    html += "  document.getElementById('wifi-pass').value = '';";
    html += "}";

    html += "function connectWifi() {";
    html += "  const ssid = document.getElementById('selected-ssid').value;";
    html += "  const pass = document.getElementById('wifi-pass').value;";
    html += "  closeModal();";
    html += "  document.getElementById('wifi-list').innerHTML = `Attempting connection to ${ssid}...`;";
    html += "  fetch(`/connect?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`);";
    html += "}";
    html += "</script></body></html>";

    return html;
}
