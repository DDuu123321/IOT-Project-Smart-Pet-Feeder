#define BLYNK_TEMPLATE_ID "TMPL6goH2FXrE"
#define BLYNK_TEMPLATE_NAME "IoT project"
#define BLYNK_AUTH_TOKEN "xxxxxxxxxxxxxxxxxxxxx"// ADD YOUR_BLYNK_AUTH_TOKEN_HERE
#define BLYNK_DEVICE_NAME "PetFeeder"
#define BLYNK_PRINT Serial  // Add this line to output Blynk debug information

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <HX711.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <UniversalTelegramBot.h>


// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const char* botToken = "YOUR_TELEGRAM_BOT_TOKEN";



// Use api.telegram.org with port 443
WiFiClientSecure secured_client;
UniversalTelegramBot bot(botToken, secured_client);

// ID of your chat or user (you can set this after first message if unknown)
String chat_id = "YOUR_TELEGRAM_CHAT_ID";

// Time to check Telegram messages (in ms)
unsigned long lastTime = 0;
unsigned long interval = 2000;

// Hardware pin definitions
const int SERVO_PIN = 5;       // Servo pin
const int HX711_DOUT_PIN = 4;  // HX710 weight sensor data pin
const int HX711_SCK_PIN = 7;   // HX710 weight sensor clock pin
const int LED_PIN = 2;         // LED indicator pin

// Create objects
Servo gateServo;  // Feed wheel servo
HX711 scale;      // Weight sensor
WidgetRTC rtc;    // Blynk real-time clock

// Global variables
float calibration_factor = -2229.0;  // Calibration factor, adjust based on actual setup
float currentWeight = 0.0;           // Current weight
float targetFeedAmount = 30.0;       // Default feeding amount 30g
bool feedingInProgress = false;      // Feeding in progress flag
int scheduledFeedings[7] = { 0 };    // Array to store feeding times (max 7 scheduled feedings)
int numScheduledFeedings = 0;        // Number of scheduled feedings
bool isOpen = false;

// Wheel servo position control
int servoPositions[] = { 0, 90, 180, 270 };  // Four preset positions
int currentPositionIndex = 0;             // Current position index

// Variables for food remaining feature
float containerCapacity = 1000.0;    // Food container total capacity (g)
float containerInitialWeight = 0.0;  // Empty bowl weight
float totalFedAmount = 0.0;          // Total amount of food dispensed

// Variables for analytics charts
unsigned long lastWeightLogTime = 0;   // Last time weight was recorded
unsigned long lastDailyStatsTime = 0;  // Last time daily stats were updated
int feedCount = 0;                     // Today's feeding count
float dailyFeedTotal = 0.0;            // Today's total feeding amount
int previousDay = 0;                   // For detecting date change
float lastRecordedWeight = 0.0;        // Last recorded weight

// Feeding history
#define MAX_FEED_HISTORY 10
float feedHistory[MAX_FEED_HISTORY];        // Feeding amount history array
unsigned long feedTimes[MAX_FEED_HISTORY];  // Feeding time history array
int feedHistoryIndex = 0;                   // Current history index

// New variables for Blynk connection status checking
unsigned long lastConnectionCheck = 0;
bool wasConnected = false;

// Function declarations (keep the original function positions unchanged to ensure compilation success)
void blinkLED(int times, int delayMs);
void checkDateChange();
void updateFoodRemaining();
void updateFeedHistory(float amount);
void feedPet(float amount);
void checkScheduledFeedings();
void sendStatusMsg(const String& msg);

// Function to blink LED for visual feedback
void blinkLED(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}

// Function to send heartbeat data to ensure connection is active
void sendHeartbeat() {
  Blynk.virtualWrite(V4, "Heartbeat: " + String(millis() / 1000) + "s");
  // Periodically send data points to charts to keep them active
  Blynk.virtualWrite(V9, currentWeight);
  // Optionally send zero value data points for Feed History
  static unsigned long lastFeedHeartbeat = 0;
  if (millis() - lastFeedHeartbeat > 300000) {  // Every 5 minutes
    Blynk.virtualWrite(V10, 0);
    lastFeedHeartbeat = millis();
  }
}

// New function: Force update all chart data
void updateAllCharts() {
  Serial.println("Force updating all chart data...");

  // Update weight chart
  Blynk.virtualWrite(V9, currentWeight);
  Serial.println("Sent weight data to V9: " + String(currentWeight) + "g");

  // Update feeding chart (send last feeding amount or 0)
  float lastFeedAmount = (feedHistoryIndex > 0) ? feedHistory[0] : 0;
  Blynk.virtualWrite(V10, lastFeedAmount);
  Serial.println("Sent feeding data to V10: " + String(lastFeedAmount) + "g");

  // Update daily feeding total
  Blynk.virtualWrite(V11, dailyFeedTotal);
  Serial.println("Sent daily feeding total to V11: " + String(dailyFeedTotal) + "g");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Smart Pet Feeder starting...");

  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize servo
  Serial.print("Initializing servo...");
  gateServo.attach(SERVO_PIN);
  gateServo.write(servoPositions[currentPositionIndex]);  // Set initial position
  delay(500);
  Serial.println("Done");

  // Initialize weight sensor
  Serial.print("Initializing weight sensor...");
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();                // Tare the scale
  containerInitialWeight = 0;  // Set initial container weight to 0
  Serial.println("Done");

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Connection FAILED!");
    // Continue running, but will retry connection in loop
  }

  // Connect to Blynk with timeout
  Serial.print("Connecting to Blynk...");
  Blynk.config(BLYNK_AUTH_TOKEN);              // Configure first without connecting
  bool blynkConnected = Blynk.connect(10000);  // 10 second timeout

  if (blynkConnected) {
    Serial.println("Connected!");
  } else {
    Serial.println("Connection FAILED! Will retry in loop...");
  }

  // Initialize RTC
  rtc.begin();
  setSyncInterval(10 * 60);  // Sync interval in seconds (10 minutes)

  // Record initial weight
  currentWeight = scale.get_units(5);
  if (currentWeight < 0) currentWeight = 0;
  lastRecordedWeight = currentWeight;

  // Set initial date
  previousDay = day();

  // Startup completion indication
  blinkLED(3, 200);  // Blink 3 times with 200ms delay

  // Initialize charts
  sendStatusMsg("System started, initializing charts...");

  // Clear feeding history
  for (int i = 0; i < MAX_FEED_HISTORY; i++) {
    feedHistory[i] = 0;
    feedTimes[i] = 0;
  }

  // Force send initial data to all charts
  delay(2000);  // Give enough time for connection

  // Important! Send initial values to Blynk interface
  if (Blynk.connected()) {
    Serial.println("Sending initial data to Blynk...");

    // Send current weight to weight display
    Blynk.virtualWrite(V2, currentWeight);

    // Send food remaining percentage
    Blynk.virtualWrite(V7, 100);

    // Send initial data to charts
    Blynk.virtualWrite(V9, currentWeight);
    Blynk.virtualWrite(V10, 0);
    Blynk.virtualWrite(V11, 0);

    // Ensure sending success
    delay(500);

    // Sync all Blynk component states
    Blynk.syncAll();
    Serial.println("Initial data sent, components synchronized");
  } else {
    Serial.println("Blynk not connected, will retry in loop");
  }

  secured_client.setInsecure();


  Serial.println("Initialization complete!");
}

void loop() {
  // 1. WiFi reconnection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost, attempting to reconnect...");
    WiFi.begin(ssid, password);
    delay(5000);
  }

  // 2. Blynk reconnection
  if (!Blynk.connected()) {
    if (wasConnected) {
      Serial.println("Blynk connection lost, attempting to reconnect...");
      wasConnected = false;
    }

    if (WiFi.status() == WL_CONNECTED) {
      bool reconnected = Blynk.connect(5000);  // 5s timeout
      if (reconnected) {
        Serial.println("Reconnected to Blynk successfully!");
        wasConnected = true;
        updateAllCharts();
      }
    }
  } else {
    wasConnected = true;
    Blynk.run();

    // 3. Heartbeat every 30s
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 30000) {
      sendHeartbeat();
      lastHeartbeat = millis();
    }
  }

  // 4. Update weight every 1s
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    currentWeight = scale.get_units(5);
    if (currentWeight < 0) currentWeight = 0;

    if (Blynk.connected()) {
      Blynk.virtualWrite(V2, currentWeight);
    }

    Serial.print("Current weight: ");
    Serial.print(currentWeight, 1);
    Serial.println(" g");

    // Log weight every 30s
    static unsigned long lastWeightLogTime = 0;
    if (millis() - lastWeightLogTime > 30000) {
      if (Blynk.connected()) {
        Blynk.virtualWrite(V9, currentWeight);
        Serial.println("Weight history recorded: " + String(currentWeight) + "g");
      }
      lastWeightLogTime = millis();
    }

    updateFoodRemaining();
    checkDateChange();

    if (Blynk.connected()) {
      sendStatusMsg("active");
    }

    lastUpdate = millis();
  }

  // 5. Poll Telegram messages every `interval`
  static unsigned long lastTelegramCheck = 0;
  const unsigned long interval = 3000; // Check every 3s

  if (millis() - lastTelegramCheck > interval) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("Got response");

      for (int i = 0; i < numNewMessages; i++) {
        String text = bot.messages[i].text;
        chat_id = bot.messages[i].chat_id;

        Serial.println("Received voice command (text): " + text);

        if (text == "feed") {
          gateServo.write(45);  // Adjust angle as needed
          Serial.println(">> Feed command received!");
        } else if (text == "open") {
          gateServo.write(90);  // Gate open position
          Serial.println(">> Open command received!");
        } else if (text == "close") {
          gateServo.write(180); // Gate closed position
          Serial.println(">> Close command received!");
        } 
      }

      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    lastTelegramCheck = millis();
  }
}


// Check for date change and reset daily stats
void checkDateChange() {
  int currentDay = day();

  // If day has changed
  if (currentDay != previousDay) {
    // Record previous day's total
    String statsMsg = "Date changed, previous day feeds: " + String(feedCount) + " times, total " + String(dailyFeedTotal) + "g";
    Serial.println(statsMsg);

    // Reset statistics
    feedCount = 0;
    dailyFeedTotal = 0.0;
    previousDay = currentDay;

    // Send reset data
    if (Blynk.connected()) {
      Blynk.virtualWrite(V11, 0);
    }
  }
}

// Calculate and update food remaining
void updateFoodRemaining() {
  // Calculate food remaining in the container
  float foodRemaining = containerCapacity - totalFedAmount;
  if (foodRemaining < 0) foodRemaining = 0;

  // Calculate remaining percentage
  int percentRemaining = (int)((foodRemaining / containerCapacity) * 100);
  if (percentRemaining > 100) percentRemaining = 100;

  // Low food warning logic
  static bool lowFoodWarningTriggered = false;
  if (percentRemaining < 20 && !lowFoodWarningTriggered) {
    sendStatusMsg("WARNING: Food level below 20%!");
    lowFoodWarningTriggered = true;
  } else if (percentRemaining > 30) {
    lowFoodWarningTriggered = false;
  }

  // Send to virtual pin V7 display
  if (Blynk.connected()) {
    Blynk.virtualWrite(V7, percentRemaining);
  }

  // Debug info
  Serial.print("Food remaining: ");
  Serial.print(foodRemaining, 1);
  Serial.print("g (");
  Serial.print(percentRemaining);
  Serial.println("%)");
}

// Update feeding history
void updateFeedHistory(float amount) {
  // Update total fed amount
  totalFedAmount += amount;
  if (totalFedAmount > containerCapacity) totalFedAmount = containerCapacity;

  // Shift history to make room for new entry
  for (int i = MAX_FEED_HISTORY - 1; i > 0; i--) {
    feedHistory[i] = feedHistory[i - 1];
    feedTimes[i] = feedTimes[i - 1];
  }

  // Add new record
  feedHistory[0] = amount;
  feedTimes[0] = now();  // Current timestamp

  // Update daily statistics
  feedCount++;
  dailyFeedTotal += amount;

  // Send to charts
  if (Blynk.connected()) {
    // Send the same data multiple times to ensure recording
    for (int i = 0; i < 3; i++) {
      Blynk.virtualWrite(V10, amount);  // Send to feed history chart
      delay(100);
    }
    Blynk.virtualWrite(V11, dailyFeedTotal);  // Send to daily feed total chart

    // Additional debug information
    Serial.println("Feeding data sent to V10: " + String(amount) + "g");
    Serial.println("Daily feeding total sent to V11: " + String(dailyFeedTotal) + "g");
  } else {
    Serial.println("Blynk not connected, cannot update feeding history chart");
  }

  // Update food remaining display
  updateFoodRemaining();

  // Debug info
  Serial.println("Feed history updated: " + String(amount) + "g");
  Serial.println("Today's total feed amount: " + String(dailyFeedTotal) + "g");
}

// Feeding function - For FS5115M standard servo wheel-style feeder - Modified feeding logic
void feedPet(float amount) {
  if (feedingInProgress) return;

  feedingInProgress = true;
  Serial.println("Starting feeding...");
  sendStatusMsg("Starting feeding...");

  float initialWeight = scale.get_units(10);
  Serial.print("Initial weight: ");
  Serial.print(initialWeight, 1);
  Serial.println(" g");
  sendStatusMsg("Initial weight: " + String(initialWeight, 1) + " g");

  digitalWrite(LED_PIN, HIGH);  // Turn on LED indicator

  // Feeding loop
  unsigned long startTime = millis();
  bool timeoutOccurred = false;
  int rotationCount = 0;  // Count rotations

  // Ensure initial position is at 0 degrees
  gateServo.write(0);
  currentPositionIndex = 0;
  delay(500);  // Give time for the motor to reach initial position

  while (millis() - startTime < 15000 && isOpen) {
    Blynk.run();  // Keep Blynk responsive

    currentPositionIndex = (currentPositionIndex == 0) ? 1 : 0;
    int nextPosition = servoPositions[currentPositionIndex];

    gateServo.write(nextPosition);
    rotationCount++;

    sendStatusMsg("Rotating feeder: " + String(rotationCount) + " times, position: " + String(nextPosition) + "°");

    delay(1000);  // Wait for servo

    float currentWeight = scale.get_units(5);
    float dispensedAmount = currentWeight - initialWeight;

    Serial.print("Currently dispensed: ");
    Serial.print(dispensedAmount, 1);
    Serial.println(" g");

    if (dispensedAmount >= amount * 0.9) {
      Serial.println("Target weight reached, staying at current position: " + String(nextPosition) + "°");
      sendStatusMsg("Target reached, stopping at " + String(nextPosition) + "°");
      break;
    }

    if (rotationCount >= 16) {
      timeoutOccurred = true;
      sendStatusMsg("WARNING: Insufficient food after multiple rotations");
      break;
    }

    if (!isOpen) break;
  }

  // Timeout check - Note: no longer returning the motor to 0 degrees
  if (millis() - startTime >= 15000) {
    timeoutOccurred = true;
    sendStatusMsg("WARNING: Feeding timeout occurred!");
  }

  digitalWrite(LED_PIN, LOW);  // Turn off LED indicator

  // Calculate actual feeding amount
  delay(500);
  float finalWeight = scale.get_units(10);
  float actualAmount = finalWeight - initialWeight;

  String statusMsg;
  if (timeoutOccurred) {
    statusMsg = "Feeding incomplete: " + String(actualAmount, 1) + "/" + String(amount, 1) + " g";
  } else {
    statusMsg = "Fed: " + String(actualAmount, 1) + " g";
  }

  sendStatusMsg(statusMsg);
  Serial.print("Actual feed amount: ");
  Serial.print(actualAmount, 1);
  Serial.println(" g");

  // Update feeding history and statistics
  if (actualAmount > 0) {
    updateFeedHistory(actualAmount);
  }

  feedingInProgress = false;

  // Automatically set Blynk button status back to OFF
  if (Blynk.connected()) {
    Blynk.virtualWrite(V1, 0);
  }
}

void checkScheduledFeedings() {
  static int lastFedTime = -1;

  if (numScheduledFeedings == 0) return;

  // Get current time
  int currentHour = hour();
  int currentMinute = minute();
  int currentTimeInMinutes = currentHour * 60 + currentMinute;

  for (int i = 0; i < numScheduledFeedings; i++) {
    int scheduledTime = scheduledFeedings[i];

    if (currentTimeInMinutes == scheduledTime && lastFedTime != currentTimeInMinutes) {
      Serial.println("Scheduled feeding time reached!");
      sendStatusMsg("Auto feeding triggered");
      feedPet(targetFeedAmount);
      lastFedTime = currentTimeInMinutes;
    }
  }
}

// V1: Manual feeding
BLYNK_WRITE(V1) {
  int buttonState = param.asInt();
  isOpen = (buttonState == 1);

  if (isOpen == false) {
    sendStatusMsg("Stop Manual feeding");
    gateServo.write(0);  // Stop the servo
  }
  else if (isOpen == true) {
    sendStatusMsg("Manual feeding triggered");
    if (isOpen == true){
      feedPet(targetFeedAmount);
    }
    // feedPet will set the button OFF automatically
  } 
}

// V3: Set feeding amount
BLYNK_WRITE(V3) {
  targetFeedAmount = param.asFloat();
  Serial.print("Feeding amount set to: ");
  Serial.print(targetFeedAmount);
  Serial.println(" g");

  String statusMsg = "Feed amount: " + String(targetFeedAmount) + " g";
  sendStatusMsg(statusMsg);
}

// Send status message
void sendStatusMsg(const String& msg) {
  if (Blynk.connected()) {
    Blynk.virtualWrite(V4, msg);  // Send to Blynk app (e.g., status label)
  }
  Serial.println(msg);  // Also print to Serial monitor
}

// V5: Calibration
BLYNK_WRITE(V5) {
  if (param.asInt() == 1) {
    // Perform tare operation
    scale.tare();

    // Set container weight to 0
    containerInitialWeight = 0;

    Serial.println("Weight sensor calibrated");
    sendStatusMsg("Weight sensor calibrated");

    // Blink LED to indicate completion - now using the common function
    blinkLED(3, 100);

    // Read current weight and send to display
    currentWeight = scale.get_units(5);
    if (currentWeight < 0) currentWeight = 0;

    if (Blynk.connected()) {
      Blynk.virtualWrite(V2, currentWeight);
    }

    // Update food remaining display
    updateFoodRemaining();
  }
}

// V6: Set scheduled feeding time using Menu component
BLYNK_WRITE(V6) {
  int timeInMinutes = param.asInt();
  numScheduledFeedings = 0;

  if (timeInMinutes > 0) {
    int feedHour = timeInMinutes / 60;
    int feedMinute = timeInMinutes % 60;

    scheduledFeedings[0] = timeInMinutes;
    numScheduledFeedings = 1;

    char buf[10];
    sprintf(buf, "%02d:%02d", feedHour, feedMinute);
    String statusMsg = "Schedule set: " + String(buf);

    sendStatusMsg(statusMsg);
    Serial.println(statusMsg);
  } else {
    sendStatusMsg("No feeding schedule");
    Serial.println("No feeding schedule set");
  }
}

// V12: Clear feeding history
BLYNK_WRITE(V12) {
  if (param.asInt() == 1) {
    // Clear feeding history
    for (int i = 0; i < MAX_FEED_HISTORY; i++) {
      feedHistory[i] = 0;
      feedTimes[i] = 0;
    }

    // Reset daily statistics
    feedCount = 0;
    dailyFeedTotal = 0.0;

    // Reset total fed amount
    totalFedAmount = 0.0;

    // Send reset data
    if (Blynk.connected()) {
      Blynk.virtualWrite(V10, 0);
      Blynk.virtualWrite(V11, 0);
    }

    // Update food remaining display
    updateFoodRemaining();

    // Visual feedback with LED
    blinkLED(3, 100);

    sendStatusMsg("Feeding history and statistics cleared");
    Serial.println("Feeding history and statistics cleared");
  }
}

// V13: Reset food container
BLYNK_WRITE(V13) {
  if (param.asInt() == 1) {
    // Reset total fed amount
    totalFedAmount = 0.0;

    // Update food remaining display
    updateFoodRemaining();

    // Visual feedback with LED
    blinkLED(3, 100);

    sendStatusMsg("Food container refilled");
    Serial.println("Food container refilled");
  }
}

// Connection event handler
BLYNK_CONNECTED() {
  Serial.println("Successfully connected to Blynk server!");

  // Sync time
  rtc.begin();

  // Refresh all chart data after connection
  updateAllCharts();

  // Sync all component states
  Blynk.syncAll();
}
