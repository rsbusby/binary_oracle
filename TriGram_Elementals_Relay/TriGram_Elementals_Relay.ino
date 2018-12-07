//

//lower case annotation from original code UPPER CASE CURRENT VERSION

// maxbotix sonar input with a transistor and serial output
// Sonar is MaxSonar HRLV-EZ1, using pulse width interface
// The sonar has 7 pins. V+, GND, and 5 inputs.
// Pin 2- Pulse Width Output
// Pin 3- Analog Voltage Output

// input from a touch sensor
const int TOUCH_PIN = 15; //azotek

const int pwPin1_wind = 14;  // digital pin 4, goes to sonar sensor's pin 2
const int transistorPin_wind = 19;    // connected to the base of the fan transistor
long sensor_wind, mm_wind, inches_wind;  // data from sonar sensor

bool lightning_enabled = 1;
const int lightning_time_scale = 45;
const int transistorPin_lightning = 20;    // connected to the base of the light transistor
long tSig_lightning = 0;

const int pwPin1_water = 14;  // digital pin 3, goes to sonar sensor's pin 2
const int transistorPin_water = 21;    // connected to the base of the water pump transistor
long sensor_water, mm_water, inches_water;  // data from sonar sensor


// binary output to Raspberry Pi to trigger thunder
const int output_pin_thunder_sound = 12;
int sound_signal_thunder = 0;

// binary output to Raspberry Pi to trigger rain sounds
//const int output_pin_thunder_sound = 12;

int sound_signal_rain = 0;

// smoothed output signal to transistor
long tSig_wind = 0;

// bounds on expected sonar values //THESE CAN BE VALUES ASSIGNED TO TRIGRAMS? 
float low_wind = 300.0;
float high_wind = 510.0;

// smoothed output signal to transistor
long tSig_water = 0;

// last mist
long lastMist = 0;

long mistDuration = 3500;
long mistSpacing = 3000;
long mistStarted = 0;
long currently_misting = 0;

// bounds on expected sonar values
float low_water = 550.0;
float high_water = 1000.0;

void setup() {
  Serial.begin(9600); // set the baud rate
  pinMode(transistorPin_water, OUTPUT);
  pinMode(transistorPin_wind, OUTPUT);
  //pinMode(transistorPin_lightning, OUTPUT);
  //pinMode(output_pin_thunder_sound, OUTPUT);

  pinMode(pwPin1_water, INPUT);
  pinMode(pwPin1_wind, INPUT);

  // set pullup in case the sensor is not present, avoid noise
  pinMode(TOUCH_PIN, INPUT_PULLUP);

  // set the lastMist time so that it will spray immediately
  lastMist = millis() - mistSpacing;

  analogWrite(transistorPin_lightning, 0);
}

void loop() {

// wind
//  read_sensor_wind();
    sensor_wind = 0;
  int brightness_wind = setBrightnessBounded(sensor_wind, high_wind, low_wind);
//  tSig_wind = smoothSignal(brightness_wind, tSig_wind, 2);
//  analogWrite(transistorPin_wind, tSig_wind);

// water (just use wind sensor for burningman)
//read_sensor_water();
  int brightness = brightness_wind; //setBrightnessBounded(sensor_water, high_water, low_water);
  tSig_water = smoothSignal(brightness, tSig_water, 2);

// output to mac for Pure Data 
//  Serial.print("sonar_water ");
//  Serial.print(sensor_water);
//  Serial.print(brightness_wind);
//  Serial.print(" ");
//  Serial.print("sonar_wind ");
//  Serial.println(sensor_wind);
  
  bool touch_sensed = !digitalRead(TOUCH_PIN);
  
//      else{
//          analogWrite(transistorPin_lightning, 0);
//      }
  

  // control water mist
  if (currently_misting){
    Serial.println("mist");
     if (lightning_enabled) {
          tSig_lightning = tSig_lightning - lightning_time_scale;
          if (tSig_lightning < 0){
            tSig_lightning = 0;
//             tSig_lightning = 60 + random(100);
          }
          Serial.print("bright ");
          Serial.println(tSig_lightning);
          analogWrite(transistorPin_lightning, tSig_lightning);
      }
      
     

    
  
      long timeSinceMistStarted = millis() - mistStarted;
      if (timeSinceMistStarted > mistDuration){
         analogWrite(transistorPin_water, 0);

         lastMist = millis();
         currently_misting = 0;
          Serial.println("stop mist");
           analogWrite(transistorPin_lightning, 0);
      }   
     
               
 
}
  else{  
      // not misting currently, should we be misting? 
      long timeSinceMist = millis() - lastMist;

        tSig_wind = tSig_wind - 1;
                  if (tSig_wind < 0){
                    tSig_wind = 0;
                  }
        analogWrite(transistorPin_wind, tSig_wind);

      
      
//      if ((tSig_water > 200 || !digitalRead(TOUCH_PIN) ) && timeSinceMist > mistSpacing){
      if (( touch_sensed ) && timeSinceMist > mistSpacing){
          currently_misting = 1;  
          analogWrite(transistorPin_water, 255);
          tSig_wind = 255;
          analogWrite(transistorPin_wind, tSig_wind);
          mistStarted = millis();
          
          if (lightning_enabled){
            Serial.println("lightning");
            tSig_lightning = 255;
            analogWrite(transistorPin_lightning, tSig_lightning);
          }
          
      }
       
  }

}

void read_sensor_water (){
  // read using pulse width interfaces
  sensor_water = pulseIn(pwPin1_water, HIGH);
  mm_water = sensor_water;
  inches_water = mm_water/25.4;
}

void read_sensor_wind (){
  // read using pulse width interfaces
  sensor_wind = pulseIn(pwPin1_wind, HIGH);
  mm_wind = sensor_wind;
  inches_wind = mm_wind/25.4;
}

long smoothSignal(long inSig, long curSig, long fac){
   
    long newSig = int( (inSig- curSig) / fac) + curSig;
    
    if (newSig > inSig) newSig = newSig - 1;
    if (newSig < inSig) newSig = newSig + 1;

    return newSig;
}

int setBrightnessBounded(long sensor, float high, float low){  
  // normalize sensor value
  float minBrightness = 0.0;
  float maxBrightness = 255.0;

  // if no sensor data, signal should be zero.
  if (sensor == 0){
      // sensor = high;
      return 0;
  }

  int brightness = int(  (1.0 - (sensor - low)/ (high - low) ) * maxBrightness );
 
  if (brightness > maxBrightness){
     brightness = maxBrightness; 
  }
  if (brightness < minBrightness){
     brightness = minBrightness; 
  }
  return brightness; 
}
