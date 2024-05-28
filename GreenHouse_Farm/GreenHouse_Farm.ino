#include <DHT.h>

#define SOIL_MOISTURE_PIN 32  // soil moisture output
#define DHTPIN 17             // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11         // DHT 11
#define RELAY_FAN_PIN 27      // Pin connected to the relay module controlling the fan
#define RELAY_PUMP_PIN 16     // Pin connected to the relay module controlling the pump
#define TRIG_PIN 5            // GPIO pin connected to the TRIG pin of the ultrasonic sensor
#define ECHO_PIN 18           // GPIO pin connected to the TRIG pin of the ultrasonic sensor

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
  delay(1000);  // Wait for a few seconds between readings

  // Read soil moisture value
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, 0, 4095, 100, 0);


  // Control the water pump based on soil moisture percentage
  if (soilMoisturePercent < 30) {        // Adjust threshold as needed
    digitalWrite(RELAY_PUMP_PIN, HIGH);  // Turn on the water pump
    Serial.println("Water pump turned ON");
    delay(100);
  } else {
    digitalWrite(RELAY_PUMP_PIN, LOW);  // Turn off the water pump
    Serial.println("Water pump turned OFF");
    delay(100);
  }
  Serial.print("Soil Moisture Percentage: ");
  Serial.print(soilMoisturePercent);
  Serial.println("%");

  // Add a small delay before the next reading
  delay(100);

  //measure water level using ultrasonic sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * 0.0343) / 2;  // calculate the distance in cm

  if (duration == 0) {
    Serial.println("ultrasonic sensor disconnected");
  } else {
    Serial.print("Water Level: ");
    Serial.print(distance);
    Serial.println(" cm");
  }

  // Add a small delay before the next reading
  delay(100);


  float temperatureC, temperatureF, humidity;
  bool dhtReadSuccess = false;

  for (int i = 0; i < 5; i++) {  // Try up to 5 times
    temperatureC = dht.readTemperature();
    temperatureF = dht.readTemperature(true);
    humidity = dht.readHumidity();

    if (!isnan(temperatureC) && !isnan(temperatureF) && !isnan(humidity)) {
      dhtReadSuccess = true;
      break;
    }

    delay(200);  // Wait before trying again
  }

  // Check if any reads failed and exit early (to try again).
  if (!dhtReadSuccess) {
    Serial.println("Failed to read from DHT sensor!");
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
