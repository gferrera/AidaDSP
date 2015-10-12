/*
 AIDA Template Sketch
 	
 This sketch is a Template to start coding with
 Aida DSP.
 This sketch was written for the Stellaris, and will not work on other boards.
 	
 The circuit:
 
 Audio:
 * Input range 2.0Vrms (5.66Vpp, 8.23dBu)
 * Output range 0.9Vrms (2.5Vpp, 1.30dBu) 
 
 PC:
 * Please connect with PuTTY on Stellaris USB Serial with a PC for a minimal user interface
 
 NOTE:
 Attenuation Out/In = 2.264, to have out = in you must provide 7.097dB of gain through DSP algorithm
 or externally with active LPF filter.
 
 created November 2014
 by Massimo Pennazio
 
 This example code is in the public domain.
 
 */

#include <Energia.h>
#include <pins_energia.h>
#include <Wire.h>
#include "AidaFW.h"
#include "AidaDSP.h"

#define EVER (;;)

// DEFINES USER INTERFACE
#define VOLMAX 0.00
#define VOLMIN -80.00
#define FREQMIN 20.00
#define FREQMAX 20000.00

#define ON 1
#define OFF 0

// FUNCTION PROTOTYPES
void spettacolino();
void clearAndHome(void);

// GLOBAL VARIABLES
// ENCODER
int32_t OldPulses = 0;

// UI
uint8_t count = 0;
uint8_t function = 0;
uint8_t mute = OFF;
uint8_t submenu = OFF;
uint8_t preset = 0;
uint8_t restore = OFF;
uint8_t restoreflag = false;
uint16_t PotValue = 0;
uint32_t time=0, prevtime=0;

float volume = 0.00;
float frequency = 0.00;

void setup()
{
  // put your setup code here, to run once:
  // I/O
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  // open the USBSerial port
  Serial.begin(115200);
  clearAndHome();
  Serial.println("Aida DSP control with STELLARIS"); // Welcome message
  Serial.print("0x");
  Serial.println((DEVICE_ADDR_7bit<<1)&~0x01, HEX);

  // DSP board
  InitAida();	// Initialize DSP board
  digitalWrite(RESET, HIGH); // Wake up DSP
  delay(100);  // Start-up delay for DSP
  program_download();    // Here we load program, parameters and hardware configuration to DSP
  spettacolino();
  MasterVolumeMono(DEVICE_ADDR_7bit, VolDist, 0.00);    // With DAC in mute, set volume to 0
  delay(1);   
  AIDA_WRITE_REGISTER_BLOCK(DEVICE_ADDR_7bit, CoreRegisterR4Addr, CoreRegisterR4Size, CoreRegisterR4Data);    // Mute DAC Off
  set_regulation_precision(1);
}

void loop()
{
  // put your main code here, to run repeatedly:

  if(digitalRead(ENC_PUSH)==LOW)  
  {
    digitalWrite(GREEN_LED, HIGH);
    delay(50);  // debounce
    if(digitalRead(ENC_PUSH)==LOW)
    {
      count++;
    }   
  }
  else
  {
    if(count>0 && count<10)
      function = 1;
    else if(count>10 && count<30)
      function = 2;
    else
      function = 0;  // No function triggered on switch
    count = 0;
    digitalWrite(GREEN_LED, LOW);
  }

  if(function==1)
  {
    mute ^= 1;
  }
  else if(function==2)
  {
    submenu ^=1; 
  }

  time = millis();
  if(time-prevtime >= 1000)  // Here we manage control interface every second
  {
    clearAndHome();    // !!!Warning use with real terminal emulation program
    Serial.println("********************************");
    Serial.println("*    User control interface    *");
    Serial.println("*    AIDA DroneLAB             *");
    Serial.println("********************************");
    Serial.println("Press button rapidly to switch mute on/off,");
    Serial.println("press button for 1 sec to enter submenu.");
    Serial.write('\n');

    Serial.print("Encoder pulses: ");
    Serial.println(getPulses(), DEC);
    if(mute == OFF)
    {
      if(submenu == OFF)
      {
        if(restoreflag == true)
        {
          restoreflag = false;
          setPulses(OldPulses);
        }
        //volume = processencoder(VOLMIN, VOLMAX, getPulses());
        frequency = processencoder(FREQMIN, FREQMAX, getPulses());
        square_source(DEVICE_ADDR_7bit, Square1, frequency);
        delay(1);
        square_source(DEVICE_ADDR_7bit, Square2, frequency);
        delay(1);
        square_source(DEVICE_ADDR_7bit, Square3, frequency);
        delay(1);
        square_source(DEVICE_ADDR_7bit, Square4, frequency);
        delay(1);

        //Serial.print("Master Vol. ");
        //Serial.print(volume, 1);
        //Serial.println("dB");
        
        Serial.print("Freq. ");
        Serial.print(frequency, 1);
        Serial.println("Hz");

        //volume = pow(10, volume/20);    // From dB to linear conversion --> DSP takes only linear values in 5.28 fixed point format!!!
        //MasterVolumeMono(DEVICE_ADDR_7bit, VolDist, volume);
             
      }
      else
      {
        //MasterVolumeMono(DEVICE_ADDR_7bit, VolDist, volume);
      }			
    }
    else if(mute == ON)
    {
      //MasterVolumeMono(DEVICE_ADDR_7bit, VolDist, 0.00);
      Serial.println("mute on");
    }
    if(submenu==ON)
    {
      if(restoreflag == false)
      {
        restoreflag = true;
        OldPulses = getPulses();  // Save actual Pulses for restoring when exit menu
        setPulses(16);  // Restart from a known position
      }
      preset = (uint8_t)selectorwithencoder(getPulses(), 2);  // Use the encoder as a selector

      switch(preset)
      {
      case 1:
        Serial.println(" Name this preset...");
        break;
      case 2:
		Serial.println(" Name this preset...");
        break;
      case 3:
		Serial.println(" Name this preset...");
        break;
      case 4:
		Serial.println(" Name this preset...");
        break;
      }

      Serial.write('\n');
      Serial.print("    Selected preset: ");
      Serial.println(preset, DEC);

    }
    else if(submenu==OFF)
    {
      Serial.write('\n');
      Serial.println("    Submenu OFF");
    }

    prevtime = time;
  } 

} // End void loop

void spettacolino()
{
  byte i;
  byte count = 0x00;

  for(i=0;i<6;i++)
  {
    count += 1;
    if(count==1)
      digitalWrite(RED_LED, HIGH);   
    else if(count==2)
      digitalWrite(GREEN_LED, HIGH);
    else{
      digitalWrite(BLUE_LED, HIGH);
      count=0;
    }
    delay(250);
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    delay(250);
  }
}

void clearAndHome(void)
{
  Serial.write(0x1b); // ESC
  Serial.print("[2J"); // clear screen
  Serial.write(0x1b); // ESC
  Serial.print("[H"); // cursor to home
}


