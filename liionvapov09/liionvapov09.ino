// This is the code for the open hardware project Lion DIY Vaporizer
// The code is o course GPL
// code hacked mostly by waste, gounis, dominik (until now 24/04/15). 


// digital pins
#define LEDRED 12
#define LEDGREEN 11
#define MOSFET 10


// analogue pins
const int LMHPin = A2; // potentiometer pin
const int thermPin = A1; // thermistor pin
const int supvoltPin = A3; // voltage supply check pin



// lowmedhigh  The temperatures set are just for testing. 
// You should change them according to your needs

#define lowtempset 100
#define medtempset 150
#define hightempset 200



int LMHset = 0;
int settemp = 0 ;




//supply voltage calc
#define NUM_SAMPLES 5


float supvoldiv;
float refvolt = 5.02;
int supresist1 = 10000;
int supresist2 = 9990;

unsigned char sample_count = 0; 

//count errors to stop
int lowbatterror_count = 0;
#define LOWBAT_COUNTS 3
#define LOWBAT 6.2



// thermistor reading
#define TEMP_SAMPLES 5
int celsius = 0;
int raw = 0;
unsigned char tempsample_count = 0; 

boolean isRoufa = false;


// Thermistor lookup table for RepRap Temperature Sensor Boards (http://reprap.org/wiki/Thermistor)
// Made with the online thermistor table generator by nathan7 at http://nathan7.eu/stuff/RepRapCalculator/RepRapCalculator.html#TempLookup
// r0: 6500
// t0: 100
// r1: 0
// r2: 4700
// beta: 3950
// max adc: 1023
#define NUMTEMPS 60
short temptable[NUMTEMPS][2] = {
   {1, 912},   {11, 415},  {41, 284},  {51, 267},  {61, 253},  {71, 242},
   {81, 232},  {91, 224},  {101, 217}, {111, 211}, {121, 205}, {131, 200},
   {141, 195}, {151, 191}, {161, 187}, {171, 183}, {181, 179}, {191, 176},
   {201, 173}, {211, 170}, {221, 167}, {231, 164}, {251, 159}, {261, 156},
   {271, 154}, {311, 145}, {331, 141}, {351, 137}, {371, 134}, {401, 129},
   {411, 127}, {431, 124}, {441, 122}, {461, 119}, {491, 114}, {511, 111},
   {531, 109}, {551, 106}, {561, 104}, {581, 101}, {601, 99},  {621, 96},
   {661, 90},  {681, 87},  {721, 81},  {741, 78},  {761, 75},  {791, 70},
   {811, 67},  {841, 61},  {851, 60},  {871, 56},  {891, 51},  {931, 41},
   {941, 38}, {961, 31},   {971, 26},  {991, 15},  {1001, 8},  {1021, -28}
};   




void setup() {
  Serial.begin(9600);
  
  pinMode(LEDRED, OUTPUT);
  pinMode(LEDGREEN, OUTPUT);
  pinMode(MOSFET, OUTPUT);
  
  // supply voltage divider check 
  supvoldiv = (float)supresist2 / (supresist1 + supresist2);
  
  Serial.println("Vaporiza is in the house!");
  Serial.println("Lets check battery and start heading:");

}




void loop() {
    
  Serial.print("SupplyV");
  Serial.print("\t");
  Serial.print("LMHset");
  Serial.print("\t");
  Serial.print("Tempset:");
  Serial.print("\t");
  Serial.print("Temp:");
  Serial.println("");
  
  // error count to end 
  if ( LOWBAT_COUNTS == lowbatterror_count ) {
    digitalWrite(MOSFET, LOW);
    digitalWrite(LEDRED, HIGH); delay(250); 
    digitalWrite(LEDRED, LOW); delay(150);
    digitalWrite(LEDRED, HIGH); delay(250); 
    digitalWrite(LEDRED, LOW); delay(150);

    return;  // <- Everything behind this line will not be called ->
  } 
  
  float supvol;
  while (sample_count < NUM_SAMPLES) {
    supvol += analogRead(supvoltPin);
    sample_count++;
    delay(20);    
  }
  //Convert to actual voltage (0 - 5 Vdc)
  supvol = (((float)supvol / (float)NUM_SAMPLES) / 1024.0) * (refvolt);
  //Convert to voltage before divider ( Divide by divider = multiply || Divide by 1/5 = multiply by 5 )
  supvol = supvol / supvoldiv;
  //Output to serial
  Serial.print(supvol);
  Serial.print("\t");

  //low-no bat check
  if ( supvol < LOWBAT ) { 
    digitalWrite(MOSFET, LOW); delay(50);
    digitalWrite(LEDGREEN, LOW); delay(10);
    digitalWrite(LEDRED, HIGH); delay(1000); 
    digitalWrite(LEDRED, LOW); delay(500);
    digitalWrite(LEDRED, HIGH); delay(1000); 
    digitalWrite(LEDRED, LOW); delay(500);
    digitalWrite(LEDRED, HIGH); delay(1000); 
    digitalWrite(LEDRED, LOW); delay(1500);
    
    sample_count = 0;
    supvol = 0;
    lowbatterror_count += 1;
    
    Serial.println("");
    Serial.print("LowBat count: "); Serial.println(lowbatterror_count);
    
    return;
  }
  else { 
    
    // set temperature selector LOW MED HIGH
    
    LMHset = analogRead(LMHPin);
   Serial.print(LMHset);
   Serial.print("\t");  
  }



  if ( LMHset < 300 )        settemp = lowtempset;
  else if ( LMHset < 723 )   settemp = medtempset;
  else if ( LMHset < 1024 )  settemp = hightempset;
 
  //Output to serial 
  Serial.print(settemp); 
  Serial.print("\t");
  
  sample_count = 0;
  supvol = 0;
  
  // thermistor reading / collect samples
  while (tempsample_count < TEMP_SAMPLES) {
    raw += analogRead(thermPin);
    tempsample_count++;
    delay(20);  
  }


  //do the calculation on temp celsius according to table 
  // stolen from the reprap granny FiveD firmware
  raw = raw/TEMP_SAMPLES;
  
  
  byte i;
  for (i=1; i<NUMTEMPS; i++) { 
    if (temptable[i][0] > raw) {
      celsius = temptable[i-1][1] +
                (raw - temptable[i-1][0]) *
                (temptable[i][1] - temptable[i-1][1]) /
                (temptable[i][0] - temptable[i-1][0]);

      break;  
    }
  }
  // Overflow: Set to last value in the table
  if (i == NUMTEMPS)  celsius = temptable[i-1][1];
  
  // Clip to byte
  if (celsius > 265) { celsius = 265; }
  else if (celsius < 0){ celsius = 0; }
  
  //Output to serial 
  Serial.print(celsius);  Serial.print("\t");  Serial.println("");

  

  //mosfet controll
  if ( settemp - 15 >= celsius ) {
    isRoufa = false;
    digitalWrite(LEDGREEN, LOW); delay(5); 
    digitalWrite(LEDRED, HIGH); delay(5);
    digitalWrite(MOSFET, HIGH); delay(5);
    Serial.println("Temp not high enough -> continue heating");
  }
  else if ( settemp  < celsius ) { 
    isRoufa = true;
    digitalWrite(LEDRED, LOW); delay(5);
    digitalWrite(LEDGREEN, HIGH); delay(5);
    digitalWrite(MOSFET, LOW); delay(5);
    Serial.println("Set temp reached -> stop heating");
  }
  else if ( settemp >= celsius && settemp -15 < celsius ) {
    if (isRoufa) {
      digitalWrite(LEDRED, LOW); delay(5);
      digitalWrite(LEDGREEN, HIGH); delay(5);
      digitalWrite(MOSFET, HIGH); delay(5);
    }
    else {
      digitalWrite(LEDGREEN, LOW); delay(5); 
      digitalWrite(LEDRED, HIGH); delay(5);
      digitalWrite(MOSFET, HIGH); delay(5);
    }
    Serial.println("We try to keep the temp in the right range...");
  }

  tempsample_count = 0;
  raw = 0;
  celsius = 0;
  
} 


