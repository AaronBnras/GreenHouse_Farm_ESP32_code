#include <DHT.h>
#include <NewPing.h>
#include <WiFi.h>
#include <ThingSpeak.h>

// Pin Definitions
#define SOIL_MOISTURE_PIN 32  // Soil moisture sensor output
#define DHTPIN 17

             // Digital pin connected to the DHT sensor
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

// ThingSpeak channel information
unsigned long myChannelNumber = 2562505;
const char* myWriteAPIKey = "QTPT88ASWOOPS4B9";

// Create a WiFi client
WiFiClient client;

// Variables to store sensor readings
int soilMoisturePercent = 0;
float waterLevel = 0;
float temperatureC = 0, temperatureF = 0, humidity = 0;

// Function to connect to WiFi
void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi already connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    return;  // Exit function if already connected
  }
  
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");

  int attempts = 0;
  const int maxAttempts = 50; // Increase maximum attempts

  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" failed to connect to WiFi");
    Serial.println("Check WiFi credentials and router configuration.");
  }
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
  } else {
    digitalWrite(RELAY_PUMP_PIN, LOW);  // Turn off the water pump
    Serial.println("Water pump turned OFF");
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
  } else {
    digitalWrite(RELAY_FAN_PIN, LOW);
    Serial.println("Fan turned OFF");
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

  // Initialize ThingSpeak
  ThingSpeak.begin(client);

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

    // Set ThingSpeak fields with sensor data
    ThingSpeak.setField(1, temperatureC);
    ThingSpeak.setField(2, humidity);
    ThingSpeak.setField(3, soilMoisturePercent);
    ThingSpeak.setField(4, waterLevel);

    // Write the fields to the ThingSpeak channel
    int x = -301;  // Initial value to trigger retry
    int retryCount = 0;
    while (x == -301 && retryCount < 5) {  // Retry up to 5 times
      x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      if (x == 200) {
        Serial.println("Channel update successful.");
      } else {
        Serial.print("Problem updating channel. HTTP error code: ");
        Serial.println(x);
        Serial.println("Retrying...");
        retryCount++;
        delay(5000);  // Wait before retrying (5 seconds)
      }
    }

    if (x == -301) {
      Serial.println("Failed to update channel after multiple retries. Check ThingSpeak status.");
      delay(2000); 
    }

    delay(20000); // Delay to avoid flooding ThingSpeak with updates
  }
}
