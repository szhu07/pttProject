#include <PulseSensorPlayground.h>

const int PULSE_SENSOR_COUNT = 2;

// Pinout configuration
const int PULSE_INPUT0 = A0;  // Wrist sensor
const int PULSE_INPUT1 = A1;  // Fingertip sensor
const int THRESHOLD0 = 475;   // Wrist threshold
const int THRESHOLD1 = 400;   // Fingertip threshold

// Constants for calculating blood pressure
const float k_s = -0.5;  // Systolic coefficient
const float b_s = 160.0; 
const float k_d = -0.2;  
const float b_d = 100.0; 

PulseSensorPlayground pulseSensor(PULSE_SENSOR_COUNT);

// Variables for calculating PTT
unsigned long lastBeatSampleNumber[PULSE_SENSOR_COUNT];
unsigned long lastBeatTime[PULSE_SENSOR_COUNT];
bool beatDetected[PULSE_SENSOR_COUNT] = {false, false};
const unsigned long BEAT_TIMEOUT = 300; // Reduced timeout for faster processing
int PTT = 0;

// Moving average for PTT smoothing
const int PTT_SAMPLES = 5;
int pttBuffer[PTT_SAMPLES];
int pttIndex = 0;
bool bufferFilled = false;

// Timing for BP readings
unsigned long lastBPTime = 0;
const unsigned long BP_INTERVAL = 2000; // 2 seconds between readings

// Variables for retry mechanism
unsigned long lastAnyBeatTime = 0;
const unsigned long RETRY_TIMEOUT = 5000; // 5 seconds without beats before retry

// Signal monitoring
const unsigned long SIGNAL_CHECK_INTERVAL = 1000; // Check signal every second
unsigned long lastSignalCheckTime = 0;
const unsigned long MAX_BEAT_DURATION = 1000; // 1 second maximum for a single beat

void setup() {
 Serial.begin(250000);
 
 Serial.println("Initializing sensors...");
 
 // Configure the sensors
 pulseSensor.analogInput(PULSE_INPUT0, 0);
 pulseSensor.setThreshold(THRESHOLD0, 0);

 pulseSensor.analogInput(PULSE_INPUT1, 1);
 pulseSensor.setThreshold(THRESHOLD1, 1);

 if (!pulseSensor.begin()) {
   Serial.println("PulseSensor initialization failed!");
   while(1);
 }

 Serial.println("Waiting for sensors to stabilize...");
 delay(3000);
 
 Serial.println("PTT Measurement System Started");
 Serial.println("------------------------------");
 Serial.println("Note: It can take a while for both beats to be detected. Please Stay still!");
 
 lastAnyBeatTime = millis();
 lastSignalCheckTime = millis();
}

void loop() {
 unsigned long currentTime = millis();

 // Regular signal level monitoring
 if (currentTime - lastSignalCheckTime >= SIGNAL_CHECK_INTERVAL) {
   // Check for stuck beat detections
   for (int i = 0; i < PULSE_SENSOR_COUNT; i++) {
     if (beatDetected[i] && (currentTime - lastBeatTime[i] > MAX_BEAT_DURATION)) {
       beatDetected[i] = false;
       Serial.print("Reset stuck beat on sensor ");
       Serial.println(i);
     }
   }
   
   Serial.print("Signal Levels - Wrist: ");
   Serial.print(pulseSensor.getLatestSample(0));
   Serial.print(", Fingertip: ");
   Serial.println(pulseSensor.getLatestSample(1));
   
   Serial.print("Beat Status - Wrist: ");
   Serial.print(beatDetected[0]);
   Serial.print(", Fingertip: ");
   Serial.println(beatDetected[1]);
   
   lastSignalCheckTime = currentTime;
 }

 // Check if we need to retry due to no beats detected
 if (currentTime - lastAnyBeatTime > RETRY_TIMEOUT) {
   Serial.println("\n----- RETRY TRIGGERED -----");
   Serial.println("No beats detected for 5 seconds");
   Serial.println("Resetting detection system...");
   
   // Reset variables
   beatDetected[0] = false;
   beatDetected[1] = false;
   pttIndex = 0;
   bufferFilled = false;
   
   // Reinitialize the sensors
   pulseSensor.setThreshold(THRESHOLD0, 0);
   pulseSensor.setThreshold(THRESHOLD1, 1);
   
   Serial.println("Please check sensor placement:");
   Serial.println("1. Ensure good skin contact");
   Serial.println("2. Minimize movement");
   Serial.println("3. Check sensor positions");
   Serial.println("--------------------------\n");
   
   lastAnyBeatTime = currentTime;
 }

 // Only process beats if enough time has passed since last BP calculation
 if (currentTime - lastBPTime >= BP_INTERVAL || !bufferFilled) {
   // Check for beats on each sensor
   for (int i = 0; i < PULSE_SENSOR_COUNT; ++i) {
     if (pulseSensor.sawStartOfBeat(i)) {
       lastAnyBeatTime = currentTime; // Update last beat time
       unsigned long beatTime = millis();
       unsigned long sampleNumber = pulseSensor.getLastBeatTime(i);
       
       lastBeatSampleNumber[i] = sampleNumber;
       lastBeatTime[i] = beatTime;
       beatDetected[i] = true;

       if (beatDetected[0] && beatDetected[1]) {
         if (lastBeatSampleNumber[1] > lastBeatSampleNumber[0]) {
           unsigned long timeDiff = lastBeatTime[1] - lastBeatTime[0];
           
           if (timeDiff < BEAT_TIMEOUT) {
             PTT = (lastBeatSampleNumber[1] - lastBeatSampleNumber[0]) * 2;
             
             if (PTT >= 15 && PTT <= 60) {  // Tightened PTT range
               Serial.print("Both beats detected | PTT: ");
               Serial.print(PTT);
               Serial.println(" ms");

               pttBuffer[pttIndex] = PTT;
               pttIndex = (pttIndex + 1) % PTT_SAMPLES;
               
               if (pttIndex == 0) {
                 bufferFilled = true;
                 int sumPTT = 0;
                 for (int j = 0; j < PTT_SAMPLES; j++) {
                   sumPTT += pttBuffer[j];
                 }
                 float avgPTT = (float)sumPTT / PTT_SAMPLES;
                 float pttSeconds = avgPTT / 1000.0;
                 float systolicBP = k_s * (1.0 / pttSeconds) + b_s;
                 float diastolicBP = k_d * (1.0 / pttSeconds) + b_d;
                 
                 Serial.print("BP Reading: ");
                 Serial.print(systolicBP, 1);
                 Serial.print("/");
                 Serial.print(diastolicBP, 1);
                 Serial.println(" mmHg");
                 Serial.println("------------------------------");
                 
                 lastBPTime = currentTime;
               }
             }
           }
         }
         beatDetected[0] = false;
         beatDetected[1] = false;
       }
     }
   }
 }

 delay(10);  // Reduced delay for faster sampling
}
