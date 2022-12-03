#include <Wire.h>
#include "Adafruit_TCS34725.h"

#include <Adafruit_MotorShield.h>
#include "Servo.h"
// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61);

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #2 (M3 and M4)
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 1);

// Example code for the Adafruit TCS34725 breakout library applied to Skittle colour classification.
// This example uses RGB vectors for classification.  It also converts the RGB vector to a HSB colourspace, 
// which is likely more robust for this classification, but does not implement the HSB classification.
// (If you change to HSB, you will likely only need hue and saturation, and not brightness). 

// More information:
// Breakout board: https://www.adafruit.com/product/1334
// Library: https://github.com/adafruit/Adafruit_TCS34725
// Installation instructions: https://learn.adafruit.com/adafruit-all-about-arduino-libraries-install-use/how-to-install-a-library

   
// Initialise TCS24725 with specific int time and gain values 
// Note: 2-4 millisecond integration (sampling) times mean we can sample at about 250-500Hz
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

/*
 * Global colour sensing variables
 */
#define SERVO_TUBE_PIN 11
#define NUM_COLORS  9
#define VALID_COLORS 7
// Skittle colours to indices
#define COL_RED     0
#define COL_S_RED     1
#define COL_GREEN   2
#define COL_ORANGE  3
#define COL_S_ORANGE 4
#define COL_YELLOW  5
#define COL_PURPLE  6
#define COL_NOTHING 7
#define COL_BLUE 8

// Names for colours
#define COLNAME_RED     "RED"
#define COLNAME_S_RED "S_RED"
#define COLNAME_GREEN   "GREEN"
#define COLNAME_ORANGE  "ORANGE"
#define COLNAME_S_ORANGE "S_ORANGE"
#define COLNAME_YELLOW  "YELLOW"
#define COLNAME_PURPLE  "PURPLE"
#define COLNAME_NOTHING "WOOD"
#define COLNAME_BLUE "BLUE"
// RGB channels in the array
#define CHANNEL_R   0
#define CHANNEL_G   1
#define CHANNEL_B   2
#define RED_ANGLE 15
#define GREEN_ANGLE 45
#define YELLOW_ANGLE 135
#define ORANGE_ANGLE 175
#define PURPLE_ANGLE 90

// Training colours (populate these manually, but these vectors must be of unit length (i.e. length 1))
Servo tube_servo;
bool all_ready = false;
int color_angle_array[5];
int color_to_angle_index[VALID_COLORS] = {0,0,1,2,2,3,4};
float trainingColors[3][NUM_COLORS];    // 3(rgb) x NUM_COLORS.
int rotation = 0;
// Last read colour
float rNorm = 0.0f;
float gNorm = 0.0f;
float bNorm = 0.0f;
float hue = 0.0f;
float saturation = 0.0f;
float brightness = 0.0f;
int cur_angle = 0;
// Last classified class
int lastClass = -1;
int maxIdx;
int secondMaxIdx;
float lastCosine = 0;
float distances[NUM_COLORS] = {0.0f};

void recalibrate(){
    Serial.println("Recalibrating");
    myMotor->step(10, BACKWARD, DOUBLE);
    delay(200);
    int i = 0;
    float similarity_max = 0.0;
    int col_class = 0;
    while(true){
          Serial.println("Here");
          getNormalizedColor();
          col_class = getColorClass();
          printColourName(col_class);
          if(similarity_max > distances[col_class] and col_class == COL_BLUE or col_class == COL_PURPLE){
              myMotor->step(5, BACKWARD, DOUBLE);
              delay(200);
              getNormalizedColor();
              col_class = getColorClass();
              if (col_class == COL_NOTHING){
                 myMotor->step(5, FORWARD, DOUBLE);
                 delay(200);
                }
         
              myMotor->step(10, FORWARD, DOUBLE);
              delay(150);
              return;
              
            }
          if(col_class == COL_BLUE or col_class == COL_PURPLE){
              similarity_max = max(distances[col_class],similarity_max);
              getNormalizedColor();
              similarity_max = min(distances[col_class], similarity_max);
            }

          myMotor->step(1, FORWARD, MICROSTEP);
          delay(50);
          // Not sure on the resolution of microstep relative to regular step
          i+=1;
          if (i == 8){
            //This is to avoid the stepper motor 
            myMotor->step(10, BACKWARD, DOUBLE);
            delay(200);
            i = 0;
            }

    }
  }

/*
 * Colour sensing
 */
void initializeTrainingColors() {


  // Skittle: red 0.848  0.399 0.349
  // 0.912,0.304,0.274
  trainingColors[CHANNEL_R][COL_RED] =  0.869;
  trainingColors[CHANNEL_G][COL_RED] = 0.377;
  trainingColors[CHANNEL_B][COL_RED] = 0.320;
  // RED HIGH 0.885,0.356,0.299
  trainingColors[CHANNEL_R][COL_S_RED] =  0.888;
  trainingColors[CHANNEL_G][COL_S_RED] = 0.351;
  trainingColors[CHANNEL_B][COL_S_RED] = 0.296;
  // Skittle: green
  trainingColors[CHANNEL_R][COL_GREEN] = 0.6;
  trainingColors[CHANNEL_G][COL_GREEN] = 0.714;
  trainingColors[CHANNEL_B][COL_GREEN] = 0.354;

  // S0.928,0.319,0.194
  trainingColors[CHANNEL_R][COL_ORANGE] = 0.901;
  trainingColors[CHANNEL_G][COL_ORANGE] = 0.375;
  trainingColors[CHANNEL_B][COL_ORANGE] = 0.217;
  //0.880,0.389,0.272
   trainingColors[CHANNEL_R][COL_S_ORANGE] = 0.866;
  trainingColors[CHANNEL_G][COL_S_ORANGE] = 0.415;
  trainingColors[CHANNEL_B][COL_S_ORANGE] = 0.277;

  // 0.766,0.601,0.226
  trainingColors[CHANNEL_R][COL_YELLOW] = 0.760;
  trainingColors[CHANNEL_G][COL_YELLOW] = 0.608;
  trainingColors[CHANNEL_B][COL_YELLOW] = 0.228;
  // Skittle: purple 0.633,0.575,0.518
  trainingColors[CHANNEL_R][COL_PURPLE] = 0.633;
  trainingColors[CHANNEL_G][COL_PURPLE] = 0.575;
  trainingColors[CHANNEL_B][COL_PURPLE] = 0.518;

  //0.730,0.549,0.407
  trainingColors[CHANNEL_R][COL_NOTHING] = 0.730;
  trainingColors[CHANNEL_G][COL_NOTHING] = 0.549;
  trainingColors[CHANNEL_B][COL_NOTHING] = 0.407;
  //0.621,0.563,0.546
  trainingColors[CHANNEL_R][COL_BLUE] = 0.621;
  trainingColors[CHANNEL_G][COL_BLUE] = 0.563;
  trainingColors[CHANNEL_B][COL_BLUE] = 0.546;
}


void getNormalizedColor() {
  uint16_t r, g, b, c, colorTemp, lux;  
  tcs.getRawData(&r, &g, &b, &c);

  float lenVec = sqrt((float)r*(float)r + (float)g*(float)g + (float)b*(float)b);

  // Note: the Arduino only has 2k of RAM, so rNorm/gNorm/bNorm are global variables. 
  rNorm = (float)r/lenVec;
  gNorm = (float)g/lenVec;
  bNorm = (float)b/lenVec;

  // Also convert to HSB:
  RGBtoHSV(rNorm, gNorm, bNorm, &hue, &saturation, &brightness);
}

void flush_feeder(){
    myMotor->step(200, FORWARD, DOUBLE);
  
  
  
  }

int getColorClass() {

  // Step 1: Compute the cosine similarity between the query vector and all the training colours. 
  for (int i=0; i<NUM_COLORS; i++) {
    // For normalized (unit length) vectors, the cosine similarity is the same as the dot product of the two vectors.
    float cosineSimilarity = rNorm*trainingColors[CHANNEL_R][i] + gNorm*trainingColors[CHANNEL_G][i] + bNorm*trainingColors[CHANNEL_B][i];
    distances[i] = cosineSimilarity;
    // DEBUG: Output cosines
    Serial.print("   C"); Serial.print(i); Serial.print(": "); Serial.println(cosineSimilarity, 3);
  }
  // Step 2: Find the vector with the highest cosine (meaning, the closest to the training color)
  float maxVal = distances[0];
  for (int i=0; i<NUM_COLORS; i++) {
    if (distances[i] > maxVal) {
      //If senses wood, store second closest color

      maxVal = distances[i];
      maxIdx = i;
      if(i < VALID_COLORS){
          secondMaxIdx = maxIdx;
      }
    }
  }
  //

  if(all_ready){
    
  }
  // Step 3: Return the index of the minimum color
  lastCosine = maxVal;
  lastClass = maxIdx;
  return maxIdx;
}


// Convert from colour index to colour name.
void printColourName(int colIdx) {
  switch (colIdx) {
    case COL_RED:
      Serial.println(COLNAME_RED);
      break;
    case COL_S_RED:
      Serial.println(COLNAME_S_RED);
      break;
    case COL_GREEN:
      Serial.println(COLNAME_GREEN);
      break;
    case COL_ORANGE:
      Serial.println(COLNAME_ORANGE);
      break;
    case COL_S_ORANGE:
      Serial.println(COLNAME_S_ORANGE);
      break;
    case COL_YELLOW:
      Serial.println(COLNAME_YELLOW);
      break;
    case COL_PURPLE:
      Serial.println(COLNAME_PURPLE);
      break;
    case COL_NOTHING:
      Serial.println(COLNAME_NOTHING);
      break;
    case COL_BLUE:
      Serial.println(COLNAME_BLUE);
      break;
    default:
      Serial.print("ERROR");
      break;
  }
}

/*
 * Colour converstion
 */

// RGB to HSV.  From https://www.cs.rit.edu/~ncs/color/t_convert.html . 
void RGBtoHSV( float r, float g, float b, float *h, float *s, float *v ) {  
  float minVal = min(min(r, g), b);
  float maxVal = max(max(r, g), b);
  *v = maxVal;       // v
  float delta = maxVal - minVal;
  if( maxVal != 0 )
    *s = delta / maxVal;   // s
  else {
    // r = g = b = 0    // s = 0, v is undefined
    *s = 0;
    *h = -1;
    return;
  }
  if( r == maxVal )
    *h = ( g - b ) / delta;   // between yellow & magenta
  else if( g == maxVal )
    *h = 2 + ( b - r ) / delta; // between cyan & yellow
  else
    *h = 4 + ( r - g ) / delta; // between magenta & cyan
  *h *= 60;       // degrees
  if( *h < 0 )
    *h += 360;
}

void move_to_color(int color){
     Serial.println(color);
     color = color_to_angle_index[color];
     int delta_angle = abs(cur_angle - color_angle_array[color]);
     cur_angle = color_angle_array[color];
     Serial.println(color);
     Serial.println(cur_angle);
     tube_servo.write(cur_angle);
     delay(50+delta_angle);
  }
/*
 * Main Arduino functions
 */
 
void setup(void) {
  Serial.begin(115200);
  Serial.print("here");
  while (!Serial);
  tube_servo.attach(SERVO_TUBE_PIN);
  Serial.println("Stepper test!");

  if (!AFMS.begin()) {         // create with the default frequency 1.6KHz
  // if (!AFMS.begin(1000)) {  // OR with a different frequency, say 1KHz
    Serial.println("Could not find Motor Shield. Check wiring.");
    while (1);
  }
  Serial.println("Motor Shield found.");

  myMotor->setSpeed(10);  // 10 rpm
  // Populate array of training colours for classification. 
  initializeTrainingColors();
  
  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1);
  }
  color_angle_array[0] = RED_ANGLE;
  color_angle_array[1] = GREEN_ANGLE;
  color_angle_array[2] = ORANGE_ANGLE;
  color_angle_array[3] = YELLOW_ANGLE;
  color_angle_array[4] = PURPLE_ANGLE;
  flush_feeder();
  recalibrate();
  Serial.print("CAllibrated");
  all_ready = true;
  // Now we're ready to get readings!
}

void loop(void) {

  // Step 1: Get normalized colour vector
  getNormalizedColor();
  int colClass = getColorClass(); 
    //Avoid redundant steps in calibration
  if(all_ready){
//  Receneter the item
    if(colClass == COL_NOTHING){
         recalibrate();
         getNormalizedColor();
         colClass = getColorClass();
      }
    
  }  
  // Step 2: Output colour
    Serial.print(rNorm, 3); Serial.print(",");
    Serial.print(gNorm, 3); Serial.print(",");
    Serial.print(bNorm, 3);  Serial.print(",");
    if (colClass != COL_BLUE){
    printColourName(colClass);  
    }
    rotation ++;
  Serial.println("");
    if (maxIdx >= VALID_COLORS){
      move_to_color(secondMaxIdx);
      }
    else{
       move_to_color(maxIdx);
    }

    myMotor->step(10, FORWARD, DOUBLE);
 
   
  delay(200);
  

}
