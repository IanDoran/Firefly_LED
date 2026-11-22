#include <FastLED.h>

// Number of LEDs in your strip
#define NUM_LEDS 144

// The ESP32 GPIO pin connected to the data line of the LEDs
#define DATA_PIN 25 

// --- TIMING CONFIGURATIONS ---
// Fade duration in milliseconds
#define FADE_TIME_MS 300 

// How long each LED stays ON (randomized between these two values in milliseconds)
#define MIN_ON_TIME_MS 500
#define MAX_ON_TIME_MS 1500

// How long to wait in between any one LED lighting up again (randomized between these two values)
#define MIN_WAIT_TIME_MS 10000
#define MAX_WAIT_TIME_MS 20000
// -----------------------------

// Define the array of leds
CRGB leds[NUM_LEDS];

// Define your custom color: RGB(212, 223, 76)
CRGB targetColor = CRGB(203, 228, 46);  //Firefly Green CRGB(212, 223, 76); Luna Moth green CRGB(196, 255, 76); 

// Tracking variables for independent, non-blocking random blinking and fading
unsigned long nextToggleTime[NUM_LEDS] = {0, 0, 0};
bool ledState[NUM_LEDS] = {false, false, false};

// Fading state variables for each LED
CRGB startColor[NUM_LEDS];
CRGB endColor[NUM_LEDS];
unsigned long fadeStartTime[NUM_LEDS] = {0, 0, 0};
bool isFading[NUM_LEDS] = {false, false, false};

void setup() {
    // Initialize the FastLED library for WS2812/NeoPixel type LEDs
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    
    // Set global brightness (0-255) to avoid drawing too much current
    FastLED.setBrightness(50);
    
    // Seed the random number generator using ESP32's hardware randomness
    randomSeed(esp_random());
    
    // Initialize random starting intervals for each LED
    unsigned long currentMillis = millis();
    for(int i = 0; i < NUM_LEDS; i++) {
        nextToggleTime[i] = currentMillis + random(0, MAX_WAIT_TIME_MS);
        startColor[i] = CRGB::Black;
        endColor[i] = CRGB::Black;
        leds[i] = CRGB::Black;
    }
}

void loop() {
    unsigned long currentMillis = millis();
    bool needsShow = false;

    // 1. Check each LED independently to see if it's time to trigger a state change
    for(int i = 0; i < NUM_LEDS; i++) {
        if (currentMillis >= nextToggleTime[i]) {
            // Toggle the state (ON/OFF)
            ledState[i] = !ledState[i];
            
            // Set up the fade parameters
            startColor[i] = leds[i]; // Start from whatever color it's currently at
            if (ledState[i]) {
                endColor[i] = targetColor;
                // If it just turned ON, schedule the next toggle after it stays on for a random duration
                nextToggleTime[i] = currentMillis + random(MIN_ON_TIME_MS, MAX_ON_TIME_MS + 1);
            } else {
                endColor[i] = CRGB::Black;
                // If it just turned OFF, schedule the next toggle after a random wait duration
                nextToggleTime[i] = currentMillis + random(MIN_WAIT_TIME_MS, MAX_WAIT_TIME_MS + 1);
            }
            
            fadeStartTime[i] = currentMillis;
            isFading[i] = true;
        }
    }

    // 2. Handle active fades smoothly on every loop iteration
    for(int i = 0; i < NUM_LEDS; i++) {
        if (isFading[i]) {
            unsigned long elapsed = currentMillis - fadeStartTime[i];
            
            if (elapsed >= FADE_TIME_MS) {
                // Fade is complete
                leds[i] = endColor[i];
                isFading[i] = false;
            } else {
                // Calculate intermediate color using FastLED's built-in blend function
                uint8_t progress = map(elapsed, 0, FADE_TIME_MS, 0, 255);
                leds[i] = blend(startColor[i], endColor[i], progress);
            }
            needsShow = true;
        }
    }

    // Only update the strip hardware if animation/fading is actively occurring
    if (needsShow) {
        FastLED.show();
    }
}