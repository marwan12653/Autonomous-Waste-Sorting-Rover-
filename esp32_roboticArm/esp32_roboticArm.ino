#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>
#include <iostream>
#include <sstream>
#include <vector>

// --- DIGITAL CAR CONTROL PINS (Trigger) ---
// These pins will send HIGH/LOW signals to the STM32.
const int MOVE_PIN = 21; 
const int STOP_PIN = 19; 

// --- Ultrasonic Sensor Pins (HC-SR04) ---
const int trigPin = 12; 
const int echoPin = 14; 

// --- Obstacle Detection Settings ---
const int STOP_DISTANCE_CM = 20; 
bool isCarMoving = false;
bool wasCarStoppedByLitter = false; // Flag to enforce manual 'STOP' before 'MOVE'

// -----------------------------
// Structs (Servo Definitions)
// -----------------------------
struct ServoPins
{
  Servo servo;
  int servoPin;
  String servoName;
  int initialPosition; 
};


// Your 4-servo configuration
std::vector<ServoPins> servoPins = 
{
  { Servo(), 27 , "Base", 90},
  { Servo(), 26 , "Shoulder", 90},
  { Servo(), 25 , "Elbow", 90},
  { Servo(), 13 , "Gripper", 90},
};

// -----------------------------
// Flags & Time (Record/Play Removed)
// -----------------------------
// Variables related to recording/playback have been removed.

// -----------------------------
// WiFi & Server
// -----------------------------
const char* ssid     = "RobotArm";
const char* password = "12345678"; 

AsyncWebServer server(80);
AsyncWebSocket wsRobotArmInput("/RobotArmInput");

// -----------------------------
// Function Prototypes
// -----------------------------
float getDistanceCm();
void sendCarCommand(const char* command);
void sendCurrentRobotArmState();
void writeServoValues(int servoIndex, int value);
// playRecordedRobotArmSteps() removed

// -----------------------------
// HTML PAGE (Web Interface)
// -----------------------------
const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>

    input[type=button]
    {
      background-color:red;color:white;border-radius:30px;width:100%;height:40px;font-size:20px;text-align:center;
    }
        
    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
          -khtml-user-select: none; /* Konqueror HTML */
            -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                     supported by Chrome and Opera */
    }

    .slidecontainer {
      width: 100%;
    }

    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 20px;
      border-radius: 5px;
      background: #d3d3d3;
      outline: none;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
    }

    .slider:hover {
      opacity: 1;
    }
  
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

    .slider::-moz-range-thumb {
      width: 40px;
      height: 40px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

    </style>
  
  </head>
  <body class="noselect" align="center" style="background-color:white">
      
    <h2 style="color: teal;text-align:center;">Robot Arm Controller</h2>
    
    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr/><tr/>
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Gripper:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="180" value="0" class="slider" id="Gripper" oninput='sendButtonInput("Gripper",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/>
      <tr>
        <td style="text-align:left;font-size:25px"><b>Elbow:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Elbow" oninput='sendButtonInput("Elbow",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Shoulder:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Shoulder" oninput='sendButtonInput("Shoulder",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/>      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Base:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="180" value="90" class="slider" id="Base" oninput='sendButtonInput("Base",value)'>
          </div>
        </td>
      </tr> 
      <tr/><tr/> 
      
      <tr>
        <td style="text-align:left;font-size:25px"><b>Car Control:</b></td>
        <td><input type="button" id="CarMove" value="MOVE" ontouchend='onclickButton(this)'></td>
        <td><input type="button" id="CarStop" value="STOP" ontouchend='onclickButton(this)'></td>
      </tr>    
    </table>
  
    <script>
      var webSocketRobotArmInputUrl = "ws:\/\/" + window.location.hostname + "/RobotArmInput";      
      var websocketRobotArmInput;
      
      function initRobotArmInputWebSocket() 
      {
        websocketRobotArmInput = new WebSocket(webSocketRobotArmInputUrl);
        websocketRobotArmInput.onopen     = function(event){};
        websocketRobotArmInput.onclose  = function(event){setTimeout(initRobotArmInputWebSocket, 2000);};
        websocketRobotArmInput.onmessage    = function(event)
        {
          // Only receives CarMove/CarStop state updates (not currently implemented in ESP32)
          // or initial servo positions (which we don't need to update the client)
        };
      }
      
      function sendButtonInput(key, value) 
      {
        var data = key + "," + value;
        websocketRobotArmInput.send(data);
      }
      
      function onclickButton(button) 
      {
        // --- CAR CONTROL LOGIC ---
        if (button.id == "CarMove") 
        {
          sendButtonInput("CarCommand", "m"); // Send 'm' for move
          button.style.backgroundColor = "green"; 
          document.getElementById("CarStop").style.backgroundColor = "red";
          return;
        }
        
        if (button.id == "CarStop")
        {
          sendButtonInput("CarCommand", "s"); // Send 's' for stop
          button.style.backgroundColor = "red";
          document.getElementById("CarMove").style.backgroundColor = "red";
          return;
        }

        // --- Only Car buttons remain in the HTML, this is fallback for any future buttons ---
        button.value = (button.value == "ON") ? "OFF" : "ON" ;         
        button.style.backgroundColor = (button.value == "ON" ? "green" : "red");         
        var value = (button.value == "ON") ? 1 : 0 ;
        sendButtonInput(button.id, value);
      }
      
      // Removed enableDisableButtonsSliders function as it only handled Record/Play interaction
      
      window.onload = initRobotArmInputWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
      });      
    </script>
  </body>     
</html>
)HTMLHOMEPAGE";

// -----------------------------
// Ultrasonic Sensor Function (HC-SR04)
// -----------------------------
float getDistanceCm() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the duration with a 30ms timeout
  long duration = pulseIn(echoPin, HIGH, 30000); 

  if (duration == 0) return 999.0; 

  // Calculate distance: (duration * speed_of_sound) / 2
  float distance = duration * 0.0343 / 2; 

  return distance;
}

// -----------------------------
// Send Car Command (Digital Trigger)
// -----------------------------
void sendCarCommand(const char* command) {
  if (strcmp(command, "m") == 0) {
    isCarMoving = true;
    wasCarStoppedByLitter = false;
    
    digitalWrite(MOVE_PIN, HIGH);
    digitalWrite(STOP_PIN, LOW);
    
    Serial.println("[Car] Sending MOVE (Pin " + String(MOVE_PIN) + " HIGH)");
    
  } else if (strcmp(command, "s") == 0) {
    isCarMoving = false;
    
    digitalWrite(MOVE_PIN, LOW);
    digitalWrite(STOP_PIN, HIGH);
    
    Serial.println("[Car] Sending STOP (Pin " + String(STOP_PIN) + " HIGH)");
    
  } else {
    // Failsafe
    digitalWrite(MOVE_PIN, LOW);
    digitalWrite(STOP_PIN, LOW);
  }
  
  delay(10); 
}

// -----------------------------
// HTTP handlers 
// -----------------------------
void handleRoot(AsyncWebServerRequest *request) 
{
  Serial.println("[HTTP] Serving /");
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) 
{
    Serial.println("[HTTP] 404 Not Found");
    request->send(404, "text/plain", "File Not Found");
}

// -----------------------------
// WebSocket event handler 
// -----------------------------
void onRobotArmInputWebSocketEvent(AsyncWebSocket *server, 
             AsyncWebSocketClient *client, 
             AwsEventType type,
             void *arg, 
             uint8_t *data, 
             size_t len) 
{ 
  switch (type) 
  {
    case WS_EVT_CONNECT:
      Serial.printf("[WS] Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      sendCurrentRobotArmState();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("[WS] Client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      {
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
      {
        std::string myData = "";
        myData.assign((char *)data, len);
        std::istringstream ss(myData);
        std::string key, value;
        std::getline(ss, key, ',');
        std::getline(ss, value, ',');
        Serial.printf("[WS] Key [%s] Value[%s]\n", key.c_str(), value.c_str()); 
        int valueInt = atoi(value.c_str()); 
        
        // --- Simplified command handling (no Record/Play) ---
        if (key == "CarCommand")
        {
          if (value == "m" && wasCarStoppedByLitter) {
            Serial.println("[Car] Cannot move: Safety stop active. Must hit STOP first to reset.");
          } else {
            sendCarCommand(value.c_str());
          }
        }
        else if (key == "Base")
        {
          writeServoValues(0, valueInt);            
        } 
        else if (key == "Shoulder")
        {
          writeServoValues(1, valueInt);            
        } 
        else if (key == "Elbow")
        {
          writeServoValues(2, valueInt);            
        }         
        else if (key == "Gripper")
        {
          writeServoValues(3, valueInt);        
        }   
        else
        {
          Serial.printf("[WS] Unknown key: %s\n", key.c_str());
        }
      }
      break;
      }
    case WS_EVT_PONG:
      Serial.println("[WS] PONG");
      break;
    case WS_EVT_ERROR:
      Serial.println("[WS] ERROR");
      break;
    default:
      break;  
  }
}

// -----------------------------
// send current state (to clients)
// -----------------------------
void sendCurrentRobotArmState()
{
  Serial.println("[State] Broadcasting current servo positions...");
  for (int i = 0; i < servoPins.size(); i++)
  {
    int pos = servoPins[i].servo.read();
    Serial.printf("[State] %s -> %d\n", servoPins[i].servoName.c_str(), pos);
    wsRobotArmInput.textAll(servoPins[i].servoName + "," + pos);
  }
  // Record/Play state removed
}

// -----------------------------
// write servo values (no recording)
// -----------------------------
void writeServoValues(int servoIndex, int value)
{
  if (servoIndex < 0 || servoIndex >= servoPins.size())
  {
    Serial.printf("[Error] Invalid servo index: %d\n", servoIndex);
    return;
  }

  // No recording logic needed here
  
  servoPins[servoIndex].servo.write(value);
  Serial.printf("[Servo] Written -> %s (pin %d) = %d\n", servoPins[servoIndex].servoName.c_str(), servoPins[servoIndex].servoPin, value);
  // broadcast new position
  wsRobotArmInput.textAll(servoPins[servoIndex].servoName + "," + String(value));
}

// -----------------------------
// setup pins and attach servos (no playback function)
// -----------------------------
void setUpPinModes()
{
  for (int i = 0; i < servoPins.size(); i++)
  {
    // Use min/max pulse width if necessary for servo stability (usually not needed for standard servos)
    servoPins[i].servo.attach(servoPins[i].servoPin);
    servoPins[i].servo.write(servoPins[i].initialPosition);    
    Serial.printf("[Setup] Attached servo %d (%s) on pin %d -> initial %d\n", i, servoPins[i].servoName.c_str(), servoPins[i].servoPin, servoPins[i].initialPosition);
  }
  // Ultrasonic pin setup
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Digital Car Control Pins setup
  pinMode(MOVE_PIN, OUTPUT);
  pinMode(STOP_PIN, OUTPUT);
  
  // Ensure the car is stopped at boot 
  digitalWrite(MOVE_PIN, LOW);
  digitalWrite(STOP_PIN, HIGH);
  Serial.printf("[Setup] Car control pins set: MOVE_PIN (%d), STOP_PIN (%d)\n", MOVE_PIN, STOP_PIN);
}

// -----------------------------
// setup (Initialize everything)
// -----------------------------
void setup(void) 
{
  Serial.begin(115200);
  delay(50);
  Serial.println("[BOOT] Starting Robot Arm/Litter Picker ESP32");

  setUpPinModes();

  // Create Open Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("[WiFi] AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
      
  wsRobotArmInput.onEvent(onRobotArmInputWebSocketEvent);
  server.addHandler(&wsRobotArmInput);

  server.begin();
  Serial.println("[HTTP] server started");
}

// -----------------------------
// loop (Only checks ultrasonic and web connection)
// -----------------------------
void loop() 
{
  wsRobotArmInput.cleanupClients();
  
  // Ultrasonic Safety Check (constantly runs)
  if (isCarMoving && !wasCarStoppedByLitter) {
    float distance = getDistanceCm();
    
    // Check if distance is within the stop range but not an error reading (> 1.0)
    if (distance <= STOP_DISTANCE_CM && distance > 1.0) { 
      Serial.printf("!!! LITTER DETECTED at %.2f cm - EMERGENCY STOP !!!\n", distance);
      sendCarCommand("s"); // Stop the car via digital trigger
      wasCarStoppedByLitter = true; // Set the safety flag
    }
  }

  // Small delay for loop stability
  delay(50); 
}