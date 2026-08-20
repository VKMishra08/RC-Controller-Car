#include <WiFi.h>
#include <WebServer.h>

// --- WiFi Credentials (Access Point Mode) ---
const char* ssid = "Vinay";
const char* password = "vinay1234";

WebServer server(80);

// --- L298N Motor Driver Pins ---
// Left Motor (A)
const int ENA = 14; 
const int IN1 = 27;
const int IN2 = 26;
// Right Motor (B)
const int ENB = 32;
const int IN3 = 25;
const int IN4 = 33;

// --- Global Variables ---
int speedLimit = 255; 
bool isInverted = false; // Tracks if controls are flipped

// --- The Web Interface ---
const char* htmlPage = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>ESP32 Robot Remote</title>
    <style>
        :root {
            --navy: #0b192c;
            --cyan: #00e5ff;
            --green: #39ff14;
            --orange: #ff9900;
            --dark-panel: #142847;
        }
        
        body {
            background-color: var(--navy);
            color: white;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
            touch-action: none;
        }

        h1 { margin-top: 0; color: var(--cyan); text-transform: uppercase; letter-spacing: 2px; }

        #status {
            padding: 10px 20px;
            border-radius: 20px;
            background-color: var(--dark-panel);
            color: #ff4444; 
            font-weight: bold;
            margin-bottom: 20px;
            border: 1px solid #ff4444;
            transition: all 0.3s;
        }

        .joystick-container {
            position: relative;
            background-color: var(--dark-panel);
            border: 3px solid var(--cyan);
            border-radius: 50%;
            box-shadow: 0 0 15px rgba(0, 229, 255, 0.3);
            margin-bottom: 30px;
        }

        canvas { display: block; }

        .controls-panel {
            width: 100%;
            max-width: 400px;
            display: flex;
            flex-direction: column;
            gap: 20px;
        }

        .slider-container label {
            display: flex;
            justify-content: space-between;
            color: var(--cyan);
            margin-bottom: 10px;
            font-weight: bold;
        }

        input[type=range] {
            -webkit-appearance: none;
            width: 100%;
            background: transparent;
        }
        
        input[type=range]::-webkit-slider-runnable-track {
            width: 100%;
            height: 10px;
            background: var(--dark-panel);
            border-radius: 5px;
            border: 1px solid var(--cyan);
        }
        
        input[type=range]::-webkit-slider-thumb {
            -webkit-appearance: none;
            height: 24px;
            width: 24px;
            border-radius: 50%;
            background: var(--green);
            cursor: pointer;
            margin-top: -8px;
            box-shadow: 0 0 10px var(--green);
        }

        .button-group {
            display: flex;
            width: 100%;
        }

        .action-btn {
            flex: 1;
            padding: 15px;
            background-color: transparent;
            color: var(--cyan);
            border: 2px solid var(--cyan);
            border-radius: 10px;
            font-size: 16px;
            font-weight: bold;
            text-transform: uppercase;
            cursor: pointer;
            transition: all 0.2s;
        }

        /* Styling for when controls are inverted */
        .action-btn.active {
            background-color: var(--orange);
            color: var(--navy);
            border-color: var(--orange);
            box-shadow: 0 0 15px var(--orange);
        }
    </style>
</head>
<body>

    <h1>Robot Control</h1>
    <div id="status">● Standby</div>

    <div class="joystick-container">
        <canvas id="joystick" width="250" height="250"></canvas>
    </div>

    <div class="controls-panel">
        <div class="slider-container">
            <label><span>Speed Limit</span> <span id="speed-val">255</span></label>
            <input type="range" id="speed" min="0" max="255" value="255">
        </div>
        
        <div class="button-group">
            <button class="action-btn" id="btn-invert">Invert Controls: OFF</button>
        </div>
    </div>

    <script>
        const statusEl = document.getElementById('status');
        const speedSlider = document.getElementById('speed');
        const speedVal = document.getElementById('speed-val');
        const btnInvert = document.getElementById('btn-invert');
        
        let isInverted = false;

        function sendCommand(endpoint) {
            fetch(endpoint)
                .then(response => {
                    statusEl.innerText = "● Connected";
                    statusEl.style.color = "var(--green)";
                    statusEl.style.borderColor = "var(--green)";
                })
                .catch(error => {
                    statusEl.innerText = "● Disconnected";
                    statusEl.style.color = "#ff4444";
                    statusEl.style.borderColor = "#ff4444";
                });
        }

        // --- Speed Slider ---
        speedSlider.addEventListener('input', (e) => {
            speedVal.innerText = e.target.value;
        });
        speedSlider.addEventListener('change', (e) => {
            sendCommand(`/speed?val=${e.target.value}`);
        });

        // --- Invert Controls Button ---
        function toggleInvert(e) {
            if (e) e.preventDefault();
            isInverted = !isInverted;
            
            if (isInverted) {
                btnInvert.classList.add('active');
                btnInvert.innerText = "Invert Controls: ON";
            } else {
                btnInvert.classList.remove('active');
                btnInvert.innerText = "Invert Controls: OFF";
            }
            
            sendCommand(`/invert?state=${isInverted ? 1 : 0}`);
        }
        
        btnInvert.addEventListener('touchstart', toggleInvert);
        btnInvert.addEventListener('click', toggleInvert);


        // --- Joystick Logic ---
        const canvas = document.getElementById('joystick');
        const ctx = canvas.getContext('2d');
        const radius = canvas.width / 2;
        const maxStickDistance = radius - 30; 
        
        let isDrawing = false;
        let lastSendTime = 0;
        const THROTTLE_MS = 100;

        function drawJoystick(x, y) {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            ctx.beginPath();
            ctx.arc(x, y, 30, 0, Math.PI * 2);
            ctx.fillStyle = "var(--cyan)";
            ctx.fill();
            ctx.shadowBlur = 10;
            ctx.shadowColor = "var(--cyan)";
            ctx.closePath();
        }

        function resetJoystick() {
            drawJoystick(radius, radius);
            sendCommand(`/joy?x=0&y=0`); 
        }
        
        resetJoystick(); 

        function handleMove(clientX, clientY) {
            if (!isDrawing) return;

            const rect = canvas.getBoundingClientRect();
            const mouseX = clientX - rect.left;
            const mouseY = clientY - rect.top;

            const dx = mouseX - radius;
            const dy = mouseY - radius;
            const distance = Math.min(Math.sqrt(dx*dx + dy*dy), maxStickDistance);
            const angle = Math.atan2(dy, dx);

            const knobX = radius + distance * Math.cos(angle);
            const knobY = radius + distance * Math.sin(angle);

            drawJoystick(knobX, knobY);

            const mapX = Math.round((distance * Math.cos(angle) / maxStickDistance) * 100);
            const mapY = Math.round(((distance * Math.sin(angle) / maxStickDistance) * 100) * -1);

            const now = Date.now();
            if (now - lastSendTime > THROTTLE_MS) {
                sendCommand(`/joy?x=${mapX}&y=${mapY}`);
                lastSendTime = now;
            }
        }

        canvas.addEventListener('touchstart', (e) => {
            e.preventDefault();
            isDrawing = true;
            handleMove(e.touches[0].clientX, e.touches[0].clientY);
        });
        
        canvas.addEventListener('touchmove', (e) => {
            e.preventDefault();
            handleMove(e.touches[0].clientX, e.touches[0].clientY);
        });
        
        canvas.addEventListener('touchend', (e) => {
            e.preventDefault();
            isDrawing = false;
            resetJoystick();
        });

        canvas.addEventListener('mousedown', (e) => {
            isDrawing = true;
            handleMove(e.clientX, e.clientY);
        });
        canvas.addEventListener('mousemove', (e) => {
            handleMove(e.clientX, e.clientY);
        });
        canvas.addEventListener('mouseup', () => {
            isDrawing = false;
            resetJoystick();
        });
        canvas.addEventListener('mouseleave', () => {
            if(isDrawing) {
                isDrawing = false;
                resetJoystick();
            }
        });
    </script>
</body>
</html>)rawliteral";

void setup() {
  Serial.begin(115200);

  // Initialize Motor Pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Setup WiFi Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Robot IP Address: ");
  Serial.println(IP); // Default is 192.168.4.1

  // --- Web Server Endpoints ---

  server.on("/", []() {
    server.send(200, "text/html", htmlPage);
  });

  // Handle Joystick Movement
  server.on("/joy", []() {
    if (server.hasArg("x") && server.hasArg("y")) {
      int x = server.arg("x").toInt(); 
      int y = server.arg("y").toInt(); 

      // If Invert Controls is ON, swap the vector directions
      if (isInverted) {
        x = -x;
        y = -y;
      }

      // Calculate differential drive
      int rawLeft = y + x;
      int rawRight = y - x;

      // Clamp values between -100 and 100
      rawLeft = constrain(rawLeft, -100, 100);
      rawRight = constrain(rawRight, -100, 100);

      // Map percentages to PWM (0 to speedLimit)
      int pwmLeft = map(abs(rawLeft), 0, 100, 0, speedLimit);
      int pwmRight = map(abs(rawRight), 0, 100, 0, speedLimit);

      // Set Left Motor
      if (rawLeft > 0) { 
        digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); // Forward
      } else if (rawLeft < 0) { 
        digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); // Backward
      } else { 
        digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);  // Stop
      }
      analogWrite(ENA, pwmLeft);

      // Set Right Motor
      if (rawRight > 0) { 
        digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); // Forward
      } else if (rawRight < 0) { 
        digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); // Backward
      } else { 
        digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);  // Stop
      }
      analogWrite(ENB, pwmRight);

      server.send(200, "text/plain", "OK");
    }
  });

  // Handle Speed Slider
  server.on("/speed", []() {
    if (server.hasArg("val")) {
      speedLimit = server.arg("val").toInt();
      server.send(200, "text/plain", "OK");
    }
  });

  // Handle Control Inversion Toggle
  server.on("/invert", []() {
    if (server.hasArg("state")) {
      isInverted = (server.arg("state").toInt() == 1);
      server.send(200, "text/plain", "Invert State Updated");
    }
  });

  server.begin();
}

void loop() {
  server.handleClient();
}