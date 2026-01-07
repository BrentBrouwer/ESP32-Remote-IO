#include <WiFi.h>
#include <WebServer.h>
#include "Festo/FestoCmmsControl.h"

// --- Configuration ---
const int LED_ALIVE_PIN = 2;
const int AliveOnPeriodTime = 100;
const int AliveOffPeriodTime = 2000;
unsigned long AliveLedStatusChanged = 0;

// const char *ssid = "Sjaan-24G";
// const char *password = "Janrenlen1";
const char *ssid = "vBakel";
const char *password = "1001100111";

WebServer server(80);
FestoCmmsControl *m_FestoControl;

// --- 1. READ-ONLY OUTPUTS ---
int outputPins[] = {2, 4, 5, 12, 13, 14, 15, 16, 17, 18};
const char *outputLabels[] = {
    "Keep-Alive", "Enable controller [DIN5]", "Disable stop [DIN13]", "Enable motor [DIN4]", "Start positioning [DIN8]",
    "Record bit 0 [DIN0]", "Record bit 1 [DIN1]", "Record bit 2 [DIN2]", "Record bit 3 [DIN3]", "Record bit 4 [DIN10]"};
int outputCount = sizeof(outputPins) / sizeof(int);

// --- 2. INPUTS ---
int inputPins[] = {32, 33, 34, 35};
int inputCount = sizeof(inputPins) / sizeof(int);

// --- 3. CUSTOM ACTION BUTTONS ---
const int DefinedActions = 5;
bool actionStates[DefinedActions] = {false, false, false, false, false};
char actionLabels[DefinedActions][32] = {"Enable", "Home", "Pos 1", "Pos 2", "All off"};

// NEW: Define if button is a Toggle (true) or a Momentary Click (false)
bool actionIsToggle[DefinedActions] = {
    true,  // Enable (Toggle)
    false, // Home (Click)
    false, // Position 1 (Click)
    false, // Position 2 (Click)
    false  // Reset (Click)
};

// --- PLACE YOUR CUSTOM IMPLEMENTATIONS HERE ---
void customAction1()
{
    if (actionStates[0])
    {
        strcpy(actionLabels[0], "Disable");
        m_FestoControl->EnableController();
    }
    else
    {
        strcpy(actionLabels[0], "Enable");
        m_FestoControl->DisableController();
    }
}

void customAction2() { m_FestoControl->Home(); }
void customAction3() { m_FestoControl->GoToPosition(1); }
void customAction4() { m_FestoControl->GoToPosition(2); }
void customAction5() { m_FestoControl->AllOff(); }

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
        .card { background: white; padding: 15px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); width: 150px; }
        .card-action { border-top: 5px solid #ff9800; }
        .card-out { border-top: 5px solid #2196f3; }
        .card-in { border-top: 5px solid #9e9e9e; }
        .label { font-weight: bold; font-size: 0.9em; display: block; margin-bottom: 5px; height: 35px; overflow: hidden; }
        .status { font-weight: bold; padding: 4px; border-radius: 4px; font-size: 0.85em; margin-bottom: 10px; }
        .HIGH { background-color: #e8f5e9; color: #2e7d32; }
        .LOW { background-color: #f0f0f0; color: #666; }
        button { padding: 10px; width: 100%; cursor: pointer; border: none; border-radius: 5px; font-weight: bold; color: white; transition: 0.2s; }
        .btn-toggle { background-color: #ff9800; }
        .btn-click { background-color: #607d8b; }
        button:active { transform: scale(0.95); opacity: 0.8; }
    </style>
</head>
<body>
    <h2>ESP32 Festo Control</h2>
    <h3 class="section-title">Control Actions</h3>
    <div class="container" id="action-container"></div>
    <h3 class="section-title">ESP32 -> Festo</h3>
    <div class="container" id="output-container"></div>
    <h3 class="section-title">ESP32 <- Festo</h3>
    <div class="container" id="input-container"></div>

    <script>
        function triggerAction(id) {
            fetch(`/action?id=${id}`).then(() => updateStatus());
        }

        function setInputName(id) {
            if (id == 32) return "Enabled";
            if (id == 33) return "Motion finished";
            if (id == 34) return "Acknowledge start";
            if (id == 35) return "Error";
            return "Input " + id;
        }

        function updateStatus() {
            fetch('/status').then(r => r.json()).then(data => {
                document.getElementById('action-container').innerHTML = data.actions.map((a, i) => `
                    <div class="card card-action">
                        <span class="label">${a.label}</span>
                        <div class="status ${a.state}">${a.isToggle ? a.state : 'READY'}</div>
                        <button class="${a.isToggle ? 'btn-toggle' : 'btn-click'}" onclick="triggerAction(${i})">
                            ${a.isToggle ? 'TOGGLE' : 'CLICK'}
                        </button>
                    </div>`).join('');

                document.getElementById('output-container').innerHTML = data.outputs.map(o => `
                    <div class="card card-out">
                        <span class="label">${o.label}</span>
                        <div class="status ${o.state}">${o.state}</div>
                    </div>`).join('');

                document.getElementById('input-container').innerHTML = data.inputs.map(i => `
                    <div class="card card-in">
                        <span class="label">${setInputName(i.pin)}</span>
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

void handleAction()
{
    if (server.hasArg("id"))
    {
        int id = server.arg("id").toInt();
        if (id >= 0 && id < DefinedActions)
        {
            // Toggle state
            actionStates[id] = !actionStates[id];

            // Execute function
            switch (id)
            {
                case 0:
                    customAction1();
                    break;
                case 1:
                    customAction2();
                    break;
                case 2:
                    customAction3();
                    break;
                case 3:
                    customAction4();
                    break;
                case 4:
                    customAction5();
                    break;
            }

            // If it's a click button, we reset state to LOW immediately for logic purposes
            if (!actionIsToggle[id])
            {
                actionStates[id] = false;
            }

            server.send(200, "text/plain", "OK");
            return;
        }
    }
    server.send(400, "text/plain", "Invalid Action");
}

void handleStatus()
{
    String json = "{\"actions\":[";
    for (int i = 0; i < DefinedActions; i++)
    {
        json += "{\"label\":\"" + String(actionLabels[i]) + "\",";
        json += "\"state\":\"" + String(actionStates[i] ? "HIGH" : "LOW") + "\",";
        json += "\"isToggle\":" + String(actionIsToggle[i] ? "true" : "false") + "}";
        if (i < (DefinedActions - 1))
            json += ",";
    }
    json += "],\"outputs\":[";
    for (int i = 0; i < outputCount; i++)
    {
        json += "{\"pin\":" + String(outputPins[i]) + ",\"label\":\"" + String(outputLabels[i]) + "\",\"state\":\"" + (digitalRead(outputPins[i]) ? "HIGH" : "LOW") + "\"}";
        if (i < outputCount - 1)
            json += ",";
    }
    json += "],\"inputs\":[";
    for (int i = 0; i < inputCount; i++)
    {
        json += "{\"pin\":" + String(inputPins[i]) + ",\"state\":\"" + (digitalRead(inputPins[i]) ? "HIGH" : "LOW") + "\"}";
        if (i < inputCount - 1)
            json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
}

void ControlAliveLed()
{
    bool on = digitalRead(LED_ALIVE_PIN);
    unsigned long now = millis();
    if (on && (now - AliveLedStatusChanged >= AliveOnPeriodTime))
    {
        AliveLedStatusChanged = now;
        digitalWrite(LED_ALIVE_PIN, LOW);
    }
    else if (!on && (now - AliveLedStatusChanged >= AliveOffPeriodTime))
    {
        AliveLedStatusChanged = now;
        digitalWrite(LED_ALIVE_PIN, HIGH);
    }
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED_ALIVE_PIN, OUTPUT);
    for (int i = 0; i < outputCount; i++)
    {
        pinMode(outputPins[i], OUTPUT);
        digitalWrite(outputPins[i], LOW);
    }
    for (int i = 0; i < inputCount; i++)
    {
        pinMode(inputPins[i], INPUT_PULLUP);
    }

    m_FestoControl = new FestoCmmsControl(4, 12, 5, 13, 14, 15, 16, 17, 18, 32, 33, 34, 35);

    WiFi.begin(ssid, password);
    Serial.print("Connecting..");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.print("\nAddress: ");
    Serial.println(WiFi.localIP());

    server.on("/", []()
              { server.send(200, "text/html", INDEX_HTML); });
    server.on("/action", handleAction);
    server.on("/status", handleStatus);
    server.begin();
    Serial.println("\nSetup executed");
}

void loop()
{
    ControlAliveLed();
    server.handleClient();
}