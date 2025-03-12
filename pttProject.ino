#include <PulseSensorPlayground.h>
#include <LiquidCrystal.h>

// LCD pin connections
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Number of PulseSensor devices
const int PULSE_SENSOR_COUNT = 2;

// Pinout configuration
const int PULSE_INPUT0 = A0;
const int PULSE_INPUT1 = A1;
const int THRESHOLD0 = 550;  
const int THRESHOLD1 = 550;

// BP calculation constants
const float k_s = -0.446;
const float b_s = 145.0;
const float k_d = -0.225;
const float b_d = 90.7;

// All the PulseSensor Playground functions
PulseSensorPlayground pulseSensor(PULSE_SENSOR_COUNT);

// Variables for determining PTT
unsigned long lastBeatSampleNumber[PULSE_SENSOR_COUNT];
int PTT;

// Reading interval variables
unsigned long lastReadingTime = 0;
const unsigned long READING_INTERVAL = 1000; // 3 seconds in milliseconds

void setup() {
  Serial.begin(250000);

  // Configure the PulseSensor manager - only essential settings
  pulseSensor.analogInput(PULSE_INPUT0, 0);
  pulseSensor.setThreshold(THRESHOLD0, 0);

  pulseSensor.analogInput(PULSE_INPUT1, 1);
  pulseSensor.setThreshold(THRESHOLD1, 1);

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("BP Monitoring");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");
  delay(2000);
  
  // Show waiting message
  lcd.clear();
  lcd.print("PTT: Waiting...");
  lcd.setCursor(0, 1);
  lcd.print("BP: Waiting...");

  // Start reading the PulseSensor signal
  if (!pulseSensor.begin()) {
    Serial.println("PulseSensor initialization failed!");
    while(1); // Infinite loop if initialization fails
  }
}

void loop() {
  // Process samples
  if(pulseSensor.UsingHardwareTimer) {
    delay(10);
  } else {
    if (pulseSensor.sawNewSample()) {
      // Just process the sample, no output
    }
  }

  // Check for beats on each sensor
  for (int i = 0; i < PULSE_SENSOR_COUNT; ++i) {
    if (pulseSensor.sawStartOfBeat(i)) {
      // Show beat detection in serial
      if (i == 0) {
        Serial.println("Ear beat detected");
      } else {
        Serial.println("Finger beat detected");
      }
      
      lastBeatSampleNumber[i] = pulseSensor.getLastBeatTime(i);
      
      // Calculate PTT when we have a beat on sensor 1 (fingertip)
      if(i == 1){
        // PulseSensorPlayground PTT calculation
        PTT = lastBeatSampleNumber[1] - lastBeatSampleNumber[0];
        
        // Convert to milliseconds (each sample is 2ms)
        
        // Range check for valid PTT and check if 5 seconds have passed
        unsigned long currentTime = millis();
        if ((currentTime - lastReadingTime >= READING_INTERVAL) && (PTT >= 10 && PTT <= 200)) {
          // Update last reading time
          lastReadingTime = currentTime;
          
          // Update LCD with latest PTT
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("PTT: ");
          lcd.print(PTT);
          lcd.print(" ms");
          
          // Calculate BP from this PTT value
          float pttSeconds = PTT / 1000.0;
          float systolicBP = (k_s * PTT) + b_s;
          float diastolicBP = (k_d * PTT) + b_d;
          
          // Update LCD with BP
          lcd.setCursor(0, 1);
          lcd.print("BP: ");
          lcd.print(systolicBP, 1);
          lcd.print("/");
          lcd.print(diastolicBP, 1);
          lcd.print(" mmHg ");
          
          // Also show PTT and BP values in serial
          Serial.print("PTT: ");
          Serial.print(PTT);
          Serial.print(" ms | BP: ");
          Serial.print(systolicBP, 1);
          Serial.print("/");
          Serial.print(diastolicBP, 1);
          Serial.println(" mmHg");
        } else if (PTT >= 10 && PTT <= 200) {
          // Show PTT even if we're not updating the LCD yet
          Serial.print("PTT: ");
          Serial.print(PTT);
          Serial.println(" ms (waiting for interval)");
        }
      }
    }
  }
}
