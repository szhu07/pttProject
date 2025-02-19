#include <Wire.h>

// Constants for BP estimation (need calibration)
const float k_s = -0.029;    // Adjusted SBP coefficient
const float b_s = 120.0;   // Baseline SBP
const float k_d = -0.018;    // Adjusted DBP coefficient
const float b_d = 80.0;    // Baseline DBP

// Pin assignments
const int pulseSensor1 = A0; // First pulse sensor
const int pulseSensor2 = A1; // Second pulse sensor

unsigned long t1 = 0;
unsigned long t2 = 0;
bool firstPulseDetected = false;
bool secondPulseDetected = false;

// Constants for pulse detection
const unsigned long minTimeBetweenBeats = 10;  // Minimum delay in ms
int baseline1 = 0, baseline2 = 0;
int dynamicThreshold1 = 0, dynamicThreshold2 = 0;

// Function to calculate moving baseline
int getBaseline(int pin) {
    long sum = 0;
    for (int i = 0; i < 50; i++) {
        sum += analogRead(pin);
        delay(1);
    }
    return sum / 50;
}

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
    Serial.println("Initializing...");

    // Calculate initial baseline for both sensors
    baseline1 = getBaseline(pulseSensor1);
    baseline2 = getBaseline(pulseSensor2);

    // Set dynamic thresholds
    dynamicThreshold1 = baseline1 + 20;
    dynamicThreshold2 = baseline2 + 20;

    Serial.println("System Ready. Waiting for pulses...");
}

void loop() {
    // Read sensor values
    int sensor1 = smoothReading(pulseSensor1);
    int sensor2 = smoothReading(pulseSensor2);

    // Update thresholds dynamically
    baseline1 = (baseline1 * 0.9) + (sensor1 * 0.1);
    baseline2 = (baseline2 * 0.9) + (sensor2 * 0.1);
    
    dynamicThreshold1 = baseline1 + 20;
    dynamicThreshold2 = baseline2 + 20;

    // Detect the first pulse
    if (sensor1 > dynamicThreshold1 && !firstPulseDetected) {  
        t1 = millis(); 
        firstPulseDetected = true;
        Serial.print("Beat 1 Detected at: ");
        Serial.println(t1);
    }

    // Detect the second pulse, ensuring there's enough delay
    if (sensor2 > dynamicThreshold2 && firstPulseDetected && !secondPulseDetected && (millis() - t1 > minTimeBetweenBeats)) {  
        t2 = millis();

        // Make sure t2 is not equal to t1, if so, retry detection
        if (t2 == t1) {
            Serial.println("Error: t1 and t2 are the same! Retrying measurement...");
            firstPulseDetected = false; // Reset detection to try again
            secondPulseDetected = false;
            t1 = 0;
            t2 = 0;
        } else {
            secondPulseDetected = true;
            Serial.print("Beat 2 Detected at: ");
            Serial.println(t2);
        }
    }

    // Calculate PTT and estimate BP if both pulses are detected
    if (firstPulseDetected && secondPulseDetected) {  
        float PTT = (t2 - t1);
        Serial.print("PTT: ");
        Serial.print(PTT);
        Serial.println(" ms");

        // Apply filter for invalid PTT
        if (PTT < 5 || PTT > 500) {  
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

        Serial.println("Waiting for next pulses...");
        delay(5000);  // Wait before next measurement
    }

    delay(2);  // Small delay for smoother reading
}
