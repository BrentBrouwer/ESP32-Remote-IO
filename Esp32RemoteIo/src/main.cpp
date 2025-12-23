#include <WiFi.h>
#include <WebServer.h>
#include "Festo/FestoCmmsControl.h"

// --- Configuration ---
const int LED_ALIVE_PIN = 2;
const int AliveOnPeriodTime = 100;
const int AliveOffPeriodTime = 2000;
unsigned long AliveLedStatusChanged = 0;

const char *ssid = "Sjaan-24G";
const char *password = "Janrenlen1";

WebServer server(80);

// --- 1. READ-ONLY OUTPUTS ---
int outputPins[] = {2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27};
const char *outputLabels[] = {
    "Keep-Alive", "Enable controller [DIN5]", "Disable stop [DIN13]", "Enable motor [DIN4]", "Start positioning [DIN8]",
    "Record bit 0 [DIN0]", "Record bit 1 [DIN1]", "Output 16", "Output 17",
    "Output 18", "Output 19", "Output 21", "Output 22",
    "Output 23", "Output 25", "Output 26", "Output 27"};
int outputCount = sizeof(outputPins) / sizeof(int);

// --- 2. INPUTS ---
int inputPins[] = {32, 33, 34, 35, 36, 39};
int inputCount = sizeof(inputPins) / sizeof(int);

// --- 3. CUSTOM ACTION BUTTONS (5 Cards) ---
bool actionStates[5] = {false, false, false, false, false};
const char* actionLabels[] = {"Enable", "Home", "Position 1", "Postion 2", "Spare (empty)"};

// --- PLACE YOUR CUSTOM IMPLEMENTATIONS HERE ---
void customAction1() { Home(); }
void customAction2() { /* Add your code for Button 2 here */ }
void customAction3() { /* Add your code for Button 3 here */ }
void customAction4() { /* Add your code for Button 4 here */ }
void customAction5() { /* Add your code for Button 5 here */ }

// --- HTML & UI ---
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Advanced Panel</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: 'Segoe UI', Arial, sans-serif; text-align: center; background-color: #f0f2f5; margin: 0; padding: 20px; }
        .section-title { margin-top: 30px; color: #444; border-bottom: 2px solid #ddd; display: inline-block; padding: 0 20px 5px; }
        .container { display: flex; flex-wrap: wrap; justify-content: center; gap: 15px; padding: 15px; }
        .card { background: white; padding: 15px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); width: 140px; }
        .card-action { border-top: 5px solid #ff9800; }
        .card-out { border-top: 5px solid #2196f3; }
        .card-in { border-top: 5px solid #9e9e9e; }
        .label { font-weight: bold; font-size: 0.9em; display: block; margin-bottom: 5px; }
        .sub { font-size: 0.75em; color: #888; display: block; margin-bottom: 10px; }
        .status { font-weight: bold; padding: 4px; border-radius: 4px; font-size: 0.85em; margin-bottom: 10px; }
        .HIGH { background-color: #e8f5e9; color: #2e7d32; }
        .LOW { background-color: #ffebee; color: #c62828; }
        button { padding: 8px; width: 100%; cursor: pointer; border: none; border-radius: 5px; background-color: #ff9800; color: white; font-weight: bold; }
        button:hover { opacity: 0.8; }
    </style>
</head>
<body>
    <h2>ESP32 Control Panel</h2>

    <h3 class="section-title">Control Actions</h3>
    <div class="container" id="action-container"></div>

    <h3 class="section-title">Output Monitoring (Read-Only)</h3>
    <div class="container" id="output-container"></div>

    <h3 class="section-title">Input Status</h3>
    <div class="container" id="input-container"></div>

    <script>
        function triggerAction(id) {
            fetch(`/action?id=${id}`).then(() => updateStatus());
        }

        function updateStatus() {
            fetch('/status').then(r => r.json()).then(data => {
                const actCont = document.getElementById('action-container');
                const outCont = document.getElementById('output-container');
                const inCont = document.getElementById('input-container');

                actCont.innerHTML = data.actions.map((a, i) => `
                    <div class="card card-action">
                        <span class="label">${a.label}</span>
                        <span class="sub">Custom Logic</span>
                        <div class="status ${a.state}">${a.state}</div>
                        <button onclick="triggerAction(${i})">TOGGLE</button>
                    </div>`).join('');

                outCont.innerHTML = data.outputs.map(o => `
                    <div class="card card-out">
                        <span class="label">${o.label}</span>
                        <span class="sub">GPIO ${o.pin}</span>
                        <div class="status ${o.state}">${o.state}</div>
                    </div>`).join('');

                inCont.innerHTML = data.inputs.map(i => `
                    <div class="card card-in">
                        <span class="label">Sensor</span>
                        <span class="sub">GPIO ${i.pin}</span>
                        <div class="status ${i.state}">${i.state}</div>
                    </div>`).join('');
            });
        }
        setInterval(updateStatus, 2000);
        window.onload = updateStatus;
    </script>
</body>
</html>
)rawliteral";

// --- Server Handlers ---

void handleAction() {
    if (server.hasArg("id")) {
        int id = server.arg("id").toInt();
        if (id >= 0 && id < 5) {
            actionStates[id] = !actionStates[id];
            // Call the specific empty method
            switch(id) {
                case 0: customAction1(); break;
                case 1: customAction2(); break;
                case 2: customAction3(); break;
                case 3: customAction4(); break;
                case 4: customAction5(); break;
            }
            server.send(200, "text/plain", "OK");
            return;
        }
    }
    server.send(400, "text/plain", "Invalid Action");
}

void handleStatus() {
    String json = "{\"actions\":[";
    for (int i = 0; i < 5; i++) {
        json += "{\"label\":\"" + String(actionLabels[i]) + "\",\"state\":\"" + (actionStates[i] ? "HIGH" : "LOW") + "\"}";
        if (i < 4) json += ",";
    }
    json += "],\"outputs\":[";
    for (int i = 0; i < outputCount; i++) {
        json += "{\"pin\":" + String(outputPins[i]) + ",\"label\":\"" + String(outputLabels[i]) + "\",\"state\":\"" + (digitalRead(outputPins[i]) ? "HIGH" : "LOW") + "\"}";
        if (i < outputCount - 1) json += ",";
    }
    json += "],\"inputs\":[";
    for (int i = 0; i < inputCount; i++) {
        json += "{\"pin\":" + String(inputPins[i]) + ",\"state\":\"" + (digitalRead(inputPins[i]) ? "HIGH" : "LOW") + "\"}";
        if (i < inputCount - 1) json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
}

// ... (Rest of setup and loop from your original code remains the same)

void ControlAliveLed() {
    bool on = digitalRead(LED_ALIVE_PIN);
    unsigned long now = millis();
    if (on) {
        if (now - AliveLedStatusChanged >= AliveOnPeriodTime) {
            AliveLedStatusChanged = now;
            digitalWrite(LED_ALIVE_PIN, LOW);
        }
    } else {
        if (now - AliveLedStatusChanged >= AliveOffPeriodTime) {
            AliveLedStatusChanged = now;
            digitalWrite(LED_ALIVE_PIN, HIGH);
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_ALIVE_PIN, OUTPUT);
    for (int i = 0; i < outputCount; i++) { pinMode(outputPins[i], OUTPUT); digitalWrite(outputPins[i], LOW); }
    for (int i = 0; i < inputCount; i++) { pinMode(inputPins[i], INPUT_PULLUP); }
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    server.on("/", [](){ server.send(200, "text/html", INDEX_HTML); });
    server.on("/action", handleAction);
    server.on("/status", handleStatus);
    server.begin();
}

void loop() {
    ControlAliveLed();
    server.handleClient();
}