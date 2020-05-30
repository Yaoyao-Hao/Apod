#include "Apod.h"

/*  Hardware Connection
    Arduino (this program) Serial1   <---->  Serial1 of Bpod (with modified firmware "Bpod_Firmware_0_5_modified")
*/

/* Debug info output:
    ----------Arduino.SerialUSB-----------
*/

#define MAX_TRIAL_NUM 9999

// Initializaton
Apod apod(Serial1); // init with Serial port connectted to Bpod

// other public variables
byte TrialType = 0;

void setup() {
  // put your setup code here, to run once:

  //delay(3000); // for debug

  apod.HandShakeBpod(); // Hand shake with Bpod; get stuck until connected

  byte PortInputsEnabled[8] = {1, 1, 1, 0, 0, 0, 0, 0};
  byte WireInputsEnabled[4] = {0, 0, 0, 0};
  apod.setPortInputsEnabled(PortInputsEnabled);
  apod.setWireInputsEnabled(WireInputsEnabled);

  StateChange WaitForChoice_Cond1[] = {{"Port1In", "FlashPort1"}, {"Port2In", "FlashPort2"}};
  StateChange WaitForChoice_Cond2[] = {{"Port1In", "FlashPort2"}, {"Port2In", "FlashPort1"}};
  StateChange FlashPort1_Cond[]    = {{"Tup", "WaitForExit"}};
  StateChange FlashPort2_Cond[]    = {{"Tup", "WaitForExit"}};
  StateChange WaitForExit_Cond[]   = {{"Port1In", "exit"}, {"Port2In", "exit"}, {"Port3In", "WaitForChoice"}};

  OutputAction FlashPort1_output[]     = {{"BNCState", 1}, {"ValveState", 1}};
  OutputAction FlashPort2_output[]     = {{"PWM7", 255}, {"ValveState", 2}};
  OutputAction no_output[]             = {};

  // free reward
  apod.ManualOverride('O', 'V', B00000001); // override valve
  delay(100);
  apod.ManualOverride('O', 'V', B00000000);

  for (int trial_num = 0; trial_num < MAX_TRIAL_NUM; trial_num++)
  {
    SerialUSB.print("Starting trial number: ");
    SerialUSB.println(trial_num + 1);

    // clear matrix at the begining of each trial
    apod.EmptyMatrix();

    States states[4] = {};
    if (TrialType == 0) {
      states[0] = apod.CreateState( "WaitForChoice",                                              // State Name
                                    0,                                                            // State Timer
                                    sizeof(WaitForChoice_Cond1) / sizeof(WaitForChoice_Cond1[0]), // Number of Conditions
                                    WaitForChoice_Cond1,                                          // State Change Conditions
                                    sizeof(no_output) / sizeof(no_output[0]),                    // Number of Outputs
                                    no_output);                                                    // Output Actions
    } else {
      states[0] = apod.CreateState( "WaitForChoice",                                               // State Name
                                    0,                                                            // State Timer
                                    sizeof(WaitForChoice_Cond2) / sizeof(WaitForChoice_Cond2[0]), // Number of Conditions
                                    WaitForChoice_Cond2,                                          // State Change Conditions
                                    sizeof(no_output) / sizeof(no_output[0]),                    // Number of Outputs
                                    no_output);                                                    // Output Actions
    }
    states[1] = apod.CreateState( "FlashPort1",                                               // State Name
                                  0.1,                                                        // State Timer
                                  sizeof(FlashPort1_Cond) / sizeof(FlashPort1_Cond[0]),       // Number of Conditions
                                  FlashPort1_Cond,                                            // State Change Conditions
                                  sizeof(FlashPort1_output) / sizeof(FlashPort1_output[0]),  // Number of Outputs
                                  FlashPort1_output);                                          // Output Actions

    states[2] = apod.CreateState( "FlashPort2",                                               // State Name
                                  0.1,                                                        // State Timer
                                  sizeof(FlashPort2_Cond) / sizeof(FlashPort2_Cond[0]),       // Number of Conditions
                                  FlashPort2_Cond,                                            // State Change Conditions
                                  sizeof(FlashPort2_output) / sizeof(FlashPort2_output[0]),  // Number of Outputs
                                  FlashPort2_output);                                         // Output Actions

    states[3] = apod.CreateState( "WaitForExit",                                          // State Name
                                  0,                                                      // State Timer
                                  sizeof(WaitForExit_Cond) / sizeof(WaitForExit_Cond[0]), // Number of Conditions
                                  WaitForExit_Cond,                                       // State Change Conditions
                                  sizeof(no_output) / sizeof(no_output[0]),              // Number of Outputs
                                  no_output);                                              // Output Actions

    // Predefine State sequence, i.e., 0-3
    for (int i = 0; i < 4; i++) {
      apod.AddBlankState(states[i].Name);
    }

    // Add a state to state machine.
    for (int i = 0; i < 4; i++) {
      apod.AddState(&states[i]);
    }

    // apod.PrintMatrix();  // for debug

    apod.SendStateMatrix();
    apod.RunStateMatrix(); // return immediately

    // Wait until receiving data from bpod (when a trial is done)
    while (apod.DataReceived() == 0) {}

    // receive all the events and states visited
    apod.ReceiveBpodData(); // todo: prevent stuck due to data loss
    /* data will be stored in public variable 'apod.trial_res', which includes:
       apod.trial_res.nEvents:           number of event happened in last trial
       apod.trial_res.eventTimeStamps[]: time stamps for each event
       apod.trial_res.Events[]:          event id for each event
       apod.trial_res.nTransition:       number of states visited in last trial
       apod.trial_res.state_visited[]:   the states visited in last trail
    */

    // Change parameters for next trial
    UpdateParameters();

  } // end for loop
}

void loop() {
  // put your main code here, to run repeatedly:
}

void UpdateParameters() {
  // update parameters for next trial

  // using trial_res to calculate e.g., performance, etc.
  SerialUSB.println(apod.trial_res.nTransition);

  if (random(100) < 50) {
    TrialType = 1;
  } else {
    TrialType = 0;
  }
}
