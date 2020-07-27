#include <Wire.h>
#include "Adafruit_VL6180X.h"

Adafruit_VL6180X vl = Adafruit_VL6180X();

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

#define CHANNEL_3_INDEX   6
#define PHASOR_INDEX      7
#define FLANGER_INDEX     8
#define CHORUS_INDEX      9
#define DELAY_INDEX       10
#define BOOST_INDEX       11

#define BLINK_COUNT       3000

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

bool effect_routes[6][6] = { 
  // CH3      PHASOR    FLAGNER   CHORUS    DELAY   BOOST
  {  false,   false,    false,    false,    false,  false}, // ROUTE 1, CH1
  {  false,   false,    false,    true,     false,  false}, // ROUTE 2, CH1 + CHORUS
  {  false,   false,    false,    false,    false,  false}, // ROUTE 3, CH2
  {  false,   false,    false,    true,     false,  false}, // ROUTE 4, CH2 + CHORUS
  {  false,   false,    false,    false,    true,   false}, // ROUTE 5, CH2 + DELAY
  {  false,   false,    false,    false,    true,   true},  // ROUTE 6, SOLO - CH2 + BOOST + DELAY
};

//                            CH1    CH1    CH2    CH2    CH2    SOLO
int  base_channels[6]     =  {0,     0,     1,     1,     1,     1   };
int  current_route        = 2;
bool editing_routes       = false;
int  original_route       = 2;
int  blink_count          = 0;

// *********************************************************
// MIDI
// *********************************************************
int evh_prog_chng           = 0xC1;
int eleven_rack_cntrl_chng  = 0xB0;

// *********************************************************
// ROUTINE: SendMIDI
// *********************************************************
void SendControlChange(int inCommand, int inControl, int inValue) {
  if (!editing_routes) {
    Serial1.write(inCommand);
    Serial1.write(inControl);
    Serial1.write(inValue);
  }
}

// *********************************************************
// ROUTINE: SendMIDI
// *********************************************************
void SendProgramChange(int inCommand, int inValue) {
  if (!editing_routes) {
    Serial1.write(inCommand);
    Serial1.write(inValue);
  }
}

// *********************************************************
// ROUTINE: PrintLEDStates
// *********************************************************
void PrintLEDStates() {
  Serial.print("LED States:\n");
  for (int index = 0; index < num_leds; ++index) {
    Serial.print("  led[");
    Serial.print(index);
    Serial.print("] = ");
    Serial.print(led_states[index]);
    Serial.print("\n");
  }
}

// *********************************************************
// ROUTINE: UpdateLEDs
// *********************************************************
void UpdateLEDs() {
  bool blink_off = false;
  if (editing_routes) {
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
    if (editing_routes && cur_led_row == 1 && index == (PHASOR_INDEX - 6)) {
      is_on = true;
    }

    int led_state = HIGH; // LED OFF
    if (is_on && !blink_off) {
      led_state = LOW; // LED ON
    }
    digitalWrite(led_pin, led_state);
  }

  cur_led_count++;
  if (cur_led_count > 150) {
    cur_led_count = 0;
    cur_led_row = (cur_led_row + 1) % num_led_rows;  
  }
}

// *********************************************************
// ROUTINE: TurnLEDsOff
// *********************************************************
void TurnLEDsOff() {
  for (int index = 0; index < num_led_pins; ++index) {
    int led_pin = led_pins[index];
    int led_state = HIGH; // LED OFF
    digitalWrite(led_pin, led_state);
  }
}

// *********************************************************
// ROUTINE: UpdateRouteChannel
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
// ROUTINE: SendEffect
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
      Serial.print("Phasor: ");
      control = 63;
      break;
    }
    case FLANGER_INDEX: {
      Serial.print("Flanger: ");
      control = 86;
      break;
    }
    case CHORUS_INDEX: {
      Serial.print("Chorus: ");
      control = 50;
      break;
    }
    case DELAY_INDEX: {
      Serial.print("Delay: ");
      control = 28;
      break;
    }
    case BOOST_INDEX: {
      Serial.print("Boost: ");
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

    Serial.print(led_states[inEffect]);
    Serial.print("\n");
  }
}

// *********************************************************
// ROUTINE: ToggleEffect
// *********************************************************
void ToggleEffect(int inEffect) {
  if (inEffect == PHASOR_INDEX) {
    if (editing_routes) {
      current_route = original_route;
      UpdateRoute();
      editing_routes = false;
    }
    else {
      original_route = current_route;
      editing_routes = true;
    }
  }
  else {
    int route_offset = inEffect - 6;
    effect_routes[current_route][route_offset] = !effect_routes[current_route][route_offset];
    UpdateEffect(inEffect);
  }
}

// *********************************************************
// ROUTINE: UpdateRoute
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
}

// *********************************************************
// ROUTINE: SwitchRoute
// *********************************************************
void SwitchRoute(int inRoute) {
  current_route = inRoute;
  UpdateRoute();
}

// *********************************************************
// ROUTINE: CheckSwitches
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
          Serial.print("\nSwitch Down: ");
          Serial.print(switch_state_index);
          Serial.print("\n");
        }
        switch_delay = 300;
        switch_states[switch_state_index] = value;
        if (value) {
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

  cur_switch_count++;
  if (cur_switch_count > 325) {
    cur_switch_count = 0;
    cur_switch_row = (cur_switch_row + 1) % num_switch_rows;  
  }

  if (switch_delay > 0) {
    switch_delay--;
  }
}

#define HYSTER_RANGE   4
#define MIN_EXPRESSION 24
#define MAX_EXPRESSION 40

const int expression_table[] =  {
  127, // 24
  116, // 25
  104, // 26
  92,  // 27
  80,  // 28
  68,  // 29
  56,  // 30
  44,  // 31
  
  32,  // 32 - Low volume
  27,  // 33
  22,  // 34
  17,  // 35
  12,  // 36
  7,   // 37
  3,   // 38
  1,   // 39
  0    // 40
};

int  expression_check = 0;
int  latest_value = -100;
bool going_up = false;
int  midi_value = 0;
int  sent_midi_value = -1;
int  sent_inc_amount = 15;
float value_range = (MAX_EXPRESSION - MIN_EXPRESSION);
bool  updating_expression_pedal = false;

// *********************************************************
// ROUTINE: UpdateSentExpressionValue
// *********************************************************
void UpdateSentExpressionValue() {  
  if (sent_midi_value != midi_value)  {  
    updating_expression_pedal = true;
    
    if (sent_midi_value == -1) { 
      sent_midi_value = midi_value;  
    }
    else if (sent_midi_value < midi_value) {
      sent_midi_value += sent_inc_amount;
      if (sent_midi_value > midi_value) {
        sent_midi_value = midi_value;
        updating_expression_pedal = false;
      }
    }
    else if (sent_midi_value > midi_value) {
      sent_midi_value -= sent_inc_amount;
      if (sent_midi_value < midi_value) {
        sent_midi_value = midi_value;
        updating_expression_pedal = false;
      }
    }
    
    SendControlChange(eleven_rack_cntrl_chng, 7, sent_midi_value);
  }
  else {
    updating_expression_pedal = false;
  }
}

// *********************************************************
// ROUTINE: CheckExpression
// *********************************************************
void CheckExpression() {  
  expression_check++;
  if (expression_check > 1000 || updating_expression_pedal) {
    expression_check = 0;
    
    uint8_t new_value = vl.readRange();
    uint8_t status = vl.readRangeStatus();
    if (status == VL6180X_ERROR_NONE) {
      bool handle_value = false;
      if (going_up) {
        if (new_value > latest_value) {
          handle_value = true;
        }
        else if (new_value < (latest_value - HYSTER_RANGE)) {
          handle_value = true;
          going_up = false;
        }
      }
      else { 
        if (new_value < latest_value) {
          handle_value = true;
        }
        else if (new_value > (latest_value + HYSTER_RANGE)) {
          handle_value = true;
          going_up = true;
        }
      }
      
      if (handle_value) {
        if (latest_value != new_value) {
          latest_value = new_value;
          int value_offset = latest_value;
          if (value_offset < MIN_EXPRESSION) {
            value_offset = MIN_EXPRESSION;
          }
          else if (value_offset > MAX_EXPRESSION) {
            value_offset = MAX_EXPRESSION;
          }
          value_offset -= MIN_EXPRESSION;
          midi_value = expression_table[value_offset];
        }

//         Serial.print("new_value: "); 
//         Serial.println(new_value);
//         Serial.print("midi_value: "); 
//         Serial.println(midi_value);
      }

      UpdateSentExpressionValue();
    }
  }

}

// *********************************************************
// ROUTINE: setup
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

  if (!vl.begin()) {
    Serial.println("Failed to find sensor");
    while (1);
  }


  UpdateRoute();
}

int volume_check = 0;

// *********************************************************
// ROUTINE: loop
// *********************************************************
void loop() {
  if (updating_expression_pedal) {
    TurnLEDsOff();
  }
  else {
    UpdateLEDs();
    CheckSwitches();
  }
  
  CheckExpression();
}
