/*
   Apod.cpp - Library for construct and send state matrix to Bpod.
   Created by Yaoyao Hao (yaoyaoh90@gmail.com), April 2, 2020.
   Citation: Apod: An Arduino Library for Controlling Bpod (2020) https://github.com/Yaoyao-Hao/Apod/
   Released into the public domain.
*/

#ifndef Apod_h
#define Apod_h

#include "Arduino.h"
#include "String.h"

// Constant variables
const PROGMEM String EventNames[50] = { // Event codes list.
  "Port1In", "Port1Out", "Port2In", "Port2Out", "Port3In", "Port3Out", "Port4In", "Port4Out", "Port5In", "Port5Out", "Port6In", "Port6Out", "Port7In", "Port7Out", "Port8In", "Port8Out",
  "BNC1High", "BNC1Low", "BNC2High", "BNC2Low",
  "Wire1High", "Wire1Low", "Wire2High", "Wire2Low", "Wire3High", "Wire3Low", "Wire4High", "Wire4Low",
  "SoftCode1", "SoftCode2", "SoftCode3", "SoftCode4", "SoftCode5", "SoftCode6", "SoftCode7", "SoftCode8", "SoftCode9", "SoftCode10",
  "UnUsed",
  "Tup",
  "GlobalTimer1_End", "GlobalTimer2_End", "GlobalTimer3_End", "GlobalTimer4_End", "GlobalTimer5_End",
  "GlobalCounter1_End", "GlobalCounter2_End", "GlobalCounter3_End", "GlobalCounter4_End", "GlobalCounter5_End"
};
const PROGMEM String OutputActionNames[17] = { // Output action name list.
  "ValveState", "BNCState", "WireState",
  "Serial1Code", "SerialUSBCode", "SoftCode", "GlobalTimerTrig", "GlobalTimerCancel", "GlobalCounterReset",
  "PWM1", "PWM2", "PWM3", "PWM4", "PWM5", "PWM6", "PWM7", "PWM8"
};
const PROGMEM String MetaActions[4] = {"Placeholder", "Valve", "LED", "LEDState"}; // Meta action name list.
const PROGMEM int TimerScaleFactor = 10000; // Bpod: 0.1 ms resolution

// important structures
struct OutputAction {
  String OutputType;
  int Value;
};
struct StateChange {
  String StateChangeTrigger;
  String StateChangeTarget;
};
struct States {
  String Name;
  float StateTimer;
  int nStateChange = 0;
  StateChange *StateChangeCondition;
  int nOutput = 0;
  OutputAction *Output;
};
struct StateMatrix {
  byte nStates = 0;
  // byte nStatesInManifest = 0;
  // String Manifest = {}; // State names in the order they were added by user
  String StateNames[128]                   = {"Placeholder"}; //State names in the order they were added
  byte InputMatrix[128][40]                = {};
  byte OutputMatrix[128][17]               = {};
  byte GlobalTimerMatrix[128][5]           = {};
  float GlobalTimers[5]                    = {};
  byte GlobalTimerSet[5]                   = {0, 0, 0, 0, 0}; //Changed to 1 when the timer is given a duration with SetGlobalTimer
  byte GlobalCounterMatrix[128][5]         = {};
  byte GlobalCounterEvents[5]              = {254, 254, 254, 254, 254}; //Default event of 254 is code for "no event attached".
  unsigned long GlobalCounterThresholds[5] = {0, 0, 0, 0, 0};
  byte GlobalCounterSet[5]                 = {0, 0, 0, 0, 0}; //Changed to 1 when the counter event is identified and given a threshold with SetGlobalCounter
  float StateTimers[128]                   = {};
  byte StatesDefined[128]                  = {};              //Referenced states are set to 0. Defined states are set to 1. Both occur with AddState
};
struct TrialResult {
  uint16_t nEvents;
  unsigned long eventTimeStamps[10000] = {};
  byte Events[10000]                   = {};

  uint16_t nTransition;
  byte state_visited[1024] = {};
};

// main class
class Apod {
  public:
    // Construction
    Apod(Stream &s);

    // public variable
    TrialResult trial_res;

    // important functions
    void HandShakeBpod();
    States CreateState(String Name, float TimeOut, int nStateChange, StateChange* StateChangeCondition, int nOutput, OutputAction* Output);
    int AddBlankState(String statename);
    int AddState(States *state);
    void SetGlobalTimer(byte TimerNumber, float TimerDuration);
    void SetGlobalCounter(byte CounterNumber, String TargetEventName, unsigned long Threshold);
    int SendStateMatrix();
    int RunStateMatrix();
    int ReceiveBpodData();
    void EmptyMatrix();
    void setPortInputsEnabled(byte* PortEnabled);
    void setWireInputsEnabled(byte* WireEnabled);
    void ManualOverride(byte Command1, byte Command2, byte Data);

    // Serial related functions
    byte SerialReadByte();
    uint16_t SerialReadShort();
    unsigned long SerialReadLong();
    unsigned int DataReceived();
    void SerialReadAll();

    // other function
    int  find_idx(const String * str_array, int array_length, String target);
    void PrintMatrix();

  private:
    StateMatrix _sma;
    Stream* ApodSerial; // Stores the interface (Serial, Serial1, SerialUSB, etc.)
    // enable variables
    byte PortInputsEnabled[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    byte WireInputsEnabled[4] = {1, 1, 1, 1};
};

#endif
