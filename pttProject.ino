#include <Wire.h>

// Constants for BP estimation (need calibration)
const float k_s = -1.0;    // Adjusted SBP coefficient
const float b_s = 120.0;   // Baseline SBP
const float k_d = -0.8;    // Adjusted DBP coefficient
const float b_d = 80.0;    // Baseline DBP

// Pin assignments
const int pulseSensor1 = A0; // First pulse sensor
const int pulseSensor2 = A1; // Second pulse sensor

unsigned long t1 = 0;
unsigned long t2 = 0;
bool firstPulseDetected = false;
bool secondPulseDetected = false;

// Function to smooth out noise
int smoothReading(int pin) {
    int sum = 0;
    for (int i = 0; i < 5; i++) {  
        sum += analogRead(pin);
        delay(2);
    }
    return sum / 5;  
}

void setup() {
    Serial.begin(9600);
}

void loop() {
    int sensor1 = smoothReading(pulseSensor1);
    int sensor2 = smoothReading(pulseSensor2);

    int detectionThreshold = 500;  

    if (sensor1 > detectionThreshold && !firstPulseDetected) {  
        t1 = millis(); 
        firstPulseDetected = true;
        Serial.print("Beat 1 Detected at: ");
        Serial.println(t1);
    }

    if (sensor2 > detectionThreshold && firstPulseDetected && !secondPulseDetected) {  
        t2 = millis();
        secondPulseDetected = true;
        Serial.print("Beat 2 Detected at: ");
        Serial.println(t2);
    }

    if (firstPulseDetected && secondPulseDetected && (t2 > t1)) {  
        float PTT = (t2 - t1); //gets change in pulse detected
        Serial.print("PTT: ");
        Serial.print(PTT);
        Serial.println(" ms");

        // Apply filter for invalid PTT
        if (PTT < 0.3 || PTT > 500) {  // Ignore PTT if too small or too large
            Serial.println("PTT out of range. Skipping BP calculation.");
        } else {
            // Estimate SBP & DBP
            float SBP = (k_s * (1.0/PTT)) + b_s;
            float DBP = (k_d * (1.0/PTT)) + b_d;

            // Print BP in SBP/DBP mmHg format
            Serial.print("Estimated Blood Pressure: ");
            Serial.print(SBP);
            Serial.print("/");
            Serial.print(DBP);
            Serial.println(" mmHg");
        }

        // Reset for next measurement
        firstPulseDetected = false;
        secondPulseDetected = false;
        t1 = 0;
        t2 = 0;

        delay(3000);  // Wait before next measurement
    }

    delay(2);
}
