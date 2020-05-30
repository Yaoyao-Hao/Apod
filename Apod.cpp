/*
   Apod.cpp - Library for construct and send state matrix to Bpod.
   Created by Yaoyao Hao (yaoyaoh90@gmail.com), April 2, 2020.
   Citation: Apod: An Arduino Library for Controlling Bpod (2020) https://github.com/Yaoyao-Hao/Apod/
   Released into the public domain.
*/

#include "Apod.h"

// Apod class
Apod::Apod(Stream &s) {
  ApodSerial = &s;
}

void Apod::HandShakeBpod() {
  // Handshake with Bpod
  int isHandshake = 0;
  while (!isHandshake) {
    ApodSerial->write('6'); // handshake with Bpod
    delay(100);
    while (!ApodSerial->available()) {
      SerialUSB.println("Error: Handshake failed. Tyring again...");
      delay(5000);
      ApodSerial->write('6');
      delay(100);
    }
    if (ApodSerial->read() != '5') {
      while (ApodSerial->available()) {
        ApodSerial->read();
      }
    } else {
      SerialUSB.println("Handshake with Bpod successfully.");
      isHandshake = 1;
    }
  }
}

States Apod::CreateState(String Name,                  // State Name
                         float TimeOut,                         // State Timer
                         int nStateChange,                      // Number of Conditions
                         StateChange* StateChangeCondition, // State Change Conditions
                         int nOutput,                           // Number of Output Actions
                         OutputAction* Output)                  // Output Actions
{
  States newState;
  newState.Name = Name;
  newState.StateTimer = TimeOut;
  newState.nStateChange = nStateChange;
  newState.StateChangeCondition = StateChangeCondition;
  newState.nOutput = nOutput;
  newState.Output = Output;
  return newState;
}

int Apod::AddBlankState(String statename) {
  if (statename.compareTo("exit") == 0) {
    return 0;
  }
  _sma.StateNames[_sma.nStates] = statename;
  _sma.StatesDefined[_sma.nStates] = 0;
  _sma.nStates++;
  return 0;
}

int Apod::AddState(States *state)
{
  // Check whether the new state has already been referenced. Add new blank state to matrix.
  int CurrentState;
  int referred = 0;
  for (int i = 0; i < _sma.nStates; i++) {
    // Exit if state is already defined.
    if (_sma.StateNames[i].compareTo(state->Name) == 0 && _sma.StatesDefined[i] == 1) {
      return -1;
    }

    // Check if the state is already referred.
    if (_sma.StateNames[i].compareTo(state->Name) == 0 && _sma.StatesDefined[i] == 0) {
      referred = 1;
      CurrentState = i;
    }
  }

  if (referred == 0) {
    if (_sma.StateNames[0].compareTo("Placeholder") == 0) {
      CurrentState = 0;
    } else {
      CurrentState = _sma.nStates;
    }
    // Increment states count.{
    _sma.nStates++;
    _sma.StateNames[CurrentState] = state->Name;
  }

  // Make sure all the states in "StateChangeConditions" exist, and if not, create them as undefined states.
  for (int i = 0; i < state->nStateChange; i++) {
    int flag = 0;
    for (int j = 0; j < _sma.nStates; j++) {
      if (state->StateChangeCondition[i].StateChangeTarget.compareTo(_sma.StateNames[j]) == 0) {
        flag = 1;
      }
    }
    if (flag == 0) {
      if (state->StateChangeCondition[i].StateChangeTarget.compareTo("exit") != 0) {
        _sma.StateNames[_sma.nStates] = state->StateChangeCondition[i].StateChangeTarget;
        _sma.StatesDefined[_sma.nStates] = 0;
        _sma.nStates++;
      }
    }
  }

  for (int i = 0; i < 40; i++) {
    _sma.InputMatrix[CurrentState][i] = CurrentState;
  }
  for (int i = 0; i < 5; i++) {
    _sma.GlobalTimerMatrix[CurrentState][i] = CurrentState;
    _sma.GlobalCounterMatrix[CurrentState][i] = CurrentState;
  }

  // Add state transitions.
  for (int i = 0; i < state->nStateChange; i++) {
    int CandidateEventCode = find_idx(EventNames, 50, state->StateChangeCondition[i].StateChangeTrigger);
    if (CandidateEventCode < 0) {
      _sma.nStates--;
      return -1;
    }
    String TargetState = state->StateChangeCondition[i].StateChangeTarget;
    int TargetStateNumber = 0;
    if (TargetState.compareTo("exit") == 0) {
      TargetStateNumber = _sma.nStates; //;
    } else {
      TargetStateNumber = find_idx(_sma.StateNames, _sma.nStates, TargetState);
    }

    if (CandidateEventCode > 39) {
      String CandidateEventName = state->StateChangeCondition[i].StateChangeTrigger;
      if (CandidateEventName.length() > 4) {
        if (CandidateEventName.endsWith("_End")) {
          if (CandidateEventCode < 45) {
            int GlobalTimerNumber = CandidateEventName.substring(CandidateEventName.length() - 5, CandidateEventName.length() - 4).toInt();
            if (GlobalTimerNumber < 1 || GlobalTimerNumber > 5) {
              _sma.nStates--;
              return -1;
            }
            _sma.GlobalTimerMatrix[CurrentState][GlobalTimerNumber] = TargetStateNumber;
          } else {
            int GlobalCounterNumber = CandidateEventName.substring(CandidateEventName.length() - 5, CandidateEventName.length() - 4).toInt();
            if (GlobalCounterNumber < 1 || GlobalCounterNumber > 5) {
              _sma.nStates--;
              return -1;
            }
            _sma.GlobalTimerMatrix[CurrentState][GlobalCounterNumber] = TargetStateNumber;
          }
        } else {
          _sma.nStates--;
          return -1;
        }
      }
    } else {
      _sma.InputMatrix[CurrentState][CandidateEventCode] = TargetStateNumber;
    }
  }


  // Add output actions.
  for (int i = 0; i < 17; i++) {
    _sma.OutputMatrix[CurrentState][i] = 0;
  }
  for (int i = 0; i < state->nOutput; i++) {
    int MetaAction = find_idx(MetaActions, 4, state->Output[i].OutputType);
    if (MetaAction >= 0) {
      int Value = state->Output[i].Value;
      switch (MetaAction) {
        case 1: { // VALVE
            Value = pow(2, Value - 1);
            _sma.OutputMatrix[CurrentState][1] = Value;
          }
          break;
        case 2: { // LED
            _sma.OutputMatrix[CurrentState][9 + Value] = 255;
            break;
          }
        case 3: // LED STATE
          break;
      }
    } else {
      int TargetEventCode = find_idx(OutputActionNames, 17, state->Output[i].OutputType);
      if (TargetEventCode >= 0) {
        int Value = state->Output[i].Value;
        _sma.OutputMatrix[CurrentState][TargetEventCode] = Value;
      } else {
        _sma.nStates--;
        return -1;
      }
    }
  }

  // Add self timer.
  _sma.StateTimers[CurrentState] = state->StateTimer;

  _sma.StatesDefined[CurrentState] = 1;

  // Return 0 if success.
  return 0;
}

void Apod::SetGlobalTimer(byte TimerNumber, float TimerDuration) {
  // TimerNumber: The number of the timer you are setting (an integer, 1-5).
  // TimerDuration: The duration of the timer, following timer start (0-3600 seconds)
  _sma.GlobalTimers[TimerNumber - 1] = TimerDuration;
  _sma.GlobalTimerSet[TimerNumber - 1] = 1;
}

void Apod::SetGlobalCounter(byte CounterNumber, String TargetEventName, unsigned long Threshold) {
  // CounterNumber: The number of the counter you are setting (an integer, 1-5).
  // TargetEventName: The name of the event to count (a string; see Input Event Codes)
  // Threshold: The number of event instances to count. (an integer).
  _sma.GlobalCounterThresholds[CounterNumber - 1] = Threshold;
  byte TargetEventCode = find_idx(EventNames, 50, TargetEventName);
  _sma.GlobalCounterEvents[CounterNumber - 1] = TargetEventCode;
  _sma.GlobalCounterSet[CounterNumber - 1] = 1;
}

int Apod::SendStateMatrix() {
  // clear serial
  while (ApodSerial->available()) {
    ApodSerial->read(); // clear ApodSerial dirty data
  }

  //SerialUSB.println("Start Sending.");
  byte stateNum = _sma.nStates;
  if (stateNum > 0) {
    byte output[stateNum * 71 + 59];
    int index = 0;
    byte FourthByte;
    byte ThirdByte;
    byte SecondByte;
    byte LowByte;

    output[index++] = 'P';

    output[index++] = stateNum;

    for (int i = 0; i < stateNum; i++) {
      for (int j = 0; j < 40; j++) {
        output[index++] = _sma.InputMatrix[i][j];
      }
    }

    for (int i = 0; i < stateNum; i++) {
      for (int j = 0; j < 17; j++) {
        output[index++] = _sma.OutputMatrix[i][j];
      }
    }

    for (int i = 0; i < stateNum; i++) {
      for (int j = 0; j < 5; j++) {
        output[index++] = _sma.GlobalTimerMatrix[i][j];
      }
    }

    for (int i = 0; i < stateNum; i++) {
      for (int j = 0; j < 5; j++) {
        output[index++] = _sma.GlobalCounterMatrix[i][j];
      }
    }

    for (int i = 0; i < 5; i++) {
      output[index++] = _sma.GlobalCounterEvents[i];
    }

    for (int i = 0; i < 8; i++) {
      output[index++] = PortInputsEnabled[i];
    }

    for (int i = 0; i < 4; i++) {
      output[index++] = WireInputsEnabled[i];
    }

    for (int i = 0; i < stateNum; i++) {
      unsigned long ConvertedTimer = _sma.StateTimers[i] * TimerScaleFactor;
      FourthByte = (ConvertedTimer & 0xff000000UL) >> 24;
      ThirdByte = (ConvertedTimer & 0x00ff0000UL) >> 16;
      SecondByte = (ConvertedTimer & 0x0000ff00UL) >> 8;
      LowByte = (ConvertedTimer & 0x000000ffUL);
      output[index++] = LowByte;
      output[index++] = SecondByte;
      output[index++] = ThirdByte;
      output[index++] = FourthByte;
    }

    for (int i = 0; i < 5; i++) {
      unsigned long ConvertedTimer = _sma.GlobalTimers[i] * TimerScaleFactor;
      FourthByte = (ConvertedTimer & 0xff000000UL) >> 24;
      ThirdByte = (ConvertedTimer & 0x00ff0000UL) >> 16;
      SecondByte = (ConvertedTimer & 0x0000ff00UL) >> 8;
      LowByte = (ConvertedTimer & 0x000000ffUL);
      output[index++] = LowByte;
      output[index++] = SecondByte;
      output[index++] = ThirdByte;
      output[index++] = FourthByte;
    }

    for (int i = 0; i < 5; i++) {
      FourthByte = (_sma.GlobalCounterThresholds[i] & 0xff000000UL) >> 24;
      ThirdByte = (_sma.GlobalCounterThresholds[i] & 0x00ff0000UL) >> 16;
      SecondByte = (_sma.GlobalCounterThresholds[i] & 0x0000ff00UL) >> 8;
      LowByte = (_sma.GlobalCounterThresholds[i] & 0x000000ffUL);
      output[index++] = LowByte;
      output[index++] = SecondByte;
      output[index++] = ThirdByte;
      output[index++] = FourthByte;
    }

    for (int i = 0; i < index; i++) {
      ApodSerial->write(output[i]);
    }

    byte returnVal = SerialReadByte();
    if (returnVal != 1) {
      SerialUSB.println("Error: Sending State Machine failed (invalid op code).");
      return -1;
    }
    return 0;
  } else {
    SerialUSB.println("Error: Sending Empty Matrix.");
    return -1;
  }
}

int Apod::RunStateMatrix() {
  // clear serial
  while (ApodSerial->available()) {
    ApodSerial->read(); // clear ApodSerial dirty data
  }
  // Sending indicator 'R'
  ApodSerial->write('R');
  byte returnVal = SerialReadByte();
  if (returnVal != 1) {
    SerialUSB.println("Error: Fail to run state matrix (retrunVal != 1)");
    return -1;
  }
  return 0;
}

int Apod::ReceiveBpodData() {
  byte opCode = SerialReadByte();
  trial_res = TrialResult(); // clear trial_res
  if (opCode == 1) {
    trial_res.nEvents = SerialReadShort(); // read number of events in last trial
    for (int i = 0; i < trial_res.nEvents; i++) {
      trial_res.Events[i] = SerialReadByte();          // read event ID
      trial_res.eventTimeStamps[i] = SerialReadLong(); // read event time stamp
    }
    trial_res.nTransition = SerialReadShort();         // read number of state transitions
    for (int i = 0; i < trial_res.nTransition; i++) {
      trial_res.state_visited[i] = SerialReadByte();    // read stated visited in last trial
    }
    return 0;
  } else { // error reading Bpod data...
    SerialUSB.println("Error: Receiving Bpod Data Error...");
    delay(1000);
    SerialReadAll(); // clear serial dirty data
    trial_res.nEvents = 0;
    trial_res.nTransition = 0;
    return -1;
  }
}

void Apod::EmptyMatrix() {
  _sma = StateMatrix();
}

void Apod::setPortInputsEnabled(byte* PortEnabled) {
  for (int i = 0; i < 8; i++) {
    PortInputsEnabled[i] = PortEnabled[i];
  }
}
void Apod::setWireInputsEnabled(byte* WireEnabled) {
  for (int i = 0; i < 4; i++) {
    WireInputsEnabled[i] = WireEnabled[i];
  }
}

void Apod::ManualOverride(byte Command1, byte Command2, byte Data) {
  ApodSerial->write(Command1);
  /*
     'O':  // Override hardware state
     'V':  // Manual override: execute virtual event
  */
  ApodSerial->write(Command2);
  /*
     'P': // Override PWM lines **** NOT *** supported!
     'V': // Override valves // only for 'O'
     'B': // Override BNC lines
     'W': // Override wire terminal output lines
     'S': // Override serial module port 1 for 'O' // Soft event for 'V'
     'T': // Override serial module port 2 // only for 'O'
  */
  ApodSerial->write(Data);
}

int Apod::find_idx(const String * str_array, int array_length, String target) {
  for (int i = 0; i < array_length; i++) {
    if (target.compareTo(str_array[i]) == 0) {
      return i;
    }
  }
  return -1;
}

byte Apod::SerialReadByte() {
  while (ApodSerial->available() == 0) {}
  byte LowByte = ApodSerial->read();
  return LowByte;
}

uint16_t Apod::SerialReadShort() {
  while (ApodSerial->available() == 0) {}
  byte LowByte = ApodSerial->read();
  while (ApodSerial->available() == 0) {}
  byte SecondByte = ApodSerial->read();
  uint16_t ShortInt =  (uint16_t)(((uint16_t)SecondByte << 8) | ((uint16_t)LowByte));
  return ShortInt;
}
unsigned long Apod::SerialReadLong() {
  while (ApodSerial->available() == 0) {}
  byte LowByte = ApodSerial->read();
  while (ApodSerial->available() == 0) {}
  byte SecondByte = ApodSerial->read();
  while (ApodSerial->available() == 0) {}
  byte ThirdByte = ApodSerial->read();
  while (ApodSerial->available() == 0) {}
  byte FourthByte = ApodSerial->read();
  unsigned long LongInt =  (unsigned long)(((unsigned long)FourthByte << 24) | ((unsigned long)ThirdByte << 16) | ((unsigned long)SecondByte << 8) | ((unsigned long)LowByte));
  return LongInt;
}
unsigned int Apod::DataReceived() {
  return ApodSerial->available();
}
void Apod::SerialReadAll() {
  while (ApodSerial->available()) {
    ApodSerial->read(); // clear serial
  }
}

void Apod::PrintMatrix() {
  SerialUSB.print("Number of States: ");
  SerialUSB.println(_sma.nStates);
  SerialUSB.println("StateName      Timer     Defined");
  for (int i = 0; i < _sma.nStates; i++) {
    SerialUSB.print(_sma.StateNames[i]);
    SerialUSB.print(" ");
    SerialUSB.print(_sma.StateTimers[i]);
    SerialUSB.print(" ");
    SerialUSB.print(_sma.StatesDefined[i]);
    SerialUSB.println();
  }
  SerialUSB.println("InputMatrix: ");
  for (int i = 0; i < _sma.nStates; i++) {
    for (int j = 0; j < 40; j++) {
      SerialUSB.print(_sma.InputMatrix[i][j]);
    }
    SerialUSB.println();
  }
  SerialUSB.println("OutputMatrix: ");
  for (int i = 0; i < _sma.nStates; i++) {
    for (int j = 0; j < 17; j++) {
      SerialUSB.print(_sma.OutputMatrix[i][j]);
    }
    SerialUSB.println();
  }
  SerialUSB.println("GlobalTimer: ");
  for (int i = 0; i < _sma.nStates; i++) {
    for (int j = 0; j < 5; j++) {
      SerialUSB.print(_sma.GlobalTimerMatrix[i][j]);
    }
    SerialUSB.println();
  }
  SerialUSB.println("GlobalCounter: ");
  for (int i = 0; i < _sma.nStates; i++) {
    for (int j = 0; j < 5; j++) {
      SerialUSB.print(_sma.GlobalCounterMatrix[i][j]);
    }
    SerialUSB.println();
  }
}
