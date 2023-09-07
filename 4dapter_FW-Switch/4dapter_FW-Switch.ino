
#include "LUFAConfig.h"
#include <LUFA.h>
#include "Joystick.h"
#include "SegaController32U4.h"
#include "N64_Controller.h"

uint32_t buttonStatus[18];

#define DPAD_UP_MASK_ON         0x00
#define DPAD_UPRIGHT_MASK_ON    0x01
#define DPAD_RIGHT_MASK_ON      0x02
#define DPAD_DOWNRIGHT_MASK_ON  0x03
#define DPAD_DOWN_MASK_ON       0x04
#define DPAD_DOWNLEFT_MASK_ON   0x05
#define DPAD_LEFT_MASK_ON       0x06
#define DPAD_UPLEFT_MASK_ON     0x07
#define DPAD_NOTHING_MASK_ON    0x08

#define Y_MASK_ON         0x01
#define B_MASK_ON         0x02
#define A_MASK_ON         0x04
#define X_MASK_ON         0x08
#define LB_MASK_ON        0x10
#define RB_MASK_ON        0x20
#define ZL_MASK_ON        0x40
#define ZR_MASK_ON        0x80
#define SELECT_MASK_ON    0x100
#define START_MASK_ON     0x200
#define L3_MASK_ON        0x400
#define R3_MASK_ON        0x800
#define HOME_MASK_ON      0x1000
#define CAPTURE_MASK_ON   0x2000

#define BUTTONUP      0
#define BUTTONDOWN    1
#define BUTTONLEFT    2
#define BUTTONRIGHT   3
#define BUTTONA       4
#define BUTTONB       5
#define BUTTONX       6
#define BUTTONY       7
#define BUTTONLB      8
#define BUTTONRB      9
#define BUTTONLT      10
#define BUTTONRT      11
#define BUTTONSTART   12
#define BUTTONSELECT  13
#define BUTTONHOME    14
#define BUTTONCAPTURE 15
#define BUTTONL3      16
#define BUTTONR3      17

#define NES       0
#define SNES      1
#define GENESIS   2

#define BUTTONS   0
#define AXES      1

#define UP        0x01
#define DOWN      0x02
#define LEFT      0x04
#define RIGHT     0x08

#define NTT_BIT   0x00
#define NODATA    0x00

long LeftX = 0;
long LeftY = 0;
long RightX = 0;
long RightY = 0;

bool Swap_DPAD_JOY = false;
bool Swap_N64_Button = false;
bool Swap_Gen_Button = false;
#define toggleDelay 1500

//Set N64 Joystick Maximum Travel Range (0-127, typically between 75-85 on OEM controllers)
#define N64JoyMax 80

void sendLatch();
void sendClock();
void sendState();
void processInputs();

// Manage EEPROM by making sure everything has
// its own index.
enum EEPROMIndices { GENESIS_EEPROM };

// Set up USB HID gamepads
SegaController32U4  gen_controller(GENESIS_EEPROM);
N64Controller       n64_controller;
N64_status_packet   N64Data;

// Controllers
uint32_t  controllerData[2][2]  = {{0,0},{0,0}};
uint32_t  axisIndicator[32]     = {0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint16_t  currentGenesisState   = 0;
bool      nttActive             = false;

uint32_t  dataMaskNES[8] =        {0x02,   // A
                                   0x01,   // B
                                   0x40,   // Start 
                                   0x80,   // Select
                                   UP,     // D-Up
                                   DOWN,   // D-Down
                                   LEFT,   // D-Left
                                   RIGHT   // D-Right
                                   }; 

// Power Pad D4
uint32_t  dataMaskPowerPadD4[8] = {0x08,    // PowerPad #4
                                   0x04,    // PowerPad #3
                                   0x800,   // PowerPad #12
                                   0x80,    // PowerPad #8
                                   NODATA,  // No Data
                                   NODATA,  // No Data
                                   NODATA,  // No Data
                                   NODATA   // No Data
                                   }; 

// Power Pad D3
uint32_t  dataMaskPowerPadD3[8] = {0x02,   // PowerPad #2
                                   0x01,   // PowerPad #1
                                   0x10,   // PowerPad #5
                                   0x100,  // PowerPad #9
                                   0x20,   // PowerPad #6
                                   0x200,  // PowerPad #10
                                   0x400,  // PowerPad #11
                                   0x40    // PowerPad #7
                                   }; 

uint32_t  dataMaskSNES[32] =      {0x01,    // B
                                   0x04,    // Y
                                   0x40,    // Start   
                                   0x80,    // Select
                                   UP,      // D-Up
                                   DOWN,    // D-Down
                                   LEFT,    // D-Left
                                   RIGHT,   // D-Right
                                   0x02,    // A
                                   0x08,    // X
                                   0x10,    // L
                                   0x20,    // R
                                   NODATA,  // SNES Control Bit
                                   NTT_BIT, // NTT Indicator Bit
                                   NODATA,  // SNES Control Bit
                                   NODATA,  // SNES Control Bit
                                   0x100,   // NTT 0
                                   0x200,   // NTT 1
                                   0x400,   // NTT 2
                                   0x800,   // NTT 3
                                   0x1000,  // NTT 4
                                   0x2000,  // NTT 5
                                   0x4000,  // NTT 6
                                   0x8000,  // NTT 7
                                   0x10000, // NTT 8
                                   0x20000, // NTT 9
                                   0x40000, // NTT *
                                   0x80000, // NTT #
                                   0x100000,// NTT .
                                   0x200000,// NTT C
                                   NODATA,  // NTT No Data
                                   0x800000,// NTT End Comms
                                   };

void setup() 
{
  n64_controller.N64_init();

  // N64 Data pin setup
  DDRD  &= ~B00010000; // inputs
  PORTD |=  B00010000; // enable internal pull-ups

  // On Board TX LED
  DDRD  |= B00100000; // output
  PORTD |= B00100000; // high

  // On Board RX LED
  DDRB  |= B00000001; // output
  PORTB |= B00000001; // high
  
  // Setup NES / SNES latch and clock pins (2/3 or PD1/PD0)
  DDRD  |=  B00000011; // output
  PORTD &= ~B00000011; // low

  // Setup NES / SNES data pins (A0/A1 or PF6/PF7)
  DDRF  &= ~B11000000; // inputs
  PORTF |=  B11000000; // enable internal pull-ups

  // Setup NES PowerPad data pins (8/9 or PB4/PB5)
  DDRB  &= ~B00110000; // inputs
  PORTB |=  B00110000; // enable internal pull-ups

  // Setup power pin (DB9 Pin 5) as output high (PB2)
  DDRB  |= B00000100; // output
  PORTB |= B00000100; // high
  
  SetupHardware();
  GlobalInterruptEnable();
}

void loop() 
{   
    currentGenesisState = 0;
    
    //8 cycles needed to capture 6-button controllers
    for(uint8_t i = 0; i < 8; i++)
    {
      currentGenesisState = gen_controller.updateState();
    }

    currentGenesisState = gen_controller.getFinalState();
    
    for(uint8_t j = 0; j < 1; j++)
    {
      sendLatch();

      controllerData[NES][BUTTONS] = 0;
      controllerData[NES][AXES] = 0;
      
      controllerData[SNES][BUTTONS] = 0;
      controllerData[SNES][AXES] = 0;

      nttActive = false;
  
      for(uint8_t dataBitCounter = 0; dataBitCounter < 32; dataBitCounter++)
      {
        // If no NTT controller, end the loop early
        if(!nttActive && dataBitCounter > 13)
        {
          break;
        }

        //NES Power Pad Controller
        if((dataBitCounter < 8) && ((PINB & B00100000) == 0)) //Power Pad Pin D4 (bottom)
        { 
          controllerData[NES][BUTTONS] |= dataMaskPowerPadD4[dataBitCounter];
        }

        if((dataBitCounter < 8) && ((PINB & B00010000) == 0)) //Power Pad Pin D3 (middle)
        { 
          controllerData[NES][BUTTONS] |= dataMaskPowerPadD3[dataBitCounter];
        }

        // NES Controller
        if((dataBitCounter < 8) && ((PINF & B10000000) == 0)) //If NES data line is low (indicating a press)
        { 
          if(axisIndicator[dataBitCounter])
          {
            controllerData[NES][AXES] |= dataMaskNES[dataBitCounter];
          }
          else
          {
            controllerData[NES][BUTTONS] |= dataMaskNES[dataBitCounter];
          }
        }

        // SNES / NTT Controller 
        if((PINF & B01000000) == 0) //If SNES data line is low (indicating a press)
        {
          if(dataBitCounter == 13)
          {
            nttActive = true;
          }
          
          if(axisIndicator[dataBitCounter])
          {
            controllerData[SNES][AXES] |= dataMaskSNES[dataBitCounter];
          }
          else
          {
            controllerData[SNES][BUTTONS] |= dataMaskSNES[dataBitCounter];
          }
        }
        
        sendClock();
      }
    }

  __builtin_avr_delay_cycles(1000);

  n64_controller.getN64Packet();
  N64Data = n64_controller.N64_status;
  
  sendState();
}

void sendState()
{
  buttonRead();
  processInputs();
  HID_Task();
  USB_USBTask();
}

void sendLatch()
{
  // Send a latch pulse to NES/SNES
  PORTD |=  B00000010; // Set HIGH
  __builtin_avr_delay_cycles(192);
  PORTD &= ~B00000010; // Set LOW
  __builtin_avr_delay_cycles(72);
}

void sendClock()
{
  // Send a clock pulse to NES/SNES
  PORTD |=  B00000001; // Set HIGH
  __builtin_avr_delay_cycles(96);
  PORTD &= ~B00000001; // Set LOW
  __builtin_avr_delay_cycles(72);
}

void buttonRead()
{
  buttonStatus[BUTTONUP]      = (controllerData[NES][AXES] & UP)          |  (controllerData[SNES][AXES] & UP)          | ((currentGenesisState & SC_BTN_UP) >> SC_BIT_SH_UP) | (N64Data.data1 & 0x08 ? 1:0);
  buttonStatus[BUTTONDOWN]    = ((controllerData[NES][AXES] & DOWN) >> 1)  | ((controllerData[SNES][AXES] & DOWN) >> 1)  | ((currentGenesisState & SC_BTN_DOWN) >> SC_BIT_SH_DOWN) | (N64Data.data1 & 0x04 ? 1:0);
  buttonStatus[BUTTONLEFT]    = ((controllerData[NES][AXES] & LEFT) >> 2)  | ((controllerData[SNES][AXES] & LEFT) >> 2)  | ((currentGenesisState & SC_BTN_LEFT) >> SC_BIT_SH_LEFT) | (N64Data.data1 & 0x02 ? 1:0);
  buttonStatus[BUTTONRIGHT]   = ((controllerData[NES][AXES] & RIGHT) >> 3) | ((controllerData[SNES][AXES] & RIGHT) >> 3) | ((currentGenesisState & SC_BTN_RIGHT) >> SC_BIT_SH_RIGHT) | (N64Data.data1 & 0x01 ? 1:0);
  buttonStatus[BUTTONA]       = (controllerData[NES][BUTTONS] & 0x02 ? 1:0) | (controllerData[SNES][BUTTONS] & 0x02 ? 1:0) | (currentGenesisState & SC_BTN_C ? 1:0) | (N64Data.data1 & 0x80 ? 1:0);
  
  if(Swap_N64_Button)
  {
    buttonStatus[BUTTONRB]      = (controllerData[SNES][BUTTONS] & 0x20) | (currentGenesisState & SC_BTN_Z ? 1:0) | (N64Data.data2 & 0x10 ? 1:0);  
    buttonStatus[BUTTONB]       = (controllerData[NES][BUTTONS] & 0x01 ? 1:0) | (controllerData[SNES][BUTTONS] & 0x01 ? 1:0) | (currentGenesisState & SC_BTN_B ? 1:0);
    buttonStatus[BUTTONX]       = (controllerData[SNES][BUTTONS] & 0x08) | (currentGenesisState & SC_BTN_Y ? 1:0) | (N64Data.data1 & 0x40 ? 1:0);
  }
  else if(Swap_Gen_Button)
  {
    buttonStatus[BUTTONRB]      = (controllerData[SNES][BUTTONS] & 0x20) | (currentGenesisState & SC_BTN_Y ? 1:0) | (N64Data.data2 & 0x10 ? 1:0);
    buttonStatus[BUTTONB]       = (controllerData[NES][BUTTONS] & 0x01 ? 1:0) | (controllerData[SNES][BUTTONS] & 0x01 ? 1:0) | (currentGenesisState & SC_BTN_B ? 1:0) | (N64Data.data1 & 0x40 ? 1:0);
    buttonStatus[BUTTONX]       = (controllerData[SNES][BUTTONS] & 0x08) | (currentGenesisState & SC_BTN_Z ? 1:0);
  }
  else
  {
    buttonStatus[BUTTONRB]      = (controllerData[SNES][BUTTONS] & 0x20) | (currentGenesisState & SC_BTN_Z ? 1:0) | (N64Data.data2 & 0x10 ? 1:0);
    buttonStatus[BUTTONB]       = (controllerData[NES][BUTTONS] & 0x01 ? 1:0) | (controllerData[SNES][BUTTONS] & 0x01 ? 1:0) | (currentGenesisState & SC_BTN_B ? 1:0) | (N64Data.data1 & 0x40 ? 1:0);
    buttonStatus[BUTTONX]       = (controllerData[SNES][BUTTONS] & 0x08) | (currentGenesisState & SC_BTN_Y ? 1:0);
  }
  
  buttonStatus[BUTTONY]       = (controllerData[SNES][BUTTONS] & 0x04) | (currentGenesisState & SC_BTN_A ? 1:0);
  buttonStatus[BUTTONLB]      = (controllerData[SNES][BUTTONS] & 0x10) | (currentGenesisState & SC_BTN_X ? 1:0) | (N64Data.data2 & 0x20 ? 1:0);
  buttonStatus[BUTTONLT]      = (N64Data.data1 & 0x20 ? 1:0);
  buttonStatus[BUTTONRT]      = 0;
  buttonStatus[BUTTONSTART]   = (controllerData[NES][BUTTONS] & 0x80) | (controllerData[SNES][BUTTONS] & 0x80) | (currentGenesisState & SC_BTN_START ? 1:0) | (N64Data.data1 & 0x10 ? 1:0);
  buttonStatus[BUTTONSELECT]  = (controllerData[NES][BUTTONS] & 0x40) | (controllerData[SNES][BUTTONS] & 0x40) | (currentGenesisState & SC_BTN_MODE ? 1:0);
  buttonStatus[BUTTONHOME]    = (currentGenesisState & SC_BTN_HOME ? 1:0);
  buttonStatus[BUTTONCAPTURE] = 0;
  buttonStatus[BUTTONL3]      = 0;
  buttonStatus[BUTTONR3]      = 0;

  //////////////////////////////////////////////

  toggleControls();

  //////////////////////////////////////////////

  // N64: In-Game Menu Command (D-Down + Start)
  if((N64Data.data1 & 0x04 ? 1:0) && (N64Data.data1 & 0x10 ? 1:0)) 
  {
    buttonStatus[BUTTONDOWN]    = 0;
    buttonStatus[BUTTONSTART]   = 0;
    buttonStatus[BUTTONSELECT]  = 1;
  }

  // N64: Screenshot Command (D-Up + Start)
  if((N64Data.data1 & 0x08 ? 1:0) && (N64Data.data1 & 0x10 ? 1:0)) 
  {
    buttonStatus[BUTTONUP]      = 0;
    buttonStatus[BUTTONSTART]   = 0;
    buttonStatus[BUTTONCAPTURE] = 1;
  }
  
  // N64: Home Command (L + R + Start, trigger 'reset' flag on OEM controllers)
  if((N64Data.data2 & 0x80 ? 1:0) || ( (N64Data.data2 & 0x20 ? 1:0) && (N64Data.data2 & 0x10 ? 1:0) && (N64Data.data1 & 0x10 ? 1:0) ))
  {
    buttonStatus[BUTTONLB]      = 0;
    buttonStatus[BUTTONRB]      = 0;
    buttonStatus[BUTTONSTART]   = 0;
    buttonStatus[BUTTONHOME]    = 1;
  }

  /////////////////////////////////////////////

  // NES - SNES - Genesis: Home Command (Select/Mode + Start)
  if(buttonStatus[BUTTONSELECT] && buttonStatus[BUTTONSTART])  
  {
    buttonStatus[BUTTONSTART]   = 0;
    buttonStatus[BUTTONSELECT]  = 0;
    buttonStatus[BUTTONHOME]    = 1;
  }

  // NES - SNES - Genesis: In-Game Menu Command (Select/Mode + D-Down)
  if(buttonStatus[BUTTONSELECT] && buttonStatus[BUTTONDOWN])
  {
    buttonStatus[BUTTONSELECT]  = 0;
    buttonStatus[BUTTONDOWN]    = 0;
    buttonStatus[BUTTONLT]      = 1;
    buttonStatus[BUTTONRT]      = 1;
  }

  // NES - SNES - Genesis: Screenshot Command (Select/Mode + D-Up)
  if(buttonStatus[BUTTONSELECT] && buttonStatus[BUTTONUP])  
  {
    buttonStatus[BUTTONUP]      = 0;
    buttonStatus[BUTTONSELECT]  = 0;
    buttonStatus[BUTTONCAPTURE] = 1;
  }

  /////////////////////////////////////////////

  // Genesis 3-Button: In-Game Menu Command (A + B + C + Start)
  if((!gen_controller.sixButtonMode) && (currentGenesisState & SC_BTN_START ? 1:0) && (currentGenesisState & SC_BTN_A ? 1:0) && (currentGenesisState & SC_BTN_B ? 1:0) && (currentGenesisState & SC_BTN_C ? 1:0))
  {
    buttonStatus[BUTTONSTART] = 0;
    buttonStatus[BUTTONY]     = 0;
    buttonStatus[BUTTONB]     = 0;
    buttonStatus[BUTTONA]     = 0;
    buttonStatus[BUTTONLT]    = 1;
    buttonStatus[BUTTONRT]    = 1;
  }

  // Genesis 3-Button: Home Command (A + D-Down + Start)
  if((!gen_controller.sixButtonMode) && (currentGenesisState & SC_BTN_START ? 1:0) && (currentGenesisState & SC_BTN_A ? 1:0) && ((currentGenesisState & SC_BTN_DOWN) >> SC_BIT_SH_DOWN))
  {
    buttonStatus[BUTTONSTART] = 0;
    buttonStatus[BUTTONY]     = 0;
    buttonStatus[BUTTONDOWN]  = 0;
    buttonStatus[BUTTONHOME]  = 1;
  } 

  // Genesis 3-Button: Home Command (A + D-Up + Start)
  if((!gen_controller.sixButtonMode) && (currentGenesisState & SC_BTN_START ? 1:0) && (currentGenesisState & SC_BTN_A ? 1:0) && ((currentGenesisState & SC_BTN_UP) >> SC_BIT_SH_UP))
  {
    buttonStatus[BUTTONSTART]   = 0;
    buttonStatus[BUTTONY]       = 0;
    buttonStatus[BUTTONUP]      = 0;
    buttonStatus[BUTTONCAPTURE] = 1;
  }

  ///////////////////////////////////////////

  if     ((N64Data.data2 & 0x08 ? 1:0)) RightY = 0;   // C-Up
  else if((N64Data.data2 & 0x04 ? 1:0)) RightY = 255; // C-Down
  else                                  RightY = 128; // Neither C-Up / C-Down

  if     ((N64Data.data2 & 0x02 ? 1:0)) RightX = 0;   // C Left
  else if((N64Data.data2 & 0x01 ? 1:0)) RightX = 255; // C Right
  else                                  RightX = 128; // Neither C-Left / C-Right

  //////////////////////////////////////////

  if(N64Data.stick_x > N64JoyMax)   N64Data.stick_x = N64JoyMax;
  if(N64Data.stick_x < -N64JoyMax)  N64Data.stick_x = -N64JoyMax;
  if(N64Data.stick_y > N64JoyMax)   N64Data.stick_y = N64JoyMax;
  if(N64Data.stick_y < -N64JoyMax)  N64Data.stick_y = -N64JoyMax;

  LeftX = map(N64Data.stick_x, -N64JoyMax, N64JoyMax, 0, 255);
  LeftY = map(-N64Data.stick_y, -N64JoyMax, N64JoyMax, 0, 255);
}

void toggleControls()
{ 
  static unsigned long currentTime = 0;
  
  if(buttonStatus[BUTTONSELECT] && buttonStatus[BUTTONDOWN])
  {
     if(millis() - currentTime > toggleDelay)
     {
       if(!Swap_DPAD_JOY)
       {
        // Turn LEDs On
        Swap_DPAD_JOY = true;
        PORTD &= ~B00100000; // ON
       }
       else
       {
        // Turn LEDs Off
        Swap_DPAD_JOY = false;
        PORTD |= B00100000; // OFF
       }
       currentTime = millis();     
     }
  }
  else if(buttonStatus[BUTTONSELECT] && buttonStatus[BUTTONB])
  {
     if(millis() - currentTime > toggleDelay)
     {
       if(!Swap_Gen_Button)
       {
        // Turn LEDs On
        Swap_Gen_Button = true;
        PORTB &= ~B00000001; // ON
       }
       else
       {
        // Turn LEDs Off
        Swap_Gen_Button = false;
        PORTB |= B00000001; // OFF
       }
       currentTime = millis();     
     }
  }

  else if((N64Data.data1 & 0x10 ? 1:0) && (N64Data.data1 & 0x04 ? 1:0))
  {
     if(millis() - currentTime > toggleDelay)
     {
       if(!Swap_N64_Button)
       {
        // Turn LEDs On
        Swap_N64_Button = true;
        PORTB &= ~B00000001; // ON
       }
       else
       {
        // Turn LEDs Off
        Swap_N64_Button = false;
        PORTB |= B00000001; // OFF
       }
       currentTime = millis();     
     }
  }
  else
  {
    currentTime = millis();
  }
}


void processInputs()
{
  //D-Pad as Left Joystick
  if(Swap_DPAD_JOY == true)
  { 
    if      ((buttonStatus[BUTTONUP]) && (buttonStatus[BUTTONRIGHT]))     {ReportData.LX = 255; ReportData.LY = 000;}
    else if ((buttonStatus[BUTTONDOWN]) && (buttonStatus[BUTTONRIGHT]))   {ReportData.LX = 255; ReportData.LY = 255;} 
    else if ((buttonStatus[BUTTONDOWN]) && (buttonStatus[BUTTONLEFT]))    {ReportData.LX = 000; ReportData.LY = 255;}
    else if ((buttonStatus[BUTTONUP]) && (buttonStatus[BUTTONLEFT]))      {ReportData.LX = 000; ReportData.LY = 000;}
    else if (buttonStatus[BUTTONUP])                                      {ReportData.LX = 128; ReportData.LY = 000;}
    else if (buttonStatus[BUTTONDOWN])                                    {ReportData.LX = 128; ReportData.LY = 255;}
    else if (buttonStatus[BUTTONLEFT])                                    {ReportData.LX = 000; ReportData.LY = 128;}
    else if (buttonStatus[BUTTONRIGHT])                                   {ReportData.LX = 255; ReportData.LY = 128;}
    else                                                                  {ReportData.LX = 128; ReportData.LY = 128;}
    
    ReportData.HAT = DPAD_NOTHING_MASK_ON;
  }
  //D-Pad as Direction Pad (HAT)
  else
  {
    ReportData.LX = (N64Data.stick_x == 0 ? 128 : LeftX);
    ReportData.LY = (N64Data.stick_y == 0 ? 128 : LeftY);
    
    if      ((buttonStatus[BUTTONUP]) && (buttonStatus[BUTTONRIGHT]))     {ReportData.HAT = DPAD_UPRIGHT_MASK_ON;}
    else if ((buttonStatus[BUTTONDOWN]) && (buttonStatus[BUTTONRIGHT]))   {ReportData.HAT = DPAD_DOWNRIGHT_MASK_ON;} 
    else if ((buttonStatus[BUTTONDOWN]) && (buttonStatus[BUTTONLEFT]))    {ReportData.HAT = DPAD_DOWNLEFT_MASK_ON;}
    else if ((buttonStatus[BUTTONUP]) && (buttonStatus[BUTTONLEFT]))      {ReportData.HAT = DPAD_UPLEFT_MASK_ON;}
    else if (buttonStatus[BUTTONUP])                                      {ReportData.HAT = DPAD_UP_MASK_ON;}
    else if (buttonStatus[BUTTONDOWN])                                    {ReportData.HAT = DPAD_DOWN_MASK_ON;}
    else if (buttonStatus[BUTTONLEFT])                                    {ReportData.HAT = DPAD_LEFT_MASK_ON;}
    else if (buttonStatus[BUTTONRIGHT])                                   {ReportData.HAT = DPAD_RIGHT_MASK_ON;}
    else                                                                  {ReportData.HAT = DPAD_NOTHING_MASK_ON;}
  }
  
  ReportData.RX = RightX;
  ReportData.RY = RightY;

  if (buttonStatus[BUTTONA])            {ReportData.Button |= A_MASK_ON;}
  if (buttonStatus[BUTTONB])            {ReportData.Button |= B_MASK_ON;}
  if (buttonStatus[BUTTONX])            {ReportData.Button |= X_MASK_ON;}
  if (buttonStatus[BUTTONY])            {ReportData.Button |= Y_MASK_ON;}
  if (buttonStatus[BUTTONLB])           {ReportData.Button |= LB_MASK_ON;}
  if (buttonStatus[BUTTONRB])           {ReportData.Button |= RB_MASK_ON;}
  if (buttonStatus[BUTTONLT])           {ReportData.Button |= ZL_MASK_ON;}
  if (buttonStatus[BUTTONRT])           {ReportData.Button |= ZR_MASK_ON;}
  if (buttonStatus[BUTTONSTART])        {ReportData.Button |= START_MASK_ON;}
  if (buttonStatus[BUTTONSELECT])       {ReportData.Button |= SELECT_MASK_ON;}
  if (buttonStatus[BUTTONHOME])         {ReportData.Button |= HOME_MASK_ON;}
  if (buttonStatus[BUTTONCAPTURE])      {ReportData.Button |= CAPTURE_MASK_ON;}
  if (buttonStatus[BUTTONL3])           {ReportData.Button |= L3_MASK_ON;}
  if (buttonStatus[BUTTONR3])           {ReportData.Button |= R3_MASK_ON;}
}
