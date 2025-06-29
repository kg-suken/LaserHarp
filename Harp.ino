#include <MIDIUSB.h>

// ====== 設定しきい値 ======
const int sensorThreshold = 50; // フォトトランジスタの発動しきい値
const int octaveShiftThresholds[] = { 100, 300, 650, 740 }; // -2, -1, 0, 1, 2 の切り替え境界値

// ====== ハードウェア構成 ======
const int analogPins[] = { A0, A1, A2, A3, A4, A5 };
const byte midiNotes[] = { 60, 62, 64, 65, 67, 69 };  // ド, レ, ミ, ファ, ソ, ラ
bool noteState[6] = { false, false, false, false, false, false };

const int buttonPin = 13;
int currentChannel = 0;
int previousChannel = 0;
bool lastButtonState = HIGH;

const int ledPins[] = { 2, 3, 4, 5, 6, 7, 8 };
const int patterns[10][7] = {
  { 1, 1, 1, 1, 0, 1, 1 },
  { 1, 1, 0, 0, 0, 0, 0 },
  { 1, 0, 1, 0, 1, 1, 1 },
  { 1, 1, 1, 0, 1, 0, 1 },
  { 1, 1, 0, 1, 1, 0, 0 },
  { 0, 1, 1, 1, 1, 0, 1 },
  { 0, 1, 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 0, 0, 0 },
  { 1, 1, 1, 1, 1, 1, 1 },
  { 1, 1, 1, 1, 1, 0, 1 }
};

int currentOctaveShift = 0;
int previousOctaveShift = 0;

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 7; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
  pinMode(buttonPin, INPUT_PULLUP);
  displayNumber(currentChannel);
  previousOctaveShift = getOctaveShift();  // 初期値設定
}

void loop() {
  handleButton();
  handleSensors();
  printSensorValues();
  delay(20);
}

void handleButton() {
  bool currentButtonState = digitalRead(buttonPin);
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    allNotesOff(previousChannel, currentOctaveShift);
    currentChannel = (currentChannel + 1) % 10;
    displayNumber(currentChannel);
    Serial.print("チャンネル変更: ");
    Serial.println(currentChannel + 1);
    previousChannel = currentChannel;
    delay(200);
  }
  lastButtonState = currentButtonState;
}

void handleSensors() {
  currentOctaveShift = getOctaveShift();

  if (currentOctaveShift != previousOctaveShift) {
    allNotesOff(currentChannel, previousOctaveShift);
    previousOctaveShift = currentOctaveShift;
    Serial.print("オクターブ変更: ");
    Serial.println(currentOctaveShift);
  }

  for (int i = 0; i < 6; i++) {
    int val = analogRead(analogPins[i]);
    byte shiftedNote = midiNotes[i] + currentOctaveShift * 12;
    shiftedNote = constrain(shiftedNote, 0, 127);

    if (val < sensorThreshold && !noteState[i]) {
      sendNoteOn(currentChannel + 1, shiftedNote, 127);
      noteState[i] = true;
    } else if (val >= sensorThreshold && noteState[i]) {
      sendNoteOff(currentChannel + 1, shiftedNote, 127);
      noteState[i] = false;
    }
  }
}

int getOctaveShift() {
  int val = analogRead(A11);
  if (val > octaveShiftThresholds[3]) return 2;
  else if (val > octaveShiftThresholds[2]) return 1;
  else if (val > octaveShiftThresholds[1]) return 0;
  else if (val > octaveShiftThresholds[0]) return -1;
  else return -2;
}

void allNotesOff(int channel, int octaveShift) {
  for (int i = 0; i < 6; i++) {
    byte shiftedNote = midiNotes[i] + octaveShift * 12;
    shiftedNote = constrain(shiftedNote, 0, 127);
    if (noteState[i]) {
      sendNoteOff(channel + 1, shiftedNote, 0);
      noteState[i] = false;
    }
  }
}

void printSensorValues() {
  Serial.print("Sensors: ");
  for (int i = 0; i < 6; i++) {
    Serial.print("A");
    Serial.print(i);
    Serial.print("=");
    Serial.print(analogRead(analogPins[i]));
    Serial.print(", ");
  }
  Serial.print("A11=");
  Serial.println(analogRead(A11));
}

void sendNoteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = { 0x09, byte(0x90 | ((channel - 1) & 0x0F)), pitch, velocity };
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}

void sendNoteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = { 0x08, byte(0x80 | ((channel - 1) & 0x0F)), pitch, velocity };
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}

void displayNumber(int num) {
  for (int i = 0; i < 7; i++) {
    digitalWrite(ledPins[i], patterns[num][i] ? HIGH : LOW);
  }
}
