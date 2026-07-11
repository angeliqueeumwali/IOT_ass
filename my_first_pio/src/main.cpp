#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <secrets.h>

WebServer server(80);

const char *ap_ssid = "ESP32_Dashboard";
const char *ap_pass = "12345678";
const int LED_PIN = 2;

unsigned long lastBlinkTime = 0;
int blinkDelay = 500;
bool ledState = false;

String getDashboardPage()
{
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>ESP32 Control Dashboard</title>
    <style>
        body { font-family: 'Segoe UI', Arial, sans-serif; background-color: #F0F4F8; margin: 0; padding: 20px; color: #333; }
        .container { max-width: 900px; margin: 0 auto; display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }
        @media (max-width: 768px) { .container { grid-template-columns: 1fr; } }
        .card { background: white; padding: 25px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.05); border-top: 5px solid #34754E; }
        h2 { color: #34754E; margin-top: 0; margin-bottom: 20px; }
        button { background-color: #34754E; color: white; border: none; padding: 12px 20px; font-size: 1rem; border-radius: 6px; cursor: pointer; width: 100%; transition: background 0.2s; }
        button:hover { background-color: #34754E; }
        input[type='number'], input[type='text'], input[type='password'] { width: 100%; padding: 10px; margin: 10px 0 20px 0; border: 1px solid #ccc; border-radius: 6px; box-sizing: border-box; font-size: 1rem; }
        .wifi-item { display: flex; justify-content: space-between; align-items: center; padding: 10px; margin: 8px 0; background: #F8F9FA; border-radius: 6px; border: 1px solid #E9ECEF; cursor: pointer; }
        .wifi-item:hover { background: #E9ECEF; }
        .signal { font-weight: bold; color: #28A745; }
        .modal { display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.5); justify-content: center; align-items: center; }
        .modal-content { background: white; padding: 25px; border-radius: 12px; width: 90%; max-width: 400px; }
    </style>
</head>
<body>
    <h1 style='text-align:center; color:#0056b3; margin-bottom:30px;'>ESP32 Device Dashboard</h1>
    <div class='container'>
        <div class='card'>
            <h2>Wi-Fi Networks</h2>
            <button onclick='scanWifi()'>Scan Nearby Networks</button>
            <div id='wifi-list' style='margin-top: 20px;'></div>
        </div>

        <div class='card'>
            <h2>LED Control</h2>
            <label for='delayInput'>Blink Delay (milliseconds):</label>
            <input type='number' id='delayInput' min='0' placeholder='e.g., 500' value='
)rawliteral';

    // Inject the dynamic variable into the HTML string split
    html += String(blinkDelay);

    html += R"rawliteral('>
            <button onclick='updateDelay()'>Update Delay</button>
            <p id='status-msg' style='margin-top:15px; font-weight:bold; color:#0056b3;'></p>
        </div>
    </div>

    <div id='wifiModal' class='modal'>
        <div class='modal-content'>
            <h3>Connect to <span id='modal-ssid'></span></h3>
            <input type='hidden' id='selected-ssid'>
            <label>Password:</label><input type='password' id='wifi-pass' placeholder='Enter Wi-Fi password'>
            <button onclick='connectWifi()' style='margin-bottom:10px;'>Connect</button>
            <button onclick='closeModal()' style='background-color:#6c757d;'>Cancel</button>
        </div>
    </div>

    <script>
    function scanWifi() {
      const list = document.getElementById('wifi-list');
      list.innerHTML = 'Scanning networks, please wait...';
      fetch('/scan').then(res => res.json()).then(data => {
        list.innerHTML = '';
        if(data.length === 0) { list.innerHTML = 'No networks found.'; return; }
        data.forEach(net => {
          const div = document.createElement('div');
          div.className = 'wifi-item';
          div.onclick = () => openModal(net.ssid);
          div.innerHTML = `<span>${net.ssid} (${net.crypto})</span><span class='signal'>${net.rssi} dBm</span>`;
          list.appendChild(div);
        });
      });
    }

    function updateDelay() {
      const delayVal = document.getElementById('delayInput').value;
      if(!delayVal || delayVal < 0) { alert('Please enter a valid positive number'); return; }
      fetch(`/set_delay?val=${delayVal}`).then(res => res.text()).then(msg => {
        document.getElementById('status-msg').innerText = 'Status: ' + msg;
      });
    }

    function openModal(ssid) {
      document.getElementById('selected-ssid').value = ssid;
      document.getElementById('modal-ssid').innerText = ssid;
      document.getElementById('wifiModal').style.display = 'flex';
    }
    function closeModal() {
      document.getElementById('wifiModal').style.display = 'none';
      document.getElementById('wifi-pass').value = '';
    }

    function connectWifi() {
      const ssid = document.getElementById('selected-ssid').value;
      const pass = document.getElementById('wifi-pass').value;
      closeModal();
      document.getElementById('wifi-list').innerHTML = `Attempting connection to ${ssid}... Check serial monitor or refresh page at your new IP shortly.`;
      fetch(`/connect?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`);
    }
    </script>
</body>
</html>
)rawliteral";

    return html;
}

void handleRoot()
{
    server.send(200, "text/html", getDashboardPage());
}

void handleScan()
{
    int n = WiFi.scanNetworks();
    int indices[n];
    for (int i = 0; i < n; i++)
        indices[i] = i;
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i]))
            {
                int temp = indices[i];
                indices[i] = indices[j];
                indices[j] = temp;
            }
        }
    }

    String json = "[";
    for (int i = 0; i < n; ++i)
    {
        if (i > 0)
            json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(indices[i]) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(indices[i])) + ",";
        json += "\"crypto\":\"" + String((WiFi.encryptionType(indices[i]) == WIFI_AUTH_OPEN) ? "Open" : "Secured") + "\"";
        json += "}";
    }
    json += "]";

    server.send(200, "application/json", json);
}

void handleSetDelay()
{
    if (server.hasArg("val"))
    {
        int val = server.arg("val").toInt();
        blinkDelay = val;
        if (blinkDelay == 0)
        {
            digitalWrite(LED_PIN, LOW);
            server.send(200, "text/plain", "LED Turned OFF (Delay 0)");
        }
        else
        {
            server.send(200, "text/plain", "Delay updated to " + String(blinkDelay) + "ms");
        }
    }
    else
    {
        server.send(400, "text/plain", "Bad Request");
    }
}

void handleConnect()
{
    if (server.hasArg("ssid") && server.hasArg("pass"))
    {
        String req_ssid = server.arg("ssid");
        String req_pass = server.arg("pass");

        server.send(200, "text/plain", "Connecting...");
        delay(100);

        WiFi.disconnect();
        WiFi.begin(req_ssid.c_str(), req_pass.c_str());

        Serial.print("Connecting to target network: ");
        Serial.println(req_ssid);
    }
    else
    {
        server.send(400, "text/plain", "Missing Parameters");
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ap_ssid, ap_pass);

    Serial.println("\n--- ESP32 Dashboard Initialization ---");
    Serial.print("Local Configuration Wi-Fi Name: ");
    Serial.println(ap_ssid);
    Serial.print("Access Dashboard Portal at IP Address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/scan", handleScan);
    server.on("/set_delay", handleSetDelay);
    server.on("/connect", handleConnect);

    server.begin();
    Serial.println("HTTP Web Server successfully started.");
}

void loop()
{
    server.handleClient();

    if (blinkDelay > 0)
    {
        unsigned long currentMillis = millis();
        if (currentMillis - lastBlinkTime >= (unsigned long)blinkDelay)
        {
            lastBlinkTime = currentMillis;
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
        }
    }
}


