#include <DHT.h>
#include <NewPing.h>
#include <WiFi.h>
#include <FirebaseESP32.h>

// Pin Definitions
#define SOIL_MOISTURE_PIN 32  // Soil moisture sensor output
#define DHTPIN 17             // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11         // DHT 11
#define RELAY_FAN_PIN 27      // Pin connected to the relay module controlling the fan
#define RELAY_PUMP_PIN 16     // Pin connected to the relay module controlling the pump
#define TRIG_PIN 5            // GPIO pin connected to the TRIG pin of the ultrasonic sensor
#define ECHO_PIN 18           // GPIO pin connected to the ECHO pin of the ultrasonic sensor
#define MAX_DISTANCE 200      // Maximum distance we want to measure (in cm)

DHT dht(DHTPIN, DHTTYPE);
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

unsigned long previousMillis = 0;
const long interval = 1000;  // Interval at which to read sensors (milliseconds)

// WiFi credentials
const char* ssid = "AaronB";
const char* password = "yngezy1213";

// Firebase credentials
#define FIREBASE_HOST "https://greenhouse-farm-default-rtdb.firebaseio.com/"  //  Firebase project URL
#define FIREBASE_AUTH "y6grr34ehet0C5tXn3BJ4kzB2v7LSfRuFEOFnOH1"              //  Firebase database secret

FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// Variables to store sensor readings
int soilMoisturePercent = 0;
float waterLevel = 0;
float temperatureC = 0, temperatureF = 0, humidity = 0;
bool fanStatus = false;
bool waterPumpStatus = false;

// Function to connect to WiFi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(" connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Function to read soil moisture
int readSoilMoisture() {
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, 0, 4095, 100, 0);
  Serial.print("Soil Moisture Percentage: ");
  Serial.print(soilMoisturePercent);
  Serial.println("%");
  return soilMoisturePercent;
}

// Function to control water pump
void controlWaterPump(int soilMoisturePercent) {
  if (soilMoisturePercent < 30) {        // Adjust threshold as needed
    digitalWrite(RELAY_PUMP_PIN, HIGH);  // Turn on the water pump
    Serial.println("Water pump turned ON");
    waterPumpStatus = true;
  } else {
    digitalWrite(RELAY_PUMP_PIN, LOW);  // Turn off the water pump
    Serial.println("Water pump turned OFF");
    waterPumpStatus = false;
  }
}

// Function to read ultrasonic sensor
float readWaterLevel() {
  // Read distance from ultrasonic sensor
  const int numReadings = 10;
  float totalDistance = 0.0;
  int validReadings = 0;

  for (int i = 0; i < numReadings; i++) {
    unsigned int distance = sonar.ping_cm();
    if (distance > 0) {  // Only consider valid readings
      totalDistance += distance;
      validReadings++;
    }
    delay(50);  // Small delay between readings
  }

  if (validReadings > 0) {
    float averageDistance = totalDistance / validReadings;

    // Define the minimum and maximum water levels in cm
    const float minDistance = 8.0;  // Distance at 0% water level (sensor at bottom)
    const float maxDistance = 2.0;  // Distance at 100% water level (sensor at top)

    // Calculate water level percentage
    float waterLevelPercentage = ((minDistance - averageDistance) / (minDistance - maxDistance)) * 100.0;
    if (waterLevelPercentage < 0) waterLevelPercentage = 0;
    if (waterLevelPercentage > 100) waterLevelPercentage = 100;

    Serial.print("Water Level: ");
    Serial.print(averageDistance);
    Serial.println(" cm");
    Serial.print("Water Level Percentage: ");
    Serial.print(waterLevelPercentage);
    Serial.println("%");
    return waterLevelPercentage;
  } else {
    Serial.println("No valid readings obtained!");
    return -1;  // Return -1 to indicate an error
  }
}

// Function to read DHT sensor
bool readDHT(float& temperatureC, float& temperatureF, float& humidity) {
  for (int i = 0; i < 5; i++) {  // Try up to 5 times
    temperatureC = dht.readTemperature();
    temperatureF = dht.readTemperature(true);
    humidity = dht.readHumidity();

    if (!isnan(temperatureC) && !isnan(temperatureF) && !isnan(humidity)) {
      return true;  // Return true if valid readings are obtained
    }
    delay(100);  // Wait before trying again
  }
  return false;  // Return false if readings failed
}

// Function to control fan based on temperature and humidity
void controlFan(float temperatureC, float humidity) {
  if (temperatureC > 32.0 || humidity > 70.0) {  // Adjust the threshold values as needed
    digitalWrite(RELAY_FAN_PIN, HIGH);
    Serial.println("Fan turned ON");
    fanStatus = true;
  } else {
    digitalWrite(RELAY_FAN_PIN, LOW);
    Serial.println("Fan turned OFF");
    fanStatus = false;
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize relay pins
  pinMode(RELAY_PUMP_PIN, OUTPUT);
  digitalWrite(RELAY_PUMP_PIN, LOW);

  pinMode(SOIL_MOISTURE_PIN, INPUT);

  pinMode(RELAY_FAN_PIN, OUTPUT);
  digitalWrite(RELAY_FAN_PIN, LOW);

  // Initialize ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize DHT sensor
  dht.begin();

  // Connect to WiFi
  connectToWiFi();

  // Set Firebase configurations
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Setup completed");
}

void loop() {
  // Check WiFi connection status and reconnect if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Attempting to reconnect...");
    connectToWiFi();
  }

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Read soil moisture value
    soilMoisturePercent = readSoilMoisture();

    // Control the water pump based on soil moisture percentage
    controlWaterPump(soilMoisturePercent);

    // Measure water level using ultrasonic sensor
    waterLevel = readWaterLevel();

    // Read temperature and humidity from DHT sensor
    bool dhtReadSuccess = readDHT(temperatureC, temperatureF, humidity);

    // Check if any reads failed and print sensor data
    if (!dhtReadSuccess) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      // Print DHT sensor data
      Serial.print("Temperature: ");
      Serial.print(temperatureC);
      Serial.print(" °C / ");
      Serial.print(temperatureF);
      Serial.println(" °F");
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");

      // Control the fan based on temperature or humidity
      controlFan(temperatureC, humidity);
    }

    // Prepare data to send to Firebase
    FirebaseJson sensorData;
    FirebaseJson status;

    sensorData.set("/temperatureC", temperatureC);
    sensorData.set("/temperatureF", temperatureF);
    sensorData.set("/humidity", humidity);
    sensorData.set("/soilMoisture", soilMoisturePercent);
    sensorData.set("/waterLevel", waterLevel);

    status.set("/fan", fanStatus);
    status.set("/waterPump", waterPumpStatus);

    // Check for successful data update
    if (Firebase.updateNode(firebaseData, "/sensorData", sensorData) &&
        Firebase.updateNode(firebaseData, "/status", status)) {
      Serial.println("Data successfully sent to Firebase");
    } else {
      Serial.print("Error sending data: ");
      Serial.println(firebaseData.errorReason());
    }

    delay(1000);  // Delay to avoid flooding Firebase with updates
  }
}
