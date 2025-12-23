#include <WiFi.h>
#include <WebServer.h>

// --- Configuration ---
const int LED_ALIVE_PIN = 2; // Alive LED Pin
const int AliveOnPeriodTime = 100;
const int AliveOffPeriodTime = 2000;
unsigned long AliveLedStatusChanged = 0;

const char *ssid = "Sjaan-24G";
const char *password = "Janrenlen1";

WebServer server(80);

// --- OUTPUT CONFIGURATION ---
// Define safe GPIOs for Outputs
int outputPins[] = {4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27};
// EDIT THESE LABELS: Ensure there are exactly 16 labels to match the 16 pins above
const char *outputLabels[] = {
    "Enable controller", "Disable stop", "Enable motor", "Start positioning",
    "Record bit 0", "Record bit 1", "Output 16", "Output 17",
    "Output 18", "Output 19", "Output 21", "Output 22",
    "Output 23", "Output 25", "Output 26", "Output 27"};

int outputCount = sizeof(outputPins) / sizeof(int);

// Define safe GPIOs for Inputs
int inputPins[] = {32, 33, 34, 35, 36, 39};
int inputCount = sizeof(inputPins) / sizeof(int);

// --- HTML & CSS & JS ---
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 GPIO Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: 'Segoe UI', Arial, sans-serif; text-align: center; background-color: #f4f4f4; color: #333; }
        .container { display: flex; flex-wrap: wrap; justify-content: center; padding: 20px; }
        .card { background: white; margin: 10px; padding: 20px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); width: 150px; border-top: 5px solid #007bff; }
        .card-in { border-top: 5px solid #6c757d; }
        .label-text { font-weight: bold; font-size: 1.1em; display: block; margin-bottom: 5px; }
        .pin-subtext { font-size: 0.8em; color: #888; display: block; margin-bottom: 10px; }
        .status { font-weight: bold; margin-bottom: 15px; padding: 5px; border-radius: 4px; }
        .HIGH { background-color: #d4edda; color: #155724; }
        .LOW { background-color: #f8d7da; color: #721c24; }
        button { padding: 10px 20px; font-size: 14px; cursor: pointer; border: none; border-radius: 5px; color: white; background-color: #007bff; width: 100%; font-weight: bold; }
        button:active { transform: scale(0.98); }
        h2 { margin-top: 30px; }
    </style>
</head>
<body>
    <h2>ESP32 Dashboard</h2>
    
    <h3>Outputs</h3>
    <div class="container" id="output-container"></div>
    
    <h3>Inputs (Read Only)</h3>
    <div class="container" id="input-container"></div>

    <script>
        function togglePin(pin) {
            fetch(`/toggle?pin=${pin}`).then(() => updateStatus());
        }

        function updateStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    const outContainer = document.getElementById('output-container');
                    const inContainer = document.getElementById('input-container');
                    
                    outContainer.innerHTML = '';
                    data.outputs.forEach(item => {
                        outContainer.innerHTML += `
                            <div class="card">
                                <span class="label-text">${item.label}</span>
                                <span class="pin-subtext">GPIO ${item.pin}</span>
                                <div class="status ${item.state}">${item.state}</div>
                                <button onclick="togglePin(${item.pin})">TOGGLE</button>
                            </div>`;
                    });

                    inContainer.innerHTML = '';
                    data.inputs.forEach(item => {
                        inContainer.innerHTML += `
                            <div class="card card-in">
                                <span class="label-text">Input ${item.pin}</span>
                                <span class="pin-subtext">GPIO ${item.pin}</span>
                                <div class="status ${item.state}">${item.state}</div>
                            </div>`;
                    });
                });
        }

        setInterval(updateStatus, 2000);
        window.onload = updateStatus;
    </script>
</body>
</html>
)rawliteral";

// --- Server Handlers ---

void handleRoot()
{
    server.send(200, "text/html", INDEX_HTML);
}

void handleToggle()
{
    if (server.hasArg("pin"))
    {
        int pin = server.arg("pin").toInt();
        digitalWrite(pin, !digitalRead(pin));
        server.send(200, "text/plain", "OK");
    }
    else
    {
        server.send(400, "text/plain", "Missing Pin");
    }
}

void handleStatus()
{
    String json = "{\"outputs\":[";
    for (int i = 0; i < outputCount; i++)
    {
        json += "{";
        json += "\"pin\":" + String(outputPins[i]) + ",";
        json += "\"label\":\"" + String(outputLabels[i]) + "\",";
        json += "\"state\":\"" + String(digitalRead(outputPins[i]) ? "HIGH" : "LOW") + "\"";
        json += "}";
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

    if (on)
    {
        if (now - AliveLedStatusChanged >= AliveOnPeriodTime)
        {
            AliveLedStatusChanged = now;
            digitalWrite(LED_ALIVE_PIN, LOW);
        }
    }
    else
    {
        if (now - AliveLedStatusChanged >= AliveOffPeriodTime)
        {
            AliveLedStatusChanged = now;
            digitalWrite(LED_ALIVE_PIN, HIGH);
        }
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

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/toggle", handleToggle);
    server.on("/status", handleStatus);
    server.begin();
}

void loop()
{
    ControlAliveLed();
    server.handleClient();
}