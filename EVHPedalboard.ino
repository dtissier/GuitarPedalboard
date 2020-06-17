

// *********************************************************
// LEDs
// *********************************************************
int led_rows[] = {24, 22};
int led_pins[] = {26, 28, 30, 32, 34, 36};
bool led_states[] = {
  false, false, false, false, false, false,
  false, false, false, false, false, false
};
int num_led_rows = 2;
int num_led_pins = 6;
int num_leds = num_led_rows * num_led_pins;
int cur_led_row = 0;
int cur_led_count = 0;

int index_6          = 0 + num_led_pins;
int index_7          = 1 + num_led_pins;
int flanger_index    = 2 + num_led_pins;
int chorus_index     = 3 + num_led_pins;
int delay_index      = 4 + num_led_pins;
int boost_index      = 5 + num_led_pins;

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
// ROUTINE: SendEffect
// *********************************************************
void SendEffect(int inEffect) {
  switch (inEffect) {
    case 6: {
      Serial.print("6: ");
      break;
    }
    case 7: {
      Serial.print("7: ");
      break;
    }
    case 8: {
      Serial.print("Flanger: ");
      break;
    }
    case 9: {
      Serial.print("Chorus: ");
      break;
    }
    case 10: {
      Serial.print("Delay: ");
      break;
    }
    case 11: {
      Serial.print("Boost: ");
      break;
    }
  }
  Serial.print(led_states[inEffect]);
  Serial.print("\n");
}

// *********************************************************
// ROUTINE: UpdateLEDs
// *********************************************************
void UpdateLEDs() {
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
    int led_state = HIGH;
    if (led_states[led_state_index]) {
      led_state = LOW;
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
// ROUTINE: SwitchPatch
// *********************************************************
void SwitchPatch(int inPatch) {
  for (int index = 0; index < num_leds; ++index) {
    led_states[index] = false;
  }

  led_states[inPatch] = true;

  Serial.print("\n");
  switch (inPatch) {
    case 0: {
      Serial.print("Clean\n");
      break;
    }
    case 1: {
      Serial.print("Clean + Chorus\n");
      led_states[chorus_index] = true;
      break;
    }
    case 2: {
      Serial.print("Crunch\n");
      break;
    }
    case 3: {
      Serial.print("Crunch + Chorus\n");
      led_states[chorus_index] = true;
      break;
    }
    case 4: {
      Serial.print("Dirty\n");
      break;
    }
    case 5: {
      Serial.print("Dirty + Delay\n");
      led_states[delay_index] = true;
      break;
    }
  }

//  PrintLEDStates();
  for (int index = 0; index < num_led_pins; ++index) {
    SendEffect(index + num_led_pins);
  }
}

// *********************************************************
// ROUTINE: ToggleEffect
// *********************************************************
void ToggleEffect(int inEffect) {
//  switch (inEffect) {
//    case 6: {
//      Serial.print("Toggle 6\n");
//      break;
//    }
//    case 7: {
//      Serial.print("Toggle 7\n");
//      break;
//    }
//    case 8: {
//      Serial.print("Toggle Flanger\n");
//      break;
//    }
//    case 9: {
//      Serial.print("Toggle Chorus\n");
//      break;
//    }
//    case 10: {
//      Serial.print("Toggle Delay\n");
//      break;
//    }
//    case 11: {
//      Serial.print("Toggle Boost\n");
//      break;
//    }
//  }

  Serial.print("\nToggle ");
  led_states[inEffect] = !led_states[inEffect];
  SendEffect(inEffect);
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
            SwitchPatch(switch_state_index);
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

// *********************************************************
// ROUTINE: setup
// *********************************************************
void setup() {
  Serial.begin(9600);

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
}

// *********************************************************
// ROUTINE: loop
// *********************************************************
void loop() {
//  Serial.print("There\n");
  UpdateLEDs();
  CheckSwitches();
//  delay(500);
}
