#define A_IN1 19
#define A_IN2 18
#define A_IN3 5
#define A_IN4 17

#define B_IN1 16
#define B_IN2 4
#define B_IN3 2
#define B_IN4 15

#define Z_IN1 13
#define Z_IN2 12
#define Z_IN3 14
#define Z_IN4 27

#define STEPS_PER_MM   50bv
#define STEP_DELAY_US  1200
#define Z_STEPS_DOWN   50
#define Z_STEPS_UP     50

const int stepSequence[8][4] = {
  {1, 0, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 0},
  {0, 0, 1, 0}, {0, 0, 1, 1}, {0, 0, 0, 1}, {1, 0, 0, 1}
};

int stepA = 0, stepB = 0, stepZ = 0;
float currentX = 0, currentY = 0;
String inputBuffer = "";

void setMotorA(int s) {
  s = ((s % 8) + 8) % 8;
  digitalWrite(A_IN1, stepSequence[s][0]); digitalWrite(A_IN2, stepSequence[s][1]);
  digitalWrite(A_IN3, stepSequence[s][2]); digitalWrite(A_IN4, stepSequence[s][3]);
}
void setMotorB(int s) {
  s = ((s % 8) + 8) % 8;
  digitalWrite(B_IN1, stepSequence[s][0]); digitalWrite(B_IN2, stepSequence[s][1]);
  digitalWrite(B_IN3, stepSequence[s][2]); digitalWrite(B_IN4, stepSequence[s][3]);
}
void setMotorZ(int s) {
  s = ((s % 8) + 8) % 8;
  digitalWrite(Z_IN1, stepSequence[s][0]); digitalWrite(Z_IN2, stepSequence[s][1]);
  digitalWrite(Z_IN3, stepSequence[s][2]); digitalWrite(Z_IN4, stepSequence[s][3]);
}
void motorAOff() {
  digitalWrite(A_IN1,0); digitalWrite(A_IN2,0);
  digitalWrite(A_IN3,0); digitalWrite(A_IN4,0);
}
void motorBOff() {
  digitalWrite(B_IN1,0); digitalWrite(B_IN2,0);
  digitalWrite(B_IN3,0); digitalWrite(B_IN4,0);
}
void motorZOff() {
  digitalWrite(Z_IN1,0); digitalWrite(Z_IN2,0);
  digitalWrite(Z_IN3,0); digitalWrite(Z_IN4,0);
}

void moveCoreXY(int stepsX, int stepsY) {
  int stepsA = stepsX + stepsY;
  int stepsB = stepsX - stepsY;
  int dirA = (stepsA >= 0) ? 1 : -1;
  int dirB = (stepsB >= 0) ? 1 : -1;
  int absA = abs(stepsA), absB = abs(stepsB);
  int maxSteps = max(absA, absB);
  for (int i = 0; i < maxSteps; i++) {
    if (i < absA) { stepA += dirA; setMotorA(stepA); }
    if (i < absB) { stepB += dirB; setMotorB(stepB); }
    delayMicroseconds(STEP_DELAY_US);
  }
  motorAOff(); motorBOff();
}

void moveTo(float tx, float ty) {
  int sx = (int)((tx - currentX) * STEPS_PER_MM);
  int sy = (int)((ty - currentY) * STEPS_PER_MM);
  moveCoreXY(sx, sy);
  currentX = tx; currentY = ty;
}

void penUp() {
  for (int i = 0; i < Z_STEPS_UP; i++) {
    stepZ--; setMotorZ(stepZ);
    delayMicroseconds(STEP_DELAY_US);
  }
  motorZOff();
}

void penDown() {
  for (int i = 0; i < Z_STEPS_DOWN; i++) {
    stepZ++; setMotorZ(stepZ);
    delayMicroseconds(STEP_DELAY_US);
  }
  motorZOff();
}

float parseValue(String line, char letter) {
  int idx = line.indexOf(letter);
  if (idx == -1) return -99999;
  return line.substring(idx + 1).toFloat();
}

void processGCode(String line) {
  line.trim();
  int ci = line.indexOf(';');
  if (ci >= 0) line = line.substring(0, ci);
  line.trim();

  Serial.print("Recibido: ");
  Serial.println(line);  

  if (line.length() == 0) {
    Serial.println("ok");
    return;
  }
  line.toUpperCase();

  String cmd = "";
  for (int i = 0; i < (int)line.length(); i++) {
    if (line[i] == 'G' || line[i] == 'M') {
      int j = i + 1;
      while (j < (int)line.length() && (isDigit(line[j]) || line[j] == '.')) j++;
      cmd = line.substring(i, j);
      break;
    }
  }

  Serial.print("Comando: ");
  Serial.println(cmd);  

  float xVal = parseValue(line, 'X');
  float yVal = parseValue(line, 'Y');

  if (cmd == "G0" || cmd == "G1") {
    float tx = (xVal != -99999) ? xVal : currentX;
    float ty = (yVal != -99999) ? yVal : currentY;
    moveTo(tx, ty);
  } else if (cmd == "G28") {
    penUp(); moveTo(0, 0);
  } else if (cmd == "M3") {
    penDown();
  } else if (cmd == "M5") {
    penUp();
  } else {
    Serial.print("Comando no reconocido: ");
    Serial.println(cmd);
  }

  Serial.println("ok");
}

void setup() {
  delay(2000);  
  Serial.begin(115200);
  while (!Serial) { delay(10); }  

  pinMode(A_IN1,OUTPUT); pinMode(A_IN2,OUTPUT);
  pinMode(A_IN3,OUTPUT); pinMode(A_IN4,OUTPUT);
  pinMode(B_IN1,OUTPUT); pinMode(B_IN2,OUTPUT);
  pinMode(B_IN3,OUTPUT); pinMode(B_IN4,OUTPUT);
  pinMode(Z_IN1,OUTPUT); pinMode(Z_IN2,OUTPUT);
  pinMode(Z_IN3,OUTPUT); pinMode(Z_IN4,OUTPUT);

  penUp();
  Serial.println("Plotter listo. Esperando G-code...");
}

void loop() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        processGCode(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
}
