
// *********************************************************
// LEDs
// *********************************************************
int led_rows[] = {24, 22};
int led_pins[] = {26, 28, 30, 32, 34, 36};
bool led_states[] = {
  false, false, false, false, false, false, // Routes
  false, false, false, false, false, false  // Effects
};
int num_led_rows = 2;
int num_led_pins = 6;
int num_leds = num_led_rows * num_led_pins;
int cur_led_row = 0;
int cur_led_count = 0;

#define WAH_LED_PIN       43

#define CHANNEL_3_INDEX   6
#define PHASOR_INDEX      7
#define FLANGER_INDEX     8
#define CHORUS_INDEX      9
#define DELAY_INDEX       10
#define BOOST_INDEX       11
#define WAH_INDEX         12

#define BLINK_COUNT       1800

// *********************************************************
// Switches
// *********************************************************
int switch_rows[] = {38, 40};
int switch_pins[] = {52, 50, 48, 46, 44, 42};
bool switch_states[] = {
  false, false, false, false, false, false,
  false, false, false, false, false, false
};
int num_switch_rows = 2;
int num_switch_pins = 6;
int cur_switch_row = 0;
int cur_switch_count = 0;
int switch_delay = 0;

// *********************************************************
// Routes
// *********************************************************

bool effect_routes[6][7] = { 
  // CH3      PHASOR    FLAGNER   CHORUS    DELAY   BOOST   WAH
  {  false,   false,    false,    false,    false,  false,  false}, // ROUTE 1, CH1
  {  false,   false,    false,    true,     false,  false,  false}, // ROUTE 2, CH1 + CHORUS
  {  false,   false,    false,    false,    false,  false,  false}, // ROUTE 3, CH2
  {  false,   false,    false,    true,     false,  false,  false}, // ROUTE 4, CH2 + CHORUS
  {  false,   false,    false,    false,    true,   false,  false}, // ROUTE 5, CH2 + DELAY
  {  false,   false,    false,    false,    true,   true,   false}, // ROUTE 6, SOLO - CH2 + BOOST + DELAY
};

//                            CH1    CH1    CH2    CH2    CH2    SOLO
int  base_channels[6]     =  {0,     0,     1,     1,     1,     1   };
int  current_route        = 2;
bool editing_routes       = false;
int  original_route       = 2;
int  blink_count          = 0;
bool tuner_state          = false;

// *********************************************************
// MIDI
// *********************************************************
int evh_prog_chng           = 0xC1;
int eleven_rack_cntrl_chng  = 0xB0;

// *********************************************************
// ROUTINE
// *********************************************************
bool IsWahOn() {
  return effect_routes[current_route][6];
}

// *********************************************************
// ROUTINE
// *********************************************************
bool IsTuning() {
  return tuner_state;
}

// *********************************************************
// ROUTINE
// *********************************************************
bool IsEditing() {
  return editing_routes;
}

// *********************************************************
// ROUTINE
// *********************************************************
void SendControlChange(int inCommand, int inControl, int inValue) {
  if (!IsEditing()) {
    Serial1.write(inCommand);
    Serial1.write(inControl);
    Serial1.write(inValue);
  }
}

// *********************************************************
// ROUTINE
// *********************************************************
void SendProgramChange(int inCommand, int inValue) {
  if (!IsEditing()) {
    Serial1.write(inCommand);
    Serial1.write(inValue);

//    Serial.print("ProgramChange:");
//    Serial.print(inValue);
//    Serial.print("\n");
  }
}

// *********************************************************
// ROUTINE
// *********************************************************
void PrintLEDStates() {
//  Serial.print("LED States:\n");
//  for (int index = 0; index < num_leds; ++index) {
//    Serial.print("  led[");
//    Serial.print(index);
//    Serial.print("] = ");
//    Serial.print(led_states[index]);
//    Serial.print("\n");
//  }
}

// *********************************************************
// ROUTINE
// *********************************************************
void UpdateLEDs() {
  bool all_on = false;
  bool blink_off = false;
  if (IsTuning()) {
    all_on = true;
  }
  else if (IsEditing()) {
    blink_count++;
    if (blink_count > (2*BLINK_COUNT)) {
      blink_count = 0;
    }
    else if (blink_count > BLINK_COUNT)  {
      blink_off = true;
    }
  }

  for (int index = 0; index < num_led_rows; ++index) {
    int led_row_pin = led_rows[index];
    if (index == cur_led_row) {
      digitalWrite(led_row_pin, HIGH);
    }
    else {
      digitalWrite(led_row_pin, LOW);
    }
  }

//  Serial.print("cur_led_row: ");
//  Serial.print(cur_led_row);
//  Serial.print("\n");

  for (int index = 0; index < num_led_pins; ++index) {
    int led_pin = led_pins[index];
    int led_state_index = index + (cur_led_row * num_led_pins);
    bool is_on = led_states[led_state_index];
    int led_state = HIGH; // LED OFF
    if (all_on) {
      led_state = LOW; // LED ON
    }
    else if (is_on && !blink_off) {
      led_state = LOW; // LED ON
    }
    digitalWrite(led_pin, led_state);
  }

  bool wah_led_state = IsWahOn();
  if (all_on) {
    wah_led_state = HIGH;
  }
  else if (blink_off) {
    wah_led_state = LOW;
  }
  digitalWrite(WAH_LED_PIN, wah_led_state);

  cur_led_count++;
  if (cur_led_count > 50) {
    cur_led_count = 0;
    cur_led_row = (cur_led_row + 1) % num_led_rows;  
  }
}

// *********************************************************
// ROUTINE
// *********************************************************
void TurnLEDsOff() {
  for (int index = 0; index < num_led_pins; ++index) {
    int led_pin = led_pins[index];
    int led_state = HIGH; // LED OFF
    digitalWrite(led_pin, led_state);
  }
}

// *********************************************************
// ROUTINE
// *********************************************************
void UpdateRouteChannel() {
  // Send the channel
  int channel_num = base_channels[current_route];
  if (effect_routes[current_route][0]) {
    channel_num = 2;
  }
  SendProgramChange(evh_prog_chng , channel_num);

  // Update the Route LEDs
  for (int route = 0; route < 6; ++route) {
    led_states[route] = (route == current_route);
  }
  led_states[CHANNEL_3_INDEX] = effect_routes[current_route][0];
}

// *********************************************************
// ROUTINE
// *********************************************************
void UpdateEffect(int inEffect) {
  int control = -1;
  int route_offset = inEffect - 6;
  switch (inEffect) {
    case CHANNEL_3_INDEX: {
      UpdateRouteChannel();
      return;
    }
    case PHASOR_INDEX: {
//      Serial.print("Phasor: ");
      control = 63;
      break;
    }
    case FLANGER_INDEX: {
//      Serial.print("Flanger: ");
      control = 86;
      break;
    }
    case CHORUS_INDEX: {
//      Serial.print("Chorus: ");
      control = 50;
      break;
    }
    case DELAY_INDEX: {
//      Serial.print("Delay: ");
      control = 28;
      break;
    }
    case BOOST_INDEX: {
//      Serial.print("Boost: ");
      control = 25;
      break;
    }
  }

  if (control >= 0) {
    int value = 0;
    led_states[inEffect] = effect_routes[current_route][route_offset];
    if (led_states[inEffect]) {
      value = 127;
    }
    SendControlChange(eleven_rack_cntrl_chng, control, value);

//    Serial.print(led_states[inEffect]);
//    Serial.print("\n");
  }
}

// *********************************************************
// ROUTINE
// *********************************************************
void ToggleEffect(int inEffect) {
    int route_offset = inEffect - 6;
    effect_routes[current_route][route_offset] = !effect_routes[current_route][route_offset];
    UpdateEffect(inEffect);
}

// *********************************************************
// ROUTINE
// *********************************************************
void UpdateRoute() {
  // Send the channel
  UpdateRouteChannel();

  // Update the effects
  UpdateEffect(PHASOR_INDEX);
  UpdateEffect(FLANGER_INDEX);
  UpdateEffect(CHORUS_INDEX);
  UpdateEffect(DELAY_INDEX);
  UpdateEffect(BOOST_INDEX);
  delay(10);
  UpdateWah();
}

// *********************************************************
// ROUTINE
// *********************************************************
void SwitchRoute(int inRoute) {
  current_route = inRoute;
  UpdateRoute();
}

// *********************************************************
// ROUTINE
// *********************************************************
void UpdateWah() {
  int value = 0;
  if (IsWahOn()) {
    value = 127;
  }
  SendControlChange(eleven_rack_cntrl_chng, 4, 127);
  SendControlChange(eleven_rack_cntrl_chng, 7, 127);
  SendControlChange(eleven_rack_cntrl_chng, 43, value);
}

// *********************************************************
// ROUTINE
// *********************************************************
void ToggleWah() {
  effect_routes[current_route][6] = !effect_routes[current_route][6];
  UpdateWah();
}

// *********************************************************
// ROUTINE
// *********************************************************
void ToggleProg() {
  if (IsEditing()) {
    current_route = original_route;
    UpdateRoute();
    editing_routes = false;
  }
  else {
    original_route = current_route;
    editing_routes = true;
  }
}

// *********************************************************
// ROUTINE
// *********************************************************
void ToggleTuner() {
  tuner_state = !tuner_state;
  int value = 0;
  if (IsTuning()) {
    value = 127;
  }
  SendControlChange(eleven_rack_cntrl_chng, 69, value);
}            

// *********************************************************
// ROUTINE
// *********************************************************
void CheckSwitches() {  
  for (int index = 0; index < num_switch_rows; ++index) {
    int switch_row_pin = switch_rows[index];
    if (index == cur_switch_row) {
      digitalWrite(switch_row_pin, HIGH);
    }
    else {
      digitalWrite(switch_row_pin, LOW);
    }
  }

  for (int index = 0; index < num_switch_pins; ++index) {
    int switch_pin = switch_pins[index];   
    bool value = digitalRead(switch_pin) == 1;
    if (switch_delay == 0) {
      int switch_state_index = index + (cur_switch_row * num_switch_pins);
      if (value != switch_states[switch_state_index]) {
        if (value) {
//          Serial.print("\nSwitch Down: ");
//          Serial.print(switch_state_index);
//          Serial.print("\n");
        }
        switch_delay = 300;
        switch_states[switch_state_index] = value;
        if (value) {
          if (IsTuning()) {
            ToggleTuner();
          }
          else {
            if (switch_state_index >= 0 && switch_state_index <= 5) {
              SwitchRoute(switch_state_index);
            }
            if (switch_state_index >= 6 && switch_state_index <= 11) {
              ToggleEffect(switch_state_index);
            }
          }
        }
      }
    }
  }

  cur_switch_count++;
  if (cur_switch_count > 325) {
    cur_switch_count = 0;
    cur_switch_row = (cur_switch_row + 1) % num_switch_rows;  
  }

  if (switch_delay > 0) {
    switch_delay--;
  }
}

int last_midi_volume = -1;
int vol_update_count = 0;

// *********************************************************
// ROUTINE
// *********************************************************
void CheckExpression() {
  vol_update_count++;
  if (vol_update_count > 20) { 
    vol_update_count = 0;
    
    float ratio = 128.0/1024.0;
    float new_value = analogRead(1) * ratio;
    int   midi_volume = new_value;
    if (midi_volume > 127) {
      midi_volume = 127;
    }
    else if (midi_volume < 0) {
      midi_volume = 0;
    }

    if (midi_volume != last_midi_volume) {
      last_midi_volume = midi_volume;
      if (IsWahOn()) {
        SendControlChange(eleven_rack_cntrl_chng, 4, midi_volume);
      }
      else {
        SendControlChange(eleven_rack_cntrl_chng, 7, midi_volume);
      }
    }
  
//  Serial.print("midi_volume: "); 
//  Serial.println(midi_volume);
  }
}
#define TOLERANCE_AMOUNT  5
#define SWITCH_ANALOG_IN  0
#define MIDI_NOTE_OFFSET  35
#define MAX_COUNT_TRIGGER 200
#define BUTTON_ARRAY_SIZE 14

int read_values[BUTTON_ARRAY_SIZE] = {
    464,  // 
    486,  // 
    512,  // 
    537,  // 
    568,  // 
    603,  // 
    640,  // 
    683,  // 
    730,  // 
    787,  // TUNER - 9
    850,  // PROG - 10
    930,  //
    1024, // WAH -12
    0
};

#define TUNER_SWITCH_INDEX     9
#define PROG_SWITCH_INDEX      10
#define WAH_SWITCH_INDEX       12
 
// *************************************************************
int GetArrayIndex(int inAnalogValue) {
  int last_value = 0;
  int index = 0;
  for (; index < (BUTTON_ARRAY_SIZE-1); index++) {
    int cur_value = read_values[index];
    int lower_mid = (last_value + cur_value)/2;
    if (inAnalogValue < lower_mid) {
      return index-1;
    }
    last_value = cur_value;
  }
  return index-1;
}

int count_array[BUTTON_ARRAY_SIZE];

// *************************************************************
void AddArrayCount(int inIndex) {
  if (inIndex >= 0 && inIndex < BUTTON_ARRAY_SIZE) {
    count_array[inIndex]++;
  }
}

// *************************************************************
int GetArrayCount(int inIndex) {
  if (inIndex >= 0 && inIndex < BUTTON_ARRAY_SIZE) {
    return count_array[inIndex];
  }
  return 0;
}

// *************************************************************
void PrintCountArray() {
  for (int index = 0; index < BUTTON_ARRAY_SIZE; ++index) {
    int count = count_array[index];
    if (count > 0) {
      Serial.print("count[");
      Serial.print(index);
      Serial.print("] = ");
      Serial.print(count);
      Serial.print("\n");
    }
  }
}

// *************************************************************
void ClearCountArray() {
  for (int index = 0; index < BUTTON_ARRAY_SIZE; ++index) {
    count_array[index] = 0;
  }
}

int last_array_index = -1;
int max_array_index = 0;
int last_switch = 0;

// *********************************************************
// ROUTINE
// *********************************************************
void CheckNewSwitches() {
  int analog_value = analogRead(3);
//  Serial.print(analog_value);
//  Serial.print("\n"); 
  
  int cur_array_index = GetArrayIndex(analog_value);
  if (cur_array_index > max_array_index) {
    max_array_index = cur_array_index;
  }

  if (cur_array_index < 0) {
    if (cur_array_index != last_array_index) {      
      max_array_index = -1;
      ClearCountArray();
      last_switch = 0;
    }
  }
  else {
    if (last_switch == 0) {
      AddArrayCount(cur_array_index);
      int max_count = GetArrayCount(max_array_index);
      if (max_count > MAX_COUNT_TRIGGER) {
        if (max_array_index != last_switch) {
//          Serial.print(max_array_index);
//          Serial.print("\n"); 
          last_switch = max_array_index;
          if (IsTuning()) {
            ToggleTuner();
          }
          else if (last_switch == WAH_SWITCH_INDEX) {
            ToggleWah();
          }
          else if (last_switch == PROG_SWITCH_INDEX) {
            ToggleProg();
          }
          else if (last_switch == TUNER_SWITCH_INDEX) {
            ToggleTuner();
          }
        }
      }
    }
  }
  last_array_index = cur_array_index;
}

// *********************************************************
// ROUTINE
// *********************************************************
void setup() {
  Serial.begin(115200);
  Serial1.begin(31250);

  for (int index = 0; index < num_led_rows; ++index) {
    int led_row_pin = led_rows[index];
    pinMode(led_row_pin, OUTPUT);
  }
  
  for (int index = 0; index < num_led_pins; ++index) {
    int led_pin = led_pins[index];
    pinMode(led_pin, OUTPUT);
  }

  for (int index = 0; index < num_switch_rows; ++index) {
    int switch_row_pin = switch_rows[index];
    pinMode(switch_row_pin , OUTPUT);
  }
  
  for (int index = 0; index < num_switch_pins; ++index) {
    int switch_pin = switch_pins[index];
    pinMode(switch_pin, INPUT);
  }

  pinMode(WAH_LED_PIN , OUTPUT);
  digitalWrite(43, HIGH);

  UpdateRoute();
}

// *********************************************************
// ROUTINE
// *********************************************************
void loop() {
  UpdateLEDs();
  CheckSwitches();
  CheckExpression();
  CheckNewSwitches();
}
