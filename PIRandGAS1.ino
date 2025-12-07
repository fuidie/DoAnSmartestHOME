/*************************************************
 * ESP32 CHIP 2
 * MQ-2 + Buzzer + Servo + Fan + 3 Buttons + LED
 * + PIR Security + 4 ROOM LIGHTS + ALL ON/OFF
 * + BLYNK (gas + security + 4 đèn + all + events)
 *************************************************/

// ===== BLYNK CHO CHIP 2 =====
#define BLYNK_TEMPLATE_ID "TMPL62ngZBbZ0"
#define BLYNK_TEMPLATE_NAME "KHÔNG GIAN 2"
#define BLYNK_AUTH_TOKEN "tfSYYkncwSq_vJWGdneIeAxR9eSiIkhj"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "THUY NGUYEN";
char pass[] = "0935648995";

BlynkTimer timer;
// =============================

#include <Servo.h>

// ================== KHAI BÁO CHÂN ==================
#define GAS_PIN      34
#define BUZZER_PIN   25
#define SERVO_PIN    27    // servo MG90S
#define FAN_PIN      14    // quạt (qua relay)

// Nút nhấn
#define BTN_SERVO_PIN 32   // NÚT 1: ĐÓNG/MỞ CỬA
#define BTN_FAN_PIN   33   // NÚT 2: BẬT/TẮT QUẠT
#define BTN_SYS_PIN   26   // NÚT 3: BẬT/TẮT HỆ THỐNG GAS

// LED báo hệ thống gas
#define LED_SYS_PIN   4    // LED đỏ: ON = hệ thống bật, OFF = hệ thống tắt

// ======= PHẦN AN NINH (PIR) =======
#define PIR_PIN       35
#define PIR_LED_PIN    2
#define BTN_SEC_PIN    5
#define LED_SEC_PIN   15

// ======= 4 ĐÈN PHÒNG ĐỘC LẬP (P2–P5) =======
#define LED_P2_PIN  19
#define LED_P3_PIN  21
#define LED_P4_PIN  22
#define LED_P5_PIN  23

// ======= 4 NÚT BẬT/TẮT ĐÈN PHÒNG =======
#define BTN_P2_PIN  12
#define BTN_P3_PIN  13
#define BTN_P4_PIN  16
#define BTN_P5_PIN  17

// ======= NÚT BẬT/TẮT HẾT 4 ĐÈN =======
#define BTN_ALL_ON_PIN   18
#define BTN_ALL_OFF_PIN   3

// ======= VIRTUAL PIN (BLYNK) =======
// V9  - Nồng Độ Khí Gas (MQ-2)
// V10 - Hệ Thống Gas – Bật/Tắt
// V11 - Chế Độ An Ninh – Bật/Tắt
// V12 - Phát Hiện Chuyển Động
// V13 - Đèn Phòng 2 – Bật/Tắt
// V14 - Đèn Phòng 3 – Bật/Tắt
// V15 - Đèn Phòng 4 – Bật/Tắt
// V16 - Đèn Phòng 5 – Bật/Tắt
// V17 - Bật Tất Cả Đèn
// V18 - Tắt Tất Cả Đèn
// V19 - QUẠT GAS – Bật/Tắt
// V20 - CỬA THOÁT HIỂM (SERVO) – Mở/Đóng
// ====================================

int gasValue    = 0;
int threshold   = 4000;    // chỉnh theo thực tế

// Thời gian beep cho GAS
const unsigned long GAS_BEEP_ON  = 200;
const unsigned long GAS_BEEP_OFF = 300;

// Thời gian beep cho AN NINH (PIR)
const unsigned long SEC_BEEP_ON  = 100;
const unsigned long SEC_BEEP_OFF = 900;

unsigned long lastToggle = 0;
bool buzzerState = false;

const unsigned long SAMPLE_INTERVAL = 1000;
unsigned long lastSample = 0;

// servo mô phỏng cửa thoát hiểm
Servo doorServo;
const int SERVO_CLOSED_ANGLE = 0;
const int SERVO_OPEN_ANGLE   = 130;
bool doorOpen = false;

// trạng thái quạt
bool fanOn = false;

// nút
int lastBtnServoState = HIGH;
int lastBtnFanState   = HIGH;
int lastBtnSysState   = HIGH;

// chống dội nút
const unsigned long debounceDelay = 120;
unsigned long lastServoBtnTime = 0;
unsigned long lastFanBtnTime   = 0;
unsigned long lastSysBtnTime   = 0;

// ====== nút an ninh (BTN_SEC_PIN) ======
int lastBtnSecState   = HIGH;
unsigned long lastSecBtnTime = 0;

// trạng thái hệ thống gas
bool systemEnabled = true;

// trạng thái cảnh báo gas
bool gasActive     = false;
bool doorBeforeGas = false;
bool fanBeforeGas  = false;

// ====== trạng thái AN NINH (PIR) ======
unsigned long lastMotionTime = 0;
const unsigned long PIR_HOLD_TIME = 3000;

bool presence        = false;
bool lastPresence    = false;
bool securityEnabled = false;

// ====== TRẠNG THÁI 4 ĐÈN PHÒNG ======
bool ledP2 = false;
bool ledP3 = false;
bool ledP4 = false;
bool ledP5 = false;

// chống dội nút đèn phòng
int lastBtnP2 = HIGH, lastBtnP3 = HIGH, lastBtnP4 = HIGH, lastBtnP5 = HIGH;
unsigned long lastDebP2 = 0, lastDebP3 = 0, lastDebP4 = 0, lastDebP5 = 0;

// chống dội 2 nút ALL ON / ALL OFF
int lastBtnAllOn  = HIGH;
int lastBtnAllOff = HIGH;
unsigned long lastDebAllOn  = 0;
unsigned long lastDebAllOff = 0;

const unsigned long DEBOUNCE_P = 120;

// ====== CỜ CHỐNG SPAM EVENT BLYNK ======
bool gasEventSent    = false;   // đã gửi event gas_alert trong lần vượt ngưỡng này chưa
bool motionEventSent = false;   // đã gửi event pir_motion trong lần phát hiện người này chưa

// ============= HÀM TIỆN ÍCH =============

void setDoorState(bool open, bool syncBlynk = true) {
  doorOpen = open;
  doorServo.write(open ? SERVO_OPEN_ANGLE : SERVO_CLOSED_ANGLE);
  if (syncBlynk) {
    Blynk.virtualWrite(V20, open ? 1 : 0);
  }
}

void setFanState(bool on, bool syncBlynk = true) {
  fanOn = on;
  digitalWrite(FAN_PIN, on ? HIGH : LOW);
  if (syncBlynk) {
    Blynk.virtualWrite(V19, on ? 1 : 0);
  }
}

// ================= BLYNK HỖ TRỢ =================

// Khi vừa kết nối Blynk → đồng bộ trạng thái từ server
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V10, V11, V13, V14, V15, V16, V19, V20);
}

// Đồng bộ trạng thái 4 đèn + gas + PIR + quạt + cửa lên Blynk
// (KHÔNG gửi V11 ở đây nữa để tránh rối nút SECURITY)
void syncLightsToBlynk() {
  Blynk.virtualWrite(V13, ledP2 ? 1 : 0);
  Blynk.virtualWrite(V14, ledP3 ? 1 : 0);
  Blynk.virtualWrite(V15, ledP4 ? 1 : 0);
  Blynk.virtualWrite(V16, ledP5 ? 1 : 0);

  Blynk.virtualWrite(V9,  gasValue);
  Blynk.virtualWrite(V10, systemEnabled ? 1 : 0);
  Blynk.virtualWrite(V12, presence ? 1 : 0);
  Blynk.virtualWrite(V19, fanOn ? 1 : 0);
  Blynk.virtualWrite(V20, doorOpen ? 1 : 0);
}

// ====== BLYNK_WRITE: BẬT/TẮT HỆ THỐNG GAS (V10) ======
BLYNK_WRITE(V10) {
  int v = param.asInt();
  systemEnabled = (v == 1);

  if (!systemEnabled) {
    gasActive = false;
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;
    digitalWrite(LED_SYS_PIN, LOW);
    Serial.println("BLYNK - TAT HE THONG GAS");
  } else {
    digitalWrite(LED_SYS_PIN, HIGH);
    Serial.println("BLYNK - BAT HE THONG GAS");
  }
}

// ====== BLYNK_WRITE: BẬT/TẮT CHẾ ĐỘ AN NINH (V11) ======
BLYNK_WRITE(V11) {
  int v = param.asInt();
  securityEnabled = (v == 1);

  presence = false;
  lastPresence = false;
  digitalWrite(PIR_LED_PIN, LOW);
  digitalWrite(LED_SEC_PIN, securityEnabled ? HIGH : LOW);

  Serial.println(securityEnabled ?
                 "BLYNK - SECURITY MODE: ON" :
                 "BLYNK - SECURITY MODE: OFF");
}

// ====== BLYNK_WRITE: ĐIỀU KHIỂN 4 ĐÈN ======
BLYNK_WRITE(V13) {
  int v = param.asInt();
  ledP2 = (v == 1);
  digitalWrite(LED_P2_PIN, ledP2 ? HIGH : LOW);
  Serial.print("BLYNK - DEN PHONG 2: ");
  Serial.println(ledP2 ? "ON" : "OFF");
}

BLYNK_WRITE(V14) {
  int v = param.asInt();
  ledP3 = (v == 1);
  digitalWrite(LED_P3_PIN, ledP3 ? HIGH : LOW);
  Serial.print("BLYNK - DEN PHONG 3: ");
  Serial.println(ledP3 ? "ON" : "OFF");
}

BLYNK_WRITE(V15) {
  int v = param.asInt();
  ledP4 = (v == 1);
  digitalWrite(LED_P4_PIN, ledP4 ? HIGH : LOW);
  Serial.print("BLYNK - DEN PHONG 4: ");
  Serial.println(ledP4 ? "ON" : "OFF");
}

BLYNK_WRITE(V16) {
  int v = param.asInt();
  ledP5 = (v == 1);
  digitalWrite(LED_P5_PIN, ledP5 ? HIGH : LOW);
  Serial.print("BLYNK - DEN PHONG 5: ");
  Serial.println(ledP5 ? "ON" : "OFF");
}

// ====== BLYNK_WRITE: NÚT BẬT HẾT / TẮT HẾT ======
BLYNK_WRITE(V17) {
  int v = param.asInt();
  if (v == 1) {
    ledP2 = ledP3 = ledP4 = ledP5 = true;

    digitalWrite(LED_P2_PIN, HIGH);
    digitalWrite(LED_P3_PIN, HIGH);
    digitalWrite(LED_P4_PIN, HIGH);
    digitalWrite(LED_P5_PIN, HIGH);

    Blynk.virtualWrite(V13, 1);
    Blynk.virtualWrite(V14, 1);
    Blynk.virtualWrite(V15, 1);
    Blynk.virtualWrite(V16, 1);

    Serial.println("BLYNK - ALL ROOMS: ON (P2-P5)");
  }
}

BLYNK_WRITE(V18) {
  int v = param.asInt();
  if (v == 1) {
    ledP2 = ledP3 = ledP4 = ledP5 = false;

    digitalWrite(LED_P2_PIN, LOW);
    digitalWrite(LED_P3_PIN, LOW);
    digitalWrite(LED_P4_PIN, LOW);
    digitalWrite(LED_P5_PIN, LOW);

    Blynk.virtualWrite(V13, 0);
    Blynk.virtualWrite(V14, 0);
    Blynk.virtualWrite(V15, 0);
    Blynk.virtualWrite(V16, 0);

    Serial.println("BLYNK - ALL ROOMS: OFF (P2-P5)");
  }
}

// ====== BLYNK_WRITE: ĐIỀU KHIỂN QUẠT (V19) ======
BLYNK_WRITE(V19) {
  int v = param.asInt();

  if (!gasActive) {
    setFanState(v == 1, false);
    Serial.print("BLYNK - QUAT: ");
    Serial.println(fanOn ? "ON" : "OFF");
  } else {
    if (v == 0) {
      Blynk.virtualWrite(V19, 1);
    }
    Serial.println("BLYNK - QUAT: DANG GAS, KHONG THE TAT (AUTO ON)");
  }
}

// ====== BLYNK_WRITE: ĐIỀU KHIỂN CỬA THOÁT HIỂM (SERVO) – V20 ======
BLYNK_WRITE(V20) {
  int v = param.asInt();

  if (gasActive) {
    // đang có GAS → luôn mở, không cho đóng
    setDoorState(true, false);
    if (v == 0) {
      Blynk.virtualWrite(V20, 1);
    }
    Serial.println("BLYNK - CUA THOAT HIEM: DANG CO GAS => LUON MO, KHONG DUOC DONG");
    return;
  }

  setDoorState(v == 1, false);

  Serial.print("BLYNK - CUA THOAT HIEM: ");
  Serial.println(doorOpen ? "MO" : "DONG");
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  // ==== WIFI + BLYNK KHÔNG BLOCKING ====
  WiFi.begin(ssid, pass);
  Serial.print("Dang ket noi WiFi");
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    Serial.print(".");
    delay(200);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi KHONG KET NOI (van cho he thong chay binh thuong).");
  }

  Blynk.config(BLYNK_AUTH_TOKEN);
  timer.setInterval(1000L, syncLightsToBlynk);

  pinMode(GAS_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);
  fanOn = false;

  pinMode(BTN_SERVO_PIN, INPUT_PULLUP);
  pinMode(BTN_FAN_PIN,   INPUT_PULLUP);
  pinMode(BTN_SYS_PIN,   INPUT_PULLUP);

  pinMode(LED_SYS_PIN, OUTPUT);
  digitalWrite(LED_SYS_PIN, HIGH);

  doorServo.attach(SERVO_PIN);
  setDoorState(false, false);   // cửa đóng, không sync Blynk ở đây

  pinMode(PIR_PIN, INPUT);
  pinMode(PIR_LED_PIN, OUTPUT);
  pinMode(BTN_SEC_PIN, INPUT_PULLUP);
  pinMode(LED_SEC_PIN, OUTPUT);
  digitalWrite(PIR_LED_PIN, LOW);
  digitalWrite(LED_SEC_PIN, LOW);

  pinMode(LED_P2_PIN, OUTPUT);
  pinMode(LED_P3_PIN, OUTPUT);
  pinMode(LED_P4_PIN, OUTPUT);
  pinMode(LED_P5_PIN, OUTPUT);

  digitalWrite(LED_P2_PIN, LOW);
  digitalWrite(LED_P3_PIN, LOW);
  digitalWrite(LED_P4_PIN, LOW);
  digitalWrite(LED_P5_PIN, LOW);

  pinMode(BTN_P2_PIN, INPUT_PULLUP);
  pinMode(BTN_P3_PIN, INPUT_PULLUP);
  pinMode(BTN_P4_PIN, INPUT_PULLUP);
  pinMode(BTN_P5_PIN, INPUT_PULLUP);

  pinMode(BTN_ALL_ON_PIN,  INPUT_PULLUP);
  pinMode(BTN_ALL_OFF_PIN, INPUT_PULLUP);

  Serial.println("=== MQ-2 + Buzzer + Servo + Fan + 3 Buttons + LED + PIR + 4 ROOM LIGHTS + ALL ON/OFF + BLYNK + EVENTS ===");
  delay(2000);
}

// ================== LOOP ==================
void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  }
  timer.run();

  unsigned long now = millis();

  // ================== NÚT 4: BẬT/TẮT CHẾ ĐỘ AN NINH ==================
  int btnSecState = digitalRead(BTN_SEC_PIN);

  if (btnSecState == LOW && lastBtnSecState == HIGH &&
      (now - lastSecBtnTime) > debounceDelay) {

    lastSecBtnTime = now;
    securityEnabled = !securityEnabled;

    presence = false;
    lastPresence = false;
    digitalWrite(PIR_LED_PIN, LOW);
    digitalWrite(LED_SEC_PIN, securityEnabled ? HIGH : LOW);

    // CẬP NHẬT LÊN BLYNK (V11) KHI NHẤN NÚT VẬT LÝ
    Blynk.virtualWrite(V11, securityEnabled ? 1 : 0);

    // reset cờ event chuyển động khi tắt/bật lại chế độ an ninh
    motionEventSent = false;

    Serial.println(securityEnabled ?
                   ">>> SECURITY MODE: ON (BUTTON)" :
                   ">>> SECURITY MODE: OFF (BUTTON)");
  }
  lastBtnSecState = btnSecState;

  // ================== XỬ LÝ PIR KHI CHẾ ĐỘ AN NINH BẬT ==================
  if (securityEnabled) {
    int pirRaw = digitalRead(PIR_PIN);

    if (pirRaw == HIGH) {
      presence = true;
      lastMotionTime = now;
    }

    if (presence && (now - lastMotionTime > PIR_HOLD_TIME)) {
      presence = false;
    }

    digitalWrite(PIR_LED_PIN, presence ? HIGH : LOW);

    if (presence != lastPresence) {
      if (presence) {
        Serial.println("SECURITY: Phat hien chuyen dong (CO NGUOI).");

        // ==== GỬI EVENT BLYNK KHI PHÁT HIỆN NGƯỜI ====
        if (!motionEventSent) {
          Blynk.logEvent("pir_motion", "SECURITY: Phat hien chuyen dong trong khu vuc giam sat");
          motionEventSent = true;
        }
      } else {
        Serial.println("SECURITY: Khong phat hien chuyen dong (KHONG CON AI).");

        // cho phép lần sau phát hiện lại bắn event tiếp
        motionEventSent = false;
      }

      lastPresence = presence;
    }
  } else {
    digitalWrite(PIR_LED_PIN, LOW);
    motionEventSent = false;
  }

  // ================== PHẦN NÚT CỬA / QUẠT / HỆ THỐNG GAS ==================

  // NÚT 1: CỬA
  int btnServoState = digitalRead(BTN_SERVO_PIN);

  if (btnServoState == LOW && lastBtnServoState == HIGH &&
      (now - lastServoBtnTime) > debounceDelay) {

    lastServoBtnTime = now;

    if (!gasActive) {
      setDoorState(!doorOpen, true);

      Serial.print(">>> NÚT 1: ");
      Serial.println(doorOpen ? "MỞ CỬA THỦ CÔNG" : "ĐÓNG CỬA THỦ CÔNG");
    } else {
      Serial.println(">>> NÚT 1: ĐANG CÓ GAS, CỬA ĐANG AUTO MỞ (KHÔNG ĐÓNG THỦ CÔNG ĐƯỢC)");
    }
  }
  lastBtnServoState = btnServoState;

  // NÚT 2: QUẠT
  int btnFanState = digitalRead(BTN_FAN_PIN);

  if (btnFanState == LOW && lastBtnFanState == HIGH &&
      (now - lastFanBtnTime) > debounceDelay) {

    lastFanBtnTime = now;

    if (!gasActive) {
      setFanState(!fanOn, true);

      Serial.print(">>> NÚT 2 QUẠT: ");
      Serial.println(fanOn ? "BẬT QUẠT THỦ CÔNG" : "TẮT QUẠT THỦ CÔNG");
    } else {
      Serial.println(">>> NÚT 2 QUẠT: ĐANG CÓ GAS, QUẠT ĐANG AUTO BẬT (KHÔNG TẮT THỦ CÔNG ĐƯỢC)");
    }
  }
  lastBtnFanState = btnFanState;

  // NÚT 3: HỆ THỐNG GAS
  int btnSysState = digitalRead(BTN_SYS_PIN);

  if (btnSysState == LOW && lastBtnSysState == HIGH &&
      (now - lastSysBtnTime) > debounceDelay) {

    lastSysBtnTime = now;

    systemEnabled = !systemEnabled;

    if (!systemEnabled) {
      gasActive = false;
      digitalWrite(BUZZER_PIN, LOW);
      buzzerState = false;

      digitalWrite(LED_SYS_PIN, LOW);
      Blynk.virtualWrite(V10, 0);

      // reset cờ event gas khi tắt hệ thống gas
      gasEventSent = false;

      Serial.println(">>> NÚT 3: TẮT HỆ THỐNG GAS");
    } else {
      digitalWrite(LED_SYS_PIN, HIGH);
      Blynk.virtualWrite(V10, 1);
      Serial.println(">>> NÚT 3: BẬT HỆ THỐNG GAS");
    }
  }
  lastBtnSysState = btnSysState;

  // --- CẬP NHẬT GIÁ TRỊ + IN SERIAL MỖI 1 GIÂY ---
  if (now - lastSample >= SAMPLE_INTERVAL) {
    lastSample = now;

    gasValue = analogRead(GAS_PIN);

    Serial.print("MQ-2 = ");
    Serial.print(gasValue);

    if (systemEnabled && gasValue > threshold) {
      Serial.print("   <<< !!! CẢNH BÁO: GAS VƯỢT NGƯỠNG !!! >>>");
    }

    if (securityEnabled) {
      Serial.print("   [SECURITY: ON]");
    }

    Serial.println();
  }

  // ================== 4 ĐÈN PHÒNG ĐỘC LẬP (P2–P5) ==================

  int sP2 = digitalRead(BTN_P2_PIN);
  if (sP2 == LOW && lastBtnP2 == HIGH && (now - lastDebP2 > DEBOUNCE_P)) {
    lastDebP2 = now;
    ledP2 = !ledP2;
    digitalWrite(LED_P2_PIN, ledP2 ? HIGH : LOW);
    Blynk.virtualWrite(V13, ledP2 ? 1 : 0);
    Serial.print("DEN PHONG 2: ");
    Serial.println(ledP2 ? "ON" : "OFF");
  }
  lastBtnP2 = sP2;

  int sP3 = digitalRead(BTN_P3_PIN);
  if (sP3 == LOW && lastBtnP3 == HIGH && (now - lastDebP3 > DEBOUNCE_P)) {
    lastDebP3 = now;
    ledP3 = !ledP3;
    digitalWrite(LED_P3_PIN, ledP3 ? HIGH : LOW);
    Blynk.virtualWrite(V14, ledP3 ? 1 : 0);
    Serial.print("DEN PHONG 3: ");
    Serial.println(ledP3 ? "ON" : "OFF");
  }
  lastBtnP3 = sP3;

  int sP4 = digitalRead(BTN_P4_PIN);
  if (sP4 == LOW && lastBtnP4 == HIGH && (now - lastDebP4 > DEBOUNCE_P)) {
    lastDebP4 = now;
    ledP4 = !ledP4;
    digitalWrite(LED_P4_PIN, ledP4 ? HIGH : LOW);
    Blynk.virtualWrite(V15, ledP4 ? 1 : 0);
    Serial.print("DEN PHONG 4: ");
    Serial.println(ledP4 ? "ON" : "OFF");
  }
  lastBtnP4 = sP4;

  int sP5 = digitalRead(BTN_P5_PIN);
  if (sP5 == LOW && lastBtnP5 == HIGH && (now - lastDebP5 > DEBOUNCE_P)) {
    lastDebP5 = now;
    ledP5 = !ledP5;
    digitalWrite(LED_P5_PIN, ledP5 ? HIGH : LOW);
    Blynk.virtualWrite(V16, ledP5 ? 1 : 0);
    Serial.print("DEN PHONG 5: ");
    Serial.println(ledP5 ? "ON" : "OFF");
  }
  lastBtnP5 = sP5;

  // ===== 2 NÚT: BẬT HẾT / TẮT HẾT 4 ĐÈN =====

  int sAllOn = digitalRead(BTN_ALL_ON_PIN);
  if (sAllOn == LOW && lastBtnAllOn == HIGH && (now - lastDebAllOn > DEBOUNCE_P)) {
    lastDebAllOn = now;

    ledP2 = ledP3 = ledP4 = ledP5 = true;
    digitalWrite(LED_P2_PIN, HIGH);
    digitalWrite(LED_P3_PIN, HIGH);
    digitalWrite(LED_P4_PIN, HIGH);
    digitalWrite(LED_P5_PIN, HIGH);

    Blynk.virtualWrite(V13, 1);
    Blynk.virtualWrite(V14, 1);
    Blynk.virtualWrite(V15, 1);
    Blynk.virtualWrite(V16, 1);

    Serial.println("ALL ROOMS: ON (P2-P5)");
  }
  lastBtnAllOn = sAllOn;

  int sAllOff = digitalRead(BTN_ALL_OFF_PIN);
  if (sAllOff == LOW && lastBtnAllOff == HIGH && (now - lastDebAllOff > DEBOUNCE_P)) {
    lastDebAllOff = now;

    ledP2 = ledP3 = ledP4 = ledP5 = false;
    digitalWrite(LED_P2_PIN, LOW);
    digitalWrite(LED_P3_PIN, LOW);
    digitalWrite(LED_P4_PIN, LOW);
    digitalWrite(LED_P5_PIN, LOW);

    Blynk.virtualWrite(V13, 0);
    Blynk.virtualWrite(V14, 0);
    Blynk.virtualWrite(V15, 0);
    Blynk.virtualWrite(V16, 0);

    Serial.println("ALL ROOMS: OFF (P2-P5)");
  }
  lastBtnAllOff = sAllOff;

  // ================== AUTO KHI CÓ GAS (CHỈ KHI systemEnabled = true) ==================
  bool overThreshold = (systemEnabled && gasValue > threshold);

  // Khi GAS vượt ngưỡng
  if (overThreshold && !gasActive) {
    gasActive = true;

    doorBeforeGas = doorOpen;
    fanBeforeGas  = fanOn;

    if (!doorOpen) {
      setDoorState(true, true);
    }

    if (!fanOn) {
      setFanState(true, true);
    }

    // ==== GỬI EVENT BLYNK KHI CÓ GAS ====
    if (!gasEventSent) {
      Blynk.logEvent("gas_alert", String("Nong do GAS vuot nguong: ") + gasValue);
      gasEventSent = true;
    }

    Serial.println(">>> AUTO: GAS VƯỢT NGƯỠNG -> MỞ CỬA & BẬT QUẠT");
  }

  // Khi GAS về lại bình thường
  if (!overThreshold && gasActive) {
    gasActive = false;

    setDoorState(doorBeforeGas, true);
    setFanState(fanBeforeGas, true);

    // cho phép lần sau vượt ngưỡng lại bắn event tiếp
    gasEventSent = false;

    Serial.println(">>> AUTO: HẾT GAS -> TRẢ CỬA & QUẠT VỀ TRẠNG THÁI BAN ĐẦU");
  }

  // ================== BUZZER ==================
  // Ưu tiên GAS > AN NINH > TẮT
  unsigned long interval;

  if (overThreshold) {
    interval = buzzerState ? GAS_BEEP_ON : GAS_BEEP_OFF;
  } else if (securityEnabled && presence) {
    interval = buzzerState ? SEC_BEEP_ON : SEC_BEEP_OFF;
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;
    lastToggle = now;
    return;
  }

  if (now - lastToggle >= interval) {
    buzzerState = !buzzerState;
    digitalWrite(BUZZER_PIN, buzzerState);
    lastToggle = now;
  }
}
