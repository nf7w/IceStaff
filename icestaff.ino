#define PIN_R 12
#define PIN_G 13
#define PIN_B 14
#define PIN_FET 15
#define PIN_TILT 4

class loopMode {
public:
  struct limits {
    int minChannel;
    int maxChannel;
  };
  limits rgbLimits[3];
  int speed;
};

// 10 bit
constexpr loopMode modes[] = {
//  { 768, 1023, 768, 1023, 940, 1024, 1 },
  { 4, 16, 4, 16, 32, 48, 64}, // snooze
  { 128, 512, 128, 512, 768, 1023, 1}, // awake
};

void setup() {
  // RGB
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  // Spikes and shaft FET
  pinMode(PIN_FET, OUTPUT);
  // Tilt
  pinMode(PIN_TILT, INPUT_PULLUP);
  
  Serial.begin(9600);
  Serial.println("icestaff");
}

bool randomMove(bool up, int* rgb, const loopMode* curve) {
  int randomed[3];
  
  int rand3 = rand() % 3;
  int rand2 = rand() % 2;
  randomed[0] = rand3;
  switch(rand3) {
    case 0:
      randomed[1] = rand2 ? 1 : 2;
      randomed[2] = rand2 ? 2 : 1;
      break;
    case 1:
      randomed[1] = rand2 ? 0 : 2;
      randomed[2] = rand2 ? 2 : 0;
      break;
    case 2:
      randomed[1] = rand2 ? 0 : 1;
      randomed[2] = rand2 ? 1 : 0;
      break;
  }

  if (up) {
    if (rgb[randomed[0]] < curve->rgbLimits[randomed[0]].maxChannel) {
      ++rgb[randomed[0]];
    } else if (rgb[randomed[1]] < curve->rgbLimits[randomed[1]].maxChannel) {
      ++rgb[randomed[1]];
    } else if (rgb[randomed[2]] < curve->rgbLimits[randomed[2]].maxChannel) {
      ++rgb[randomed[2]];
    } else {
      return false;
    }
  } else {
    if (rgb[randomed[0]] > curve->rgbLimits[randomed[0]].minChannel) {
      --rgb[randomed[0]];
    } else if (rgb[randomed[1]] > curve->rgbLimits[randomed[1]].minChannel) {
      --rgb[randomed[1]];
    } else if (rgb[randomed[2]] > curve->rgbLimits[randomed[2]].minChannel) {
      --rgb[randomed[2]];
    } else {
      return false;
    }
  }
  return true;
}

bool debouncedUpright(bool* changed) {
  static bool lastRead = false;
  static int steadyCount = 0;
  static bool debounced = false;
  *changed = false;
  
  bool pin = digitalRead(PIN_TILT);
  if (pin == lastRead) {
    ++steadyCount;
  } else {
    lastRead = pin;
    steadyCount = 0;
  }
  if (steadyCount > 10 && pin != debounced) {
    debounced = pin;
    *changed = true;
    Serial.print(debounced ? "U" : "D");
  }
  return debounced;
}

void loop() {
  static int rgb[3] = {0};
  static bool up = true;
  bool changed = false;
  bool upright = debouncedUpright(&changed);
  digitalWrite(PIN_FET, upright);
  const loopMode* curve = &modes[upright];
  static bool fast = true;
  if (changed) {
    Serial.write("F");
    fast = true;
  }
  
//  analogWrite(PIN_R, 102);
//  analogWrite(PIN_G, 102);
//  analogWrite(PIN_B, 1023);

  if (!randomMove(up, rgb, curve)) {
    up = !up;
    fast = false;
    Serial.write("S");
  }

  analogWrite(PIN_R, rgb[0]);
  analogWrite(PIN_G, rgb[1]);
  analogWrite(PIN_B, rgb[2]);

  int transitionDivisor = fast ? 10 : 1;
  delay(curve->speed / transitionDivisor);
}
