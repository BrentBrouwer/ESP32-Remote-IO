#include <WiFi.h>
#include <WebServer.h>

// --- Configuration ---
const int LED_ALIVE_PIN = 2;          // Alive LED Pin
const int AliveOnPeriodTime = 100;
const int AliveOffPeriodTime = 2000;
unsigned long AliveLedStatusChanged = 0;

const char *ssid = "Sjaan-24G";
const char *password = "Janrenlen1";

WebServer server(80);

// Define safe GPIOs for Outputs
int outputPins[] = {2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27};
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
        body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f4; }
        .container { display: flex; flex-wrap: wrap; justify-content: center; padding: 20px; }
        .card { background: white; margin: 10px; padding: 20px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); width: 140px; }
        .status { font-weight: bold; margin-bottom: 10px; }
        .HIGH { color: green; }
        .LOW { color: red; }
        button { padding: 10px 20px; font-size: 16px; cursor: pointer; border: none; border-radius: 5px; color: white; transition: 0.3s; }
        .btn-toggle { background-color: #007bff; }
        .btn-toggle:hover { background-color: #0056b3; }
        h2 { color: #333; }
    </style>
</head>
<body>
    <h2>ESP32 Digital IO Control</h2>
    
    <h3>Outputs</h3>
    <div class="container" id="output-container"></div>
    
    <h3>Inputs (Read Only)</h3>
    <div class="container" id="input-container"></div>

    <script>
        function togglePin(pin) {
            fetch(`/toggle?pin=${pin}`)
                .then(response => response.text())
                .then(data => updateStatus());
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
                                <strong>GPIO ${item.pin}</strong>
                                <div class="status ${item.state}">${item.state}</div>
                                <button class="btn-toggle" onclick="togglePin(${item.pin})">Toggle</button>
                            </div>`;
                    });

                    inContainer.innerHTML = '';
                    data.inputs.forEach(item => {
                        inContainer.innerHTML += `
                            <div class="card">
                                <strong>GPIO ${item.pin}</strong>
                                <div class="status ${item.state}">${item.state}</div>
                            </div>`;
                    });
                });
        }

        // Auto-refresh every 2 seconds
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
        int state = digitalRead(pin);
        digitalWrite(pin, !state);
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
        json += "{\"pin\":" + String(outputPins[i]) + ",\"state\":\"" + (digitalRead(outputPins[i]) ? "HIGH" : "LOW") + "\"}";
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

    // Initialize Outputs
    for (int i = 0; i < outputCount; i++)
    {
        pinMode(outputPins[i], OUTPUT);
        digitalWrite(outputPins[i], LOW);
    }

    // Initialize Inputs
    for (int i = 0; i < inputCount; i++)
    {
        pinMode(inputPins[i], INPUT_PULLUP); // Using internal pullup for floating pins
    }

    // Connect WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Define Server Routes
    server.on("/", handleRoot);
    server.on("/toggle", handleToggle);
    server.on("/status", handleStatus);

    server.begin();
}

void loop()
{
    server.handleClient();
}