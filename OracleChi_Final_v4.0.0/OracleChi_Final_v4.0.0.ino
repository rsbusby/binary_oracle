#include "FastLED.h"

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

// v02: adjusts the brightness based on the A0 input
// v03: reads from a digital sensor
// v3.7.0 added rough modulation of hue by the analog input. Active and standby palettes.
// v3.7.1 active palette occurs immediately. Hue now ranges from 0 to 255 even if brightness does not.
// v3.7.2 some cleanup.
// v3.7.3 dropped delay. Moved constants. Commented out constantly changing palette
// v3.7.4 added reporting of is_active
// v3.7.5 minor change in timing code
// v3.7.6 name change in brightness variables
// 3.9 added twinkle based on analog sensor value. Decreased time frome for palette changes
// 3.9.1 adjustments.  Seems buggy after running a long time in tests.
// 3.9.2 adjustments to how brightness and boost are handled. Decreased base brightness for both phases.
// 3.9.3 slightly better mode transition?
// 3.9.4 no random twinkle in standby
// 3.9.5 lower base active brightness
// 3.9.6 change hue for twinkle brightness
// 3.9.7 starting hue for twinkle brightness adjustable for standby and active modes
#define THIS_VERSION "3.9.7"

// adapted from ColorWavesWithPalettes
// Animated shifting color waves, with several cross-fading color palettes.
// by Mark Kriegsman, August 2015
//
// Color palettes courtesy of cpt-city and its contributors:
//   http://soliton.vm.bytemark.co.uk/pub/cpt-city/
//
// Color palettes converted for FastLED using "PaletteKnife" v1:
//   http://fastled.io/tools/paletteknife/
//

// installation 
#define NUM_LEDS 360
#define DATA_PIN 15
#define FAKE_ANALOG_INPUT 0
//

//// testing in CA
//#define NUM_LEDS 60
//#define DATA_PIN 6
//#define FAKE_ANALOG_INPUT 1


// touch input pin for Teensy 3.1, 3.2
#define TEENSY_TOUCH_PIN 18

// digital input pins for Azoteq sensors (note that analog input A0 is pin 14)
#define AZOTEQ_PROX_PIN 17   
#define AZOTEQ_TOUCH_PIN 16  

// LED strip properties
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB

// maximum analog signal value
#define MAX_ANALOG  1024

// Forward declarations of an array of cpt-city gradient palettes, and 
// a count of how many there are.  The actual color palette definitions
// are at the bottom of this file.
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[];
extern const uint8_t gGradientPaletteCount;

// Current palette number from the 'playlist' of color palettes
uint8_t gCurrentPaletteNumber = 0;

CRGBPalette16 gCurrentPalette( CRGB::Black);
CRGBPalette16 gTargetPalette( gGradientPalettes[32] );


// common palettes as strings for convenience, can add more if you like
#define FUSCHIA 21 
#define MAGENTA_EVENING 26
#define MAGENTA_WHITE 29
#define GREEN_WHITE 30
#define CYAN_ETC 33

unsigned int new_brightness;
unsigned int exp_brightness;

// --- PARAMETERS TO ADJUST -----

// color palette of inactive state
//CRGBPalette16 standby_palette( CRGB::Black);  // built-in dark palette

CRGBPalette16 standby_palette( gGradientPalettes[34] );
CRGBPalette16 active_palette( gGradientPalettes[36] );

uint8_t MAX_PALETTE_CHANGES_PER_BLEND = 36;

// analog event threshold 
volatile unsigned int adc_threshold = 800;

// time of increased brightness after signal, in milliseconds. 
unsigned long TIMEOUT_MILLIS = 500;

// twinkle boost
uint8_t twinkle_boost;
uint8_t twinkle_boost_standby = 90;
uint8_t twinkle_boost_active = 200;

uint8_t hue_before_boost;
uint8_t hue_before_boost_active = 100;
uint8_t hue_before_boost_standby = 40;

#define CHANCE_OF_TWINKLE 96

// brightness range active, after signal
uint8_t min_brightness_active = 5;
uint8_t max_brightness_active = 30;

// brightness range inactive (if want dark after installation, set both to 0)
uint8_t min_brightness_standby = 3;
uint8_t max_brightness_standby = 8;

unsigned int LOGGING_PERIOD_IN_MILLIS = 400;  // not too small, around 500 is good
unsigned int SENSING_PERIOD_IN_MILLIS = 40;  // 40 works

unsigned int LOOP_DELAY_IN_MILLIS = 0;  // delay for every loop 10 works, do we even need this when installed?

unsigned int num_blending_calls_to_active = 8;
unsigned int num_blending_calls_to_standby = 6;

unsigned int blending_period_to_active = 2;
unsigned int blending_period_to_standby = 2;


// --- END ADJUSTABLE PARAMETERS

unsigned int NUM_BLENDING_CALLS_PER_LOOP = 1;
unsigned int BLENDING_PERIOD_IN_MILLIS = 40;

// testing by simulating a changing analog signal
int fake_analog_increment = 2;

uint16_t active_pixel = 0;

// if changing palettes over time 
uint8_t SECONDS_PER_PALETTE = 18;

// timing and sensor variables
unsigned int adc_val;
int touchread;
uint8_t current_brightness;
uint8_t current_hue;

unsigned int brightness_gain_from_adc = 1.0;
unsigned int min_brightness = 80;
unsigned int max_brightness = 120;

unsigned long time_of_last_event = millis();
unsigned long millis_since_last_event = 0;

volatile unsigned int prox = 0;
volatile unsigned int touch = 0;
boolean is_active;

// initialize FastLED strip
CRGB leds[NUM_LEDS];


uint16_t led_boost[NUM_LEDS];
uint8_t relative_brightnesses[NUM_LEDS];


void setup() {

  Serial.begin(9600);
  Serial.print("version ");
  Serial.println(THIS_VERSION);
  Serial.println("");

  // set up digital input pins
  pinMode(AZOTEQ_PROX_PIN, INPUT);           // set pin to input
  digitalWrite(AZOTEQ_PROX_PIN, HIGH);       // turn on pullup resistors

  pinMode(AZOTEQ_TOUCH_PIN, INPUT);           // set pin to input
  digitalWrite(AZOTEQ_TOUCH_PIN, HIGH);       // turn on pullup resistors

  delay(2000); // 2 second delay for recovery
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS);
  //.setCorrection(TypicalLEDStrip) // cpt-city palettes have different color balance

  // can turn off dithering if have flickering issues
  //FastLED.setDither(BRIGHTNESS < 255);

  // set master brightness control
  FastLED.setBrightness(100);

}

// main loop
void loop()
{

  // read from Azoteq sensor, analog sensor, and set the brightness accordingly
  EVERY_N_MILLISECONDS(SENSING_PERIOD_IN_MILLIS) {
    get_brightness(); 
    //FastLED.setBrightness(current_brightness); 
  }

  // reporting to serial output
  EVERY_N_MILLISECONDS(LOGGING_PERIOD_IN_MILLIS) {
    Serial.print("Time since last event: ");
    Serial.println(millis_since_last_event);
    Serial.print("touchRead: ");
    Serial.println(touchread);
    Serial.print("Analog: ");
    Serial.println(adc_val);
    //Serial.print("FastLED Brightness Level: ");
    //Serial.println(255); // current_brightness);
    //Serial.print("Hue: ");
    //Serial.println(current_hue);
    Serial.print("Current Max Brightness: ");
    Serial.println(max_brightness);
    Serial.print("is active: ");
    Serial.println(is_active);
    Serial.println();
  }

//  // change color palette
//  EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
//    gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
//    gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
//  }

  // blend color palette
  EVERY_N_MILLISECONDS(BLENDING_PERIOD_IN_MILLIS) {
    for( unsigned int i = 0; i < NUM_BLENDING_CALLS_PER_LOOP; i++) {
      nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, MAX_PALETTE_CHANGES_PER_BLEND);
    }
  }

  //// -- waves of colors --
  //colorwaves( leds, NUM_LEDS, gCurrentPalette);

  // -- slow moving colors --
  //fill_palette( leds, NUM_LEDS, millis()/20, 4, gCurrentPalette, 255, LINEARBLEND );

  // static palette
  //fill_palette( leds, NUM_LEDS, 1, 1, gCurrentPalette, 255, LINEARBLEND );

  // static color
  for( int i = 0; i < NUM_LEDS; i++) {

     if (led_boost[i] > 0){
        led_boost[i] -= 1;               
      }
     uint8_t hue = hue_before_boost + led_boost[i] / 2;
    leds[i] = ColorFromPalette( gCurrentPalette, hue, relative_brightnesses[i], LINEARBLEND);
   }

  // max brightness
  FastLED.setBrightness(255);
  FastLED.show();
  FastLED.delay(LOOP_DELAY_IN_MILLIS);
}


void get_brightness(){

    // read from touch and proximity sensor 
    prox = !digitalRead(AZOTEQ_PROX_PIN);
    touch = !digitalRead(AZOTEQ_TOUCH_PIN);

    // read from Teensy touch pin, 16 bit number from 0 to  65535 or 32766
    touchread = touchRead(TEENSY_TOUCH_PIN);

    // read from analog input
    if ( !FAKE_ANALOG_INPUT){
      adc_val = analogRead(A0);
    }
    else{
      // fake variable analog input
      adc_val = adc_val + fake_analog_increment;
      if (adc_val > 1000 ){
        fake_analog_increment = -2;
      }
      else if (adc_val < 20){
        fake_analog_increment = 2;
      }
    }
 
    // reset timeout if either signal exists
    if (prox || touch || adc_val > adc_threshold){
      time_of_last_event = millis();
    }

    // check if current state is active
    unsigned long current_time = millis();
    millis_since_last_event = current_time - time_of_last_event;

    // set the brightness levels and palette depending on whether we are active
    if (millis_since_last_event < TIMEOUT_MILLIS){
      min_brightness = min_brightness_active;
      max_brightness = max_brightness_active;
      twinkle_boost = twinkle_boost_active;
      hue_before_boost = hue_before_boost_active;
      gTargetPalette = active_palette;
      BLENDING_PERIOD_IN_MILLIS = blending_period_to_active;
      NUM_BLENDING_CALLS_PER_LOOP = num_blending_calls_to_active;
      MAX_PALETTE_CHANGES_PER_BLEND = 36;
      is_active = true;
    }
    else{
      min_brightness = min_brightness_standby;
      max_brightness = max_brightness_standby;
      twinkle_boost = twinkle_boost_standby;
      hue_before_boost = hue_before_boost_standby;
      gTargetPalette = standby_palette;
      BLENDING_PERIOD_IN_MILLIS = blending_period_to_standby;
      NUM_BLENDING_CALLS_PER_LOOP = num_blending_calls_to_standby;
      MAX_PALETTE_CHANGES_PER_BLEND = 24;
      is_active = false;
    }

    // calculate brightness
    float normalized_signal = float(adc_val) / MAX_ANALOG;
    new_brightness = int(normalized_signal * (max_brightness - min_brightness) + min_brightness);

//    current_hue = adc_val / 4;
    
    if (new_brightness < current_brightness){
       current_brightness -= 1;
    }
    else if ( new_brightness > current_brightness){
      current_brightness += 1;
    }

//    if (is_active){
      active_pixel = int(normalized_signal * NUM_LEDS);
      led_boost[active_pixel] = twinkle_boost;
  
//    }
//    else{
//      if (random8() < CHANCE_OF_TWINKLE){
//        active_pixel = (random8() * NUM_LEDS) / 255;
//        led_boost[active_pixel] = twinkle_boost_standby;
//        Serial.print("random pixel  ");
//        Serial.print(active_pixel);
//        Serial.print("  ");
//        Serial.println(max_brightness + led_boost[active_pixel]);
//      } 
//    }
    
    for( int i = 0; i < NUM_LEDS; i++) {
      relative_brightnesses[i] = current_brightness;
      relative_brightnesses[i] += led_boost[i];      
    }
    
}

void limit_brightness(){
   // limit brightness just in case
    if (current_brightness < 0){
      current_brightness = 0;
    }
    if (current_brightness > 255){
      current_brightness = 255;
    }
}

uint8_t limit_brightness_val(uint16_t given_brightness){
   // limit brightness just in case
    if (given_brightness < 0){
      given_brightness = 0;
    }
    if (given_brightness > 255){
      given_brightness = 255;
    }
    return given_brightness;
}

// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette) 
{
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  //uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);
  
  unsigned long ms = millis();
  unsigned long deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;
  
  for( uint16_t i = 0 ; i < numleds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
    
    uint8_t index = hue8;
    //index = triwave8( index);
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette( palette, index, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (numleds-1) - pixelnumber;
    
    nblend( ledarray[pixelnumber], newcolor, 128);
  }
}

// Alternate rendering function just scrolls the current palette 
// across the defined LED strip.
void palettetest( CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette)
{
  static uint8_t startindex = 0;
  startindex--;
  fill_palette( ledarray, numleds, startindex, (256 / NUM_LEDS) + 1, gCurrentPalette, 255, LINEARBLEND);
}





// Gradient Color Palette definitions for 33 different cpt-city color palettes.
//    956 bytes of PROGMEM for all of the palettes together,
//   +618 bytes of PROGMEM for gradient palette code (AVR).
//  1,494 bytes total for all 34 color palettes and associated code.

// Gradient palette "ib_jul01_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ing/xmas/tn/ib_jul01.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

DEFINE_GRADIENT_PALETTE( ib_jul01_gp ) {
    0, 194,  1,  1,
   94,   1, 29, 18,
  132,  57,131, 28,
  255, 113,  1,  1};

// Gradient palette "es_vintage_57_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/vintage/tn/es_vintage_57.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_vintage_57_gp ) {
    0,   2,  1,  1,
   53,  18,  1,  0,
  104,  69, 29,  1,
  153, 167,135, 10,
  255,  46, 56,  4};

// Gradient palette "es_vintage_01_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/vintage/tn/es_vintage_01.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 32 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_vintage_01_gp ) {
    0,   4,  1,  1,
   51,  16,  0,  1,
   76,  97,104,  3,
  101, 255,131, 19,
  127,  67,  9,  4,
  153,  16,  0,  1,
  229,   4,  1,  1,
  255,   4,  1,  1};

// Gradient palette "es_rivendell_15_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/rivendell/tn/es_rivendell_15.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_rivendell_15_gp ) {
    0,   1, 14,  5,
  101,  16, 36, 14,
  165,  56, 68, 30,
  242, 150,156, 99,
  255, 150,156, 99};

// Gradient palette "rgi_15_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ds/rgi/tn/rgi_15.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 36 bytes of program space.

DEFINE_GRADIENT_PALETTE( rgi_15_gp ) {
    0,   4,  1, 31,
   31,  55,  1, 16,
   63, 197,  3,  7,
   95,  59,  2, 17,
  127,   6,  2, 34,
  159,  39,  6, 33,
  191, 112, 13, 32,
  223,  56,  9, 35,
  255,  22,  6, 38};

// Gradient palette "retro2_16_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ma/retro2/tn/retro2_16.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 8 bytes of program space.

DEFINE_GRADIENT_PALETTE( retro2_16_gp ) {
    0, 188,135,  1,
  255,  46,  7,  1};

// Gradient palette "Analogous_1_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/red/tn/Analogous_1.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( Analogous_1_gp ) {
    0,   3,  0,255,
   63,  23,  0,255,
  127,  67,  0,255,
  191, 142,  0, 45,
  255, 255,  0,  0};

// Gradient palette "es_pinksplash_08_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/pink_splash/tn/es_pinksplash_08.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_pinksplash_08_gp ) {
    0, 126, 11,255,
  127, 197,  1, 22,
  175, 210,157,172,
  221, 157,  3,112,
  255, 157,  3,112};

// Gradient palette "es_pinksplash_07_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/pink_splash/tn/es_pinksplash_07.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_pinksplash_07_gp ) {
    0, 229,  1,  1,
   61, 242,  4, 63,
  101, 255, 12,255,
  127, 249, 81,252,
  153, 255, 11,235,
  193, 244,  5, 68,
  255, 232,  1,  5};

// Gradient palette "Coral_reef_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/other/tn/Coral_reef.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE( Coral_reef_gp ) {
    0,  40,199,197,
   50,  10,152,155,
   96,   1,111,120,
   96,  43,127,162,
  139,  10, 73,111,
  255,   1, 34, 71};

// Gradient palette "es_ocean_breeze_068_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/ocean_breeze/tn/es_ocean_breeze_068.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_ocean_breeze_068_gp ) {
    0, 100,156,153,
   51,   1, 99,137,
  101,   1, 68, 84,
  104,  35,142,168,
  178,   0, 63,117,
  255,   1, 10, 10};

// Gradient palette "es_ocean_breeze_036_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/ocean_breeze/tn/es_ocean_breeze_036.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_ocean_breeze_036_gp ) {
    0,   1,  6,  7,
   89,   1, 99,111,
  153, 144,209,255,
  255,   0, 73, 82};

// Gradient palette "departure_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/mjf/tn/departure.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 88 bytes of program space.

DEFINE_GRADIENT_PALETTE( departure_gp ) {
    0,   8,  3,  0,
   42,  23,  7,  0,
   63,  75, 38,  6,
   84, 169, 99, 38,
  106, 213,169,119,
  116, 255,255,255,
  138, 135,255,138,
  148,  22,255, 24,
  170,   0,255,  0,
  191,   0,136,  0,
  212,   0, 55,  0,
  255,   0, 55,  0};

// Gradient palette "es_landscape_64_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/landscape/tn/es_landscape_64.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 36 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_landscape_64_gp ) {
    0,   0,  0,  0,
   37,   2, 25,  1,
   76,  15,115,  5,
  127,  79,213,  1,
  128, 126,211, 47,
  130, 188,209,247,
  153, 144,182,205,
  204,  59,117,250,
  255,   1, 37,192};

// Gradient palette "es_landscape_33_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/landscape/tn/es_landscape_33.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_landscape_33_gp ) {
    0,   1,  5,  0,
   19,  32, 23,  1,
   38, 161, 55,  1,
   63, 229,144,  1,
   66,  39,142, 74,
  255,   1,  4,  1};

// Gradient palette "rainbowsherbet_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ma/icecream/tn/rainbowsherbet.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( rainbowsherbet_gp ) {
    0, 255, 33,  4,
   43, 255, 68, 25,
   86, 255,  7, 25,
  127, 255, 82,103,
  170, 255,255,242,
  209,  42,255, 22,
  255,  87,255, 65};

// Gradient palette "gr65_hult_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/hult/tn/gr65_hult.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE( gr65_hult_gp ) {
    0, 247,176,247,
   48, 255,136,255,
   89, 220, 29,226,
  160,   7, 82,178,
  216,   1,124,109,
  255,   1,124,109};

// Gradient palette "gr64_hult_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/hult/tn/gr64_hult.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 32 bytes of program space.

DEFINE_GRADIENT_PALETTE( gr64_hult_gp ) {
    0,   1,124,109,
   66,   1, 93, 79,
  104,  52, 65,  1,
  130, 115,127,  1,
  150,  52, 65,  1,
  201,   1, 86, 72,
  239,   0, 55, 45,
  255,   0, 55, 45};

// Gradient palette "GMT_drywet_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/gmt/tn/GMT_drywet.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( GMT_drywet_gp ) {
    0,  47, 30,  2,
   42, 213,147, 24,
   84, 103,219, 52,
  127,   3,219,207,
  170,   1, 48,214,
  212,   1,  1,111,
  255,   1,  7, 33};

// Gradient palette "ib15_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ing/general/tn/ib15.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 24 bytes of program space.

DEFINE_GRADIENT_PALETTE( ib15_gp ) {
    0, 113, 91,147,
   72, 157, 88, 78,
   89, 208, 85, 33,
  107, 255, 29, 11,
  141, 137, 31, 39,
  255,  59, 33, 89};

// Gradient palette "Fuschia_7_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/ds/fuschia/tn/Fuschia-7.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( Fuschia_7_gp ) {
    0,  43,  3,153,
   63, 100,  4,103,
  127, 188,  5, 66,
  191, 161, 11,115,
  255, 135, 20,182};

// Gradient palette "es_emerald_dragon_08_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/emerald_dragon/tn/es_emerald_dragon_08.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_emerald_dragon_08_gp ) {
    0,  97,255,  1,
  101,  47,133,  1,
  178,  13, 43,  1,
  255,   2, 10,  1};

// Gradient palette "lava_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/neota/elem/tn/lava.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 52 bytes of program space.

DEFINE_GRADIENT_PALETTE( lava_gp ) {
    0,   0,  0,  0,
   46,  18,  0,  0,
   96, 113,  0,  0,
  108, 142,  3,  1,
  119, 175, 17,  1,
  146, 213, 44,  2,
  174, 255, 82,  4,
  188, 255,115,  4,
  202, 255,156,  4,
  218, 255,203,  4,
  234, 255,255,  4,
  244, 255,255, 71,
  255, 255,255,255};

// Gradient palette "fire_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/neota/elem/tn/fire.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( fire_gp ) {
    0,   1,  1,  0,
   76,  32,  5,  0,
  146, 192, 24,  0,
  197, 220,105,  5,
  240, 252,255, 31,
  250, 252,255,111,
  255, 255,255,255};

// Gradient palette "Colorfull_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Colorfull.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 44 bytes of program space.

DEFINE_GRADIENT_PALETTE( Colorfull_gp ) {
    0,  10, 85,  5,
   25,  29,109, 18,
   60,  59,138, 42,
   93,  83, 99, 52,
  106, 110, 66, 64,
  109, 123, 49, 65,
  113, 139, 35, 66,
  116, 192,117, 98,
  124, 255,255,137,
  168, 100,180,155,
  255,  22,121,174};


// Gradient palette "Magenta_Evening_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Magenta_Evening.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( Magenta_Evening_gp ) {
    0,  71, 27, 39,
   31, 130, 11, 51,
   63, 213,  2, 64,
   70, 232,  1, 66,
   76, 252,  1, 69,
  108, 123,  2, 51,
  255,  46,  9, 35};

// Gradient palette "Pink_Purple_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Pink_Purple.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 44 bytes of program space.

DEFINE_GRADIENT_PALETTE( Pink_Purple_gp ) {
    0,  19,  2, 39,
   25,  26,  4, 45,
   51,  33,  6, 52,
   76,  68, 62,125,
  102, 118,187,240,
  109, 163,215,247,
  114, 217,244,255,
  122, 159,149,221,
  149, 113, 78,188,
  183, 128, 57,155,
  255, 146, 40,123};

// Gradient palette "Sunset_Real_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Sunset_Real.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( Sunset_Real_gp ) {
    0, 120,  0,  0,
   22, 179, 22,  0,
   51, 255,104,  0,
   85, 167, 22, 18,
  135, 100,  0,103,
  198,  16,  0,130,
  255,   0,  0,160};

// Gradient palette "es_autumn_19_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/es/autumn/tn/es_autumn_19.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 52 bytes of program space.

DEFINE_GRADIENT_PALETTE( es_autumn_19_gp ) {
    0,  26,  1,  1,
   51,  67,  4,  1,
   84, 118, 14,  1,
  104, 137,152, 52,
  112, 113, 65,  1,
  122, 133,149, 59,
  124, 137,152, 52,
  135, 113, 65,  1,
  142, 139,154, 46,
  163, 113, 13,  1,
  204,  55,  3,  1,
  249,  17,  1,  1,
  255,  17,  1,  1};

// Gradient palette "Magenta_White_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/Magenta_White.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 12 bytes of program space.

DEFINE_GRADIENT_PALETTE( Magenta_White_gp ) {
    0, 255,  0,255,
  127, 255, 55,255,
  255, 255,255,255};

// Gradient palette "Green_White_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/Green_White.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 12 bytes of program space.

DEFINE_GRADIENT_PALETTE( Green_White_gp ) {
    0,   0,255,  0,
  127,  42,255, 45,
  255, 255,255,255};

// Gradient palette "BlacK_Magenta_Red_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/BlacK_Magenta_Red.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( BlacK_Magenta_Red_gp ) {
    0,   0,  0,  0,
   63,  42,  0, 45,
  127, 255,  0,255,
  191, 255,  0, 45,
  255, 255,  0,  0};

// Gradient palette "BlacK_Red_Magenta_Yellow_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/BlacK_Red_Magenta_Yellow.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( BlacK_Red_Magenta_Yellow_gp ) {
    0,   0,  0,  0,
   42,  42,  0,  0,
   84, 255,  0,  0,
  127, 255,  0, 45,
  170, 255,  0,255,
  212, 255, 55, 45,
  255, 255,255,  0};


// Gradient palette "Blue_Cyan_Yellow_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/Blue_Cyan_Yellow.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( Blue_Cyan_Yellow_gp ) {
    0,   0,  0,255,
   63,   0, 55,255,
  127,   0,255,255,
  191,  42,255, 45,
  255, 255,255,  0};

  
// Gradient palette "bhw1_06_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw1/tn/bhw1_06.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

DEFINE_GRADIENT_PALETTE( bhw1_06_gp ) {
    0, 184,  1,128,
  160,   1,193,182,
  219, 153,227,190,
  255, 255,255,255};

// Gradient palette "bhw1_16_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw1/tn/bhw1_16.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 8 bytes of program space.

DEFINE_GRADIENT_PALETTE( bhw1_16_gp ) {
    0, 252,255,102,
  255, 155,213,250};
// Single array of defined cpt-city color palettes.
// This will let us programmatically choose one based on
// a number, rather than having to activate each explicitly 
// by name every time.
// Since it is const, this array could also be moved 
// into PROGMEM to save SRAM, but for simplicity of illustration
// we'll keep it in a regular SRAM array.
//
// This list of color palettes acts as a "playlist"; you can
// add or delete, or re-arrange as you wish.
const TProgmemRGBGradientPalettePtr gGradientPalettes[] = {
  Sunset_Real_gp,
  es_rivendell_15_gp,
  es_ocean_breeze_036_gp, // 2
  rgi_15_gp,
  retro2_16_gp,
  Analogous_1_gp,
  es_pinksplash_08_gp,
  Coral_reef_gp,
  es_ocean_breeze_068_gp,
  es_pinksplash_07_gp,
  es_vintage_01_gp,
  departure_gp,
  es_landscape_64_gp,
  es_landscape_33_gp,
  rainbowsherbet_gp,
  gr65_hult_gp,
  gr64_hult_gp,
  GMT_drywet_gp,
  ib_jul01_gp,
  es_vintage_57_gp,
  ib15_gp,
  Fuschia_7_gp,  // 21
  es_emerald_dragon_08_gp, //22
  lava_gp,
  fire_gp,
  Colorfull_gp,
  Magenta_Evening_gp, //26
  Pink_Purple_gp,
  es_autumn_19_gp,
  Magenta_White_gp, //29
  Green_White_gp, //30
  BlacK_Magenta_Red_gp,
  BlacK_Red_Magenta_Yellow_gp,
  Blue_Cyan_Yellow_gp,
  bhw1_06_gp,//34
  bhw1_16_gp//35
};


// Count of how many cpt-city gradients are defined:
const uint8_t gGradientPaletteCount = 
  sizeof( gGradientPalettes) / sizeof( TProgmemRGBGradientPalettePtr );
