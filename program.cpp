#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

// -------------------- Components --------------------
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pins
#define BUZZER_PIN 25

#define BUTTON_TAKEN 32
#define BUTTON_SNOOZE 33
#define BUTTON_TIME 34

// -------------------- Medicine Schedule --------------------
// Change these times according to your requirement
int medicineHour[]   = {8, 14, 20};
int medicineMinute[] = {0, 0, 0};

const int totalMedicines = 3;

// -------------------- Variables --------------------
bool alarmActive = false;
bool medicineTaken = false;
bool snoozed = false;

int currentMedicine = -1;

unsigned long alarmStartTime = 0;
unsigned long snoozeStartTime = 0;

const unsigned long ALARM_DURATION = 30000; // 30 seconds
const unsigned long SNOOZE_TIME = 60000;   // 1 minute

// Prevent repeated triggering in same minute
int lastTriggeredMinute = -1;
int lastTriggeredDay = -1;


// ====================================================
// SETUP
// ====================================================
void setup() {

  Serial.begin(115200);

  // I2C
  Wire.begin(21, 22);

  // LCD
  lcd.init();
  lcd.backlight();

  // Buttons
  pinMode(BUTTON_TAKEN, INPUT_PULLUP);
  pinMode(BUTTON_SNOOZE, INPUT_PULLUP);
  pinMode(BUTTON_TIME, INPUT_PULLUP);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // RTC
  if (!rtc.begin()) {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RTC Not Found!");

    Serial.println("DS3231 RTC not found!");

    while (1);
  }

  // Uncomment this ONCE to set RTC to compilation time
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Medicine");
  lcd.setCursor(0, 1);
  lcd.print("Reminder System");

  delay(2000);

  lcd.clear();
}


// ====================================================
// LOOP
// ====================================================
void loop() {

  DateTime now = rtc.now();

  // Check medicine schedule
  checkMedicineTime(now);

  // Handle active alarm
  if (alarmActive) {

    handleAlarm();

  } else {

    // Normal display
    displayCurrentTime(now);

    // Check buttons
    checkButtons();
  }

  delay(100);
}


// ====================================================
// CHECK MEDICINE TIME
// ====================================================
void checkMedicineTime(DateTime now) {

  for (int i = 0; i < totalMedicines; i++) {

    if (now.hour() == medicineHour[i] &&
        now.minute() == medicineMinute[i]) {

      // Prevent multiple triggers
      if (lastTriggeredMinute == now.minute() &&
          lastTriggeredDay == now.day()) {
        return;
      }

      lastTriggeredMinute = now.minute();
      lastTriggeredDay = now.day();

      currentMedicine = i;

      alarmActive = true;
      medicineTaken = false;
      snoozed = false;

      alarmStartTime = millis();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MEDICINE TIME!");

      lcd.setCursor(0, 1);
      lcd.print("Dose ");
      lcd.print(i + 1);

      Serial.print("Medicine reminder: Dose ");
      Serial.println(i + 1);

      digitalWrite(BUZZER_PIN, HIGH);

      return;
    }
  }
}


// ====================================================
// HANDLE ALARM
// ====================================================
void handleAlarm() {

  // Check TAKE button
  if (digitalRead(BUTTON_TAKEN) == LOW) {

    medicineTaken = true;
    alarmActive = false;

    digitalWrite(BUZZER_PIN, LOW);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Medicine Taken");

    lcd.setCursor(0, 1);
    lcd.print("Dose ");
    lcd.print(currentMedicine + 1);

    Serial.println("Medicine taken.");

    delay(2000);

    return;
  }


  // Check SNOOZE button
  if (digitalRead(BUTTON_SNOOZE) == LOW) {

    snoozed = true;
    alarmActive = false;

    digitalWrite(BUZZER_PIN, LOW);

    snoozeStartTime = millis();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Reminder");
    lcd.setCursor(0, 1);
    lcd.print("Snoozed");

    Serial.println("Medicine reminder snoozed.");

    delay(2000);

    return;
  }


  // Alarm automatically stops after 30 seconds
  if (millis() - alarmStartTime >= ALARM_DURATION) {

    digitalWrite(BUZZER_PIN, LOW);

    alarmActive = false;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dose Missed!");

    lcd.setCursor(0, 1);
    lcd.print("Dose ");
    lcd.print(currentMedicine + 1);

    Serial.println("Medicine dose missed.");

    delay(3000);
  }
}


// ====================================================
// CHECK BUTTONS
// ====================================================
void checkButtons() {

  // Time button
  if (digitalRead(BUTTON_TIME) == LOW) {

    DateTime now = rtc.now();

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Current Time:");

    lcd.setCursor(0, 1);

    if (now.hour() < 10)
      lcd.print("0");

    lcd.print(now.hour());
    lcd.print(":");

    if (now.minute() < 10)
      lcd.print("0");

    lcd.print(now.minute());
    lcd.print(":");

    if (now.second() < 10)
      lcd.print("0");

    lcd.print(now.second());

    delay(2000);
  }
}


// ====================================================
// DISPLAY CURRENT TIME
// ====================================================
void displayCurrentTime(DateTime now) {

  lcd.setCursor(0, 0);
  lcd.print("Time: ");

  if (now.hour() < 10)
    lcd.print("0");

  lcd.print(now.hour());
  lcd.print(":");

  if (now.minute() < 10)
    lcd.print("0");

  lcd.print(now.minute());
  lcd.print(":");

  if (now.second() < 10)
    lcd.print("0");

  lcd.print(now.second());

  lcd.setCursor(0, 1);
  lcd.print("Next: ");

  showNextMedicine(now);
}


// ====================================================
// SHOW NEXT MEDICINE
// ====================================================
void showNextMedicine(DateTime now) {

  int currentMinutes = now.hour() * 60 + now.minute();

  int nextIndex = -1;

  for (int i = 0; i < totalMedicines; i++) {

    int medicineTime =
      medicineHour[i] * 60 + medicineMinute[i];

    if (medicineTime > currentMinutes) {

      nextIndex = i;
      break;
    }
  }

  // If no medicine remains today,
  // show tomorrow's first dose
  if (nextIndex == -1) {
    nextIndex = 0;
  }

  if (medicineHour[nextIndex] < 10)
    lcd.print("0");

  lcd.print(medicineHour[nextIndex]);

  lcd.print(":");

  if (medicineMinute[nextIndex] < 10)
    lcd.print("0");

  lcd.print(medicineMinute[nextIndex]);
}
