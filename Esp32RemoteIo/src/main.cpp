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
bool actionIsToggle[DefinedActions] = {true, false, false, false, false};

// --- 4. CYCLE CONTROL VARIABLES ---
int cycleRemaining = 0;
int currentCycle = 0;
int currentStep = 0;
int cycleDelayMs = 1000; // Value for delay between steps
bool stepInitiated = false;
unsigned long stepTimer = 0;

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
void customAction5()
{
    m_FestoControl->AllOff();
    cycleRemaining = 0;
}

// --- HTML & UI ---
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Festo Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: 'Segoe UI', Arial, sans-serif; text-align: center; background-color: #f0f2f5; margin: 0; padding: 20px; }
        .section-title { margin-top: 30px; color: #444; border-bottom: 2px solid #ddd; display: inline-block; padding: 0 20px 5px; }
        .container { display: flex; flex-wrap: wrap; justify-content: center; gap: 15px; padding: 15px; }
        .card { background: white; padding: 15px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); width: 160px; }
        .card-action { border-top: 5px solid #ff9800; }
        .card-cycle { border-top: 5px solid #4caf50; width: 250px; }
        .card-out { border-top: 5px solid #2196f3; }
        .card-in { border-top: 5px solid #9e9e9e; }
        .label { font-weight: bold; font-size: 0.9em; display: block; margin-bottom: 5px; min-height: 30px; }
        .status { font-weight: bold; padding: 4px; border-radius: 4px; font-size: 0.85em; margin-bottom: 10px; }
        .HIGH { background-color: #e8f5e9; color: #2e7d32; }
        .LOW { background-color: #f0f0f0; color: #666; }
        button { padding: 10px; width: 100%; cursor: pointer; border: none; border-radius: 5px; font-weight: bold; color: white; transition: 0.2s; }
        .btn-toggle { background-color: #ff9800; }
        .btn-click { background-color: #607d8b; }
        .btn-cycle { background-color: #4caf50; margin-top: 10px; }
        .btn-stop { background-color: #f44336; margin-top: 10px; }
        input[type=number] { width: 80%; padding: 8px; margin: 5px 0; border: 1px solid #ccc; border-radius: 4px; font-size: 1.1em; text-align: center; }
        button:active { transform: scale(0.95); }
        button:disabled { background-color: #ccc !important; cursor: not-allowed; }
    </style>
</head>
<body>
    <h2>ESP32 Festo Control</h2>

    <h3 class="section-title">Cycle Control</h3>
    <div class="container">
        <div class="card card-cycle">
            <span class="label">Auto Cycle Count</span>
            <input type="number" id="cycleInput" value="0" min="0">
            <span class="label">Delay per Step (ms)</span>
            <input type="number" id="delayInput" value="1000" min="0">
            <div id="cycleStatus" class="status LOW">Idle</div>
            <button class="btn-cycle" id="cycleBtn" onclick="toggleCycle()">START CYCLE</button>
        </div>
    </div>

    <h3 class="section-title">Control Actions</h3>
    <div class="container" id="action-container"></div>
    <h3 class="section-title">ESP32 -> Festo</h3>
    <div class="container" id="output-container"></div>
    <h3 class="section-title">ESP32 <- Festo</h3>
    <div class="container" id="input-container"></div>

    <script>
        let isRunning = false;

        function triggerAction(id) { fetch(`/action?id=${id}`).then(() => updateStatus()); }

        function toggleCycle() {
            if(isRunning) {
                // STOP Logic: Send count=0 to stop
                fetch(`/startCycle?count=0`).then(() => updateStatus());
            } else {
                // START Logic
                const count = document.getElementById('cycleInput').value;
                const delay = document.getElementById('delayInput').value;
                fetch(`/startCycle?count=${count}&delay=${delay}`).then(() => updateStatus());
            }
        }

        function setInputName(id) {
            const names = { 32: "Enabled", 33: "Motion finished", 34: "Acknowledge start", 35: "Error" };
            return names[id] || "Input " + id;
        }

        function updateStatus() {
            fetch('/status').then(r => r.json()).then(data => {
                const cycleStat = document.getElementById('cycleStatus');
                const cycleBtn = document.getElementById('cycleBtn');
                
                if(data.cycleRemaining > 0) {
                    isRunning = true;
                    cycleStat.innerText = "Remaining: " + data.cycleRemaining;
                    cycleStat.className = "status HIGH";
                    cycleBtn.innerText = "STOP CYCLE";
                    cycleBtn.className = "btn-stop";
                } else {
                    isRunning = false;
                    cycleStat.innerText = "Idle";
                    cycleStat.className = "status LOW";
                    cycleBtn.innerText = "START CYCLE";
                    cycleBtn.className = "btn-cycle";
                }

                document.getElementById('action-container').innerHTML = data.actions.map((a, i) => `
                    <div class="card card-action">
                        <span class="label">${a.label}</span>
                        <div class="status ${a.state}">${a.isToggle ? a.state : 'READY'}</div>
                        <button class="${a.isToggle ? 'btn-toggle' : 'btn-click'}" onclick="triggerAction(${i})" ${a.enabled ? '' : 'disabled'}>
                            ${a.isToggle ? 'TOGGLE' : 'CLICK'}
                        </button>
                    </div>`).join('');

                document.getElementById('output-container').innerHTML = data.outputs.map(o => `
                    <div class="card card-out"><span class="label">${o.label}</span><div class="status ${o.state}">${o.state}</div></div>`).join('');

                document.getElementById('input-container').innerHTML = data.inputs.map(i => `
                    <div class="card card-in"><span class="label">${setInputName(i.pin)}</span><div class="status ${i.state}">${i.state}</div></div>`).join('');
            });
        }
        setInterval(updateStatus, 1000);
        window.onload = updateStatus;
    </script>
</body>
</html>
)rawliteral";

// --- Handlers ---
void handleStartCycle()
{
    if (server.hasArg("count"))
    {
        int count = server.arg("count").toInt();

        // If user sends count 0 or we are already running and they press button, STOP.
        if (count <= 0)
        {
            cycleRemaining = 0;
            server.send(200, "text/plain", "STOPPED");
            Serial.println("Cycle manually stopped.");
            return;
        }

        if (server.hasArg("delay"))
        {
            cycleDelayMs = server.arg("delay").toInt();
        }

        if (count > 0 && m_FestoControl->IsControllerReady())
        {
            cycleRemaining = count;
            currentStep = 0;
            stepInitiated = false;
            server.send(200, "text/plain", "OK");
            Serial.println("===============================");
            Serial.printf("Start %i cycles with %i ms delay\n", count, cycleDelayMs);
            return;
        }
    }
    server.send(400, "text/plain", "Error starting cycle");
}

void handleStatus()
{
    String json = "{\"cycleRemaining\":" + String(cycleRemaining) + ",\"actions\":[";
    for (int i = 0; i < DefinedActions; i++)
    {
        bool enabled = true;
        if (i > 0 && !m_FestoControl->IsControllerReady())
            enabled = false;
        json += "{\"label\":\"" + String(actionLabels[i]) + "\",\"state\":\"" + String(actionStates[i] ? "HIGH" : "LOW") + "\",\"isToggle\":" + String(actionIsToggle[i] ? "true" : "false") + ",\"enabled\":" + String(enabled ? "true" : "false") + "}";
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

void processCycleMachine()
{
    // SAFETY CHECK: If Pin 35 (Error) is HIGH, abort cycle
    // if (digitalRead(35) == HIGH && cycleRemaining > 0)
    // {
    //     cycleRemaining = 0;
    //     Serial.println("!!! SAFETY ABORT: Festo Error Detected (Pin 35) !!!");
    //     return;
    // }

    if (cycleRemaining <= 0)
    {
        currentCycle = 0;
        currentStep = 0;
        return;
    }

    unsigned long now = millis();

    switch (currentStep)
    {
    case 0: // MOVE TO POSITION 1
        if (!stepInitiated)
        {
            currentCycle++;
            m_FestoControl->GoToPosition(1);
            stepInitiated = true;
            stepTimer = now;
            Serial.printf("<%lu> Cycle %i: Moving to Pos 1\n", now, currentCycle);
            currentStep = 1;
            delay(200); // Wait for controller to clear 'Motion Finished' flag
        }
        break;

    case 1: // WAIT FOR REACHED + DELAY
        if (m_FestoControl->IsMotionFinished())
        {
            if (now - stepTimer >= (unsigned long)cycleDelayMs)
            {
                stepInitiated = false;
                stepTimer = now;
                Serial.printf("<%lu> Cycle %i: Pos 1 Dwell finished\n", now, currentCycle);
                currentStep = 2;
            }
        }
        break;

    case 2: // MOVE TO POSITION 2
        if (!stepInitiated)
        {
            m_FestoControl->GoToPosition(2);
            stepInitiated = true;
            stepTimer = now;
            Serial.printf("<%lu> Cycle %i: Moving to Pos 2\n", now, currentCycle);
            currentStep = 3;
            delay(200);
        }
        break;

    case 3: // WAIT FOR REACHED + DELAY
        if (m_FestoControl->IsMotionFinished())
        {
            if (now - stepTimer >= (unsigned long)cycleDelayMs)
            {
                cycleRemaining--;
                currentStep = 0;
                stepInitiated = false;
                Serial.printf("<%lu> Cycle %i: Pos 2 Dwell finished\n", now, currentCycle);
            }
        }
        break;
    }
}

void handleAction()
{
    if (server.hasArg("id"))
    {
        int id = server.arg("id").toInt();
        if (id >= 0 && id < DefinedActions)
        {
            actionStates[id] = !actionStates[id];
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
            if (!actionIsToggle[id])
                actionStates[id] = false;
            server.send(200, "text/plain", "OK");
            return;
        }
    }
    server.send(400, "text/plain", "Invalid");
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
        pinMode(inputPins[i], INPUT_PULLDOWN);
    }

    m_FestoControl = new FestoCmmsControl(4, 12, 5, 13, 14, 15, 16, 17, 18, 32, 33, 34, 35);

    WiFi.begin(ssid, password);
    Serial.print("Connecting...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");

    server.on("/", []()
              { server.send(200, "text/html", INDEX_HTML); });
    server.on("/action", handleAction);
    server.on("/status", handleStatus);
    server.on("/startCycle", handleStartCycle);
    server.begin();
}

void loop()
{
    ControlAliveLed();
    server.handleClient();
    // m_FestoControl->CheckStates();
    processCycleMachine();
}