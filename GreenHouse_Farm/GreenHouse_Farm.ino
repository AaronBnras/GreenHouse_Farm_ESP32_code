#include <DHT.h>

#define SOIL_MOISTURE_PIN 32
#define DHTPIN 17      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11  // DHT 11
#define RELAY_FAN_PIN 27
#define RELAY_PUMP_PIN 16  // Pin connected to the relay module controlling the pump

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);

  // Initialize relay pins
  pinMode(RELAY_PUMP_PIN, OUTPUT);
  digitalWrite(RELAY_PUMP_PIN, LOW);

  pinMode(SOIL_MOISTURE_PIN, INPUT);

  pinMode(RELAY_FAN_PIN, OUTPUT);
  digitalWrite(RELAY_FAN_PIN, LOW);

  // Initialize DHT sensor
  dht.begin();

  Serial.println("Setup completed");
}

void loop() {
  delay(2000);  // Wait for a few seconds between readings

  // Read soil moisture value
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, 0, 4095, 100, 0);

  // Read temperature and humidity data from the DHT11 sensor
  float temperatureC = dht.readTemperature();
  float temperatureF = dht.readTemperature(true);
  float humidity = dht.readHumidity();

  // Control the water pump based on soil moisture percentage
  if (soilMoisturePercent < 30) {        // Adjust threshold as needed
    digitalWrite(RELAY_PUMP_PIN, HIGH);  // Turn on the water pump
    Serial.println("Water pump turned ON");
  } else {
    digitalWrite(RELAY_PUMP_PIN, LOW);  // Turn off the water pump
    Serial.println("Water pump turned OFF");
  }

  // Add a small delay before the next reading
  delay(1000);

  // Check if any reads failed and exit early (to try again).
  if (isnan(temperatureC) || isnan(humidity) || isnan(temperatureF)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(2000);  // Wait before trying again
    return;
  }

  // Print sensor data
  Serial.print("Temperature: ");
  Serial.print(temperatureC);
  Serial.print(" °C / ");
  Serial.print(temperatureF);
  Serial.println(" °F");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.print("Soil Moisture Percentage: ");
  Serial.print(soilMoisturePercent);
  Serial.println("%");

  // Control the fan based on temperature or humidity
  if (temperatureC > 32.0 || humidity > 70.0) {  // Adjust the threshold values as needed
    digitalWrite(RELAY_FAN_PIN, HIGH);
    Serial.println("Fan turned ON");
  } else {
    digitalWrite(RELAY_FAN_PIN, LOW);
    Serial.println("Fan turned OFF");
  }

  // Add a small delay before controlling the pump to allow the system to stabilize
  delay(100);
}
