#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESP32Servo.h>

// -------------------- ACCESS POINT --------------------
const char* ssid     = "ESP32-RC";
const char* password = "12345678";

// -------------------- WEB SERVER & WEBSOCKET --------------------
WebServer server(80);
WebSocketsServer webSocket(81);

// -------------------- L298N MOTOR PINS --------------------
// Motor A
const int ENA = 25;   // PWM speed
const int IN1 = 26;   // Direction
const int IN2 = 27;   // Direction

// Motor B
const int ENB = 14;   // PWM speed
const int IN3 = 32;   // Direction
const int IN4 = 33;   // Direction

// -------------------- SERVO --------------------
Servo steering;
const int servoPin = 18;

// -------------------- CONTROL VARIABLES --------------------
int throttleValue = 0;
int steeringValue = 90;

// -------------------- MOTOR CONTROL --------------------
void driveMotors(int speed) {
  speed = constrain(speed, -255, 255);

  Serial.print("Motor command: ");
  Serial.println(speed);

  int pwmVal = abs(speed);

  if (speed > 0) {
    // Forward
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);

    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);

    ledcWrite(ENA, pwmVal);
    ledcWrite(ENB, pwmVal);
  } 
  else if (speed < 0) {
    // Reverse
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);

    ledcWrite(ENA, pwmVal);
    ledcWrite(ENB, pwmVal);
  } 
  else {
    // Stop
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);

    ledcWrite(ENA, 0);
    ledcWrite(ENB, 0);
  }
}

// -------------------- WEBSOCKET EVENT --------------------
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_TEXT) {
    String msg = String((char*)payload);

    Serial.print("WS received: ");
    Serial.println(msg);

    if (msg.startsWith("T:")) {
      throttleValue = constrain(msg.substring(2).toInt(), -255, 255);
      driveMotors(throttleValue);
    } 
    else if (msg.startsWith("S:")) {
      steeringValue = constrain(msg.substring(2).toInt(), 0, 180);
      steering.write(steeringValue);

      Serial.print("Steer command: ");
      Serial.println(steeringValue);
    }
  } 
  else if (type == WStype_DISCONNECTED) {
    Serial.println("Client disconnected - stopping motors");
    driveMotors(0);
  }
}

// -------------------- WEBPAGE HTML --------------------
String webpage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 RC Car</title>
<style>
  body {
    font-family: Arial, sans-serif;
    text-align: center;
    background: #111;
    color: #fff;
    padding: 20px;
  }

  h1 {
    color: #0f0;
  }

  .label {
    font-size: 1.1em;
    margin: 24px 0 8px;
  }

  #status {
    font-size: 0.8em;
    color: #f80;
    margin-bottom: 16px;
  }

  input[type=range] {
    -webkit-appearance: none;
    appearance: none;
    width: 75%;
    height: 28px;
    background: #333;
    border-radius: 14px;
    outline: none;
    cursor: pointer;
  }

  input[type=range]::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 44px;
    height: 44px;
    border-radius: 50%;
    background: #0f0;
    cursor: pointer;
    box-shadow: 0 0 6px #0f0;
  }

  input[type=range]::-moz-range-thumb {
    width: 44px;
    height: 44px;
    border-radius: 50%;
    background: #0f0;
    cursor: pointer;
    box-shadow: 0 0 6px #0f0;
    border: none;
  }

  #throttleVal, #steerVal {
    font-size: 1.3em;
    font-weight: bold;
    margin-top: 6px;
    color: #0f0;
  }
</style>
</head>
<body>

<h1>ESP32 RC Car</h1>
<div id="status">Connecting...</div>

<div class="label">Steering Servo</div>
<input type="range" min="0" max="180" value="90" id="steerSlider"
  oninput="sendSteer(this.value)">
<div id="steerVal">90°</div>

<div class="label">Throttle Motors</div>
<input type="range" min="-255" max="255" value="0" id="throttleSlider"
  oninput="sendThrottle(this.value)"
  onpointerup="resetThrottle()"
  ontouchend="resetThrottle()">
<div id="throttleVal">0</div>

<script>
  var ws;

  function connect() {
    ws = new WebSocket('ws://' + location.hostname + ':81/');

    ws.onopen = function() {
      document.getElementById('status').textContent = 'Connected';
      document.getElementById('status').style.color = '#0f0';
      ws.send('T:0');
    };

    ws.onclose = function() {
      document.getElementById('status').textContent = 'Disconnected - reconnecting...';
      document.getElementById('status').style.color = '#f80';
      document.getElementById('throttleSlider').value = 0;
      document.getElementById('throttleVal').textContent = '0';
      setTimeout(connect, 2000);
    };

    ws.onerror = function() {
      ws.close();
    };
  }

  function sendSteer(v) {
    document.getElementById('steerVal').textContent = v + '°';

    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send('S:' + v);
    }
  }

  function sendThrottle(v) {
    document.getElementById('throttleVal').textContent = v;

    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send('T:' + v);
    }
  }

  function resetThrottle() {
    document.getElementById('throttleSlider').value = 0;
    sendThrottle(0);
  }

  connect();
</script>

</body>
</html>
)rawliteral";

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(115200);

  // Motor direction pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Start motors stopped
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  // ESP32 core 3.x PWM setup
  ledcAttach(ENA, 20000, 8);
  ledcAttach(ENB, 20000, 8);

  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);

  // Servo setup
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  steering.setPeriodHertz(50);
  steering.attach(servoPin, 500, 2400);
  steering.write(90);

  // WiFi Access Point
  WiFi.softAP(ssid, password);

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Web server
  server.on("/", []() {
    server.send(200, "text/html", webpage);
  });

  server.begin();

  // WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("Ready.");
}

// -------------------- LOOP --------------------
void loop() {
  server.handleClient();
  webSocket.loop();
}
