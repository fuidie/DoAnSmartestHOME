/*********
  Gộp 2 dự án:
  - Khóa cửa thông minh (keypad + Blynk + web config)
  - Trạm giám sát chất lượng không khí (DHT11 + GP2Y1010)
  + Thêm: đo độ ẩm đất (SOIL_MOISTURE)
*********/

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID           "BANLINHKIEN"
#define BLYNK_TEMPLATE_NAME         "BANLINHKIEN"
char BLYNK_AUTH_TOKEN[32]   =   "";

// ================== THƯ VIỆN ==================
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SimpleKalmanFilter.h>
#include "index_html.h"
#include "data_config.h"
#include <EEPROM.h>
#include <Arduino_JSON.h>
#include "icon.h"

// Cảm biến môi trường
#include "DHT.h"
#include <GP2Y1010AU0F.h>

// Nút nhấn
#include "mybutton.h"

// Màn hình OLED
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// ================== WEB SERVER ==================
AsyncWebServer server(80);

// ================== BLYNK ==================
bool blynkConnect = true;
BlynkTimer timer; 

// Một số Macro
#define ENABLE    1
#define DISABLE   0

// ================== OLED ==================
#define i2c_Address 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SH1106G oled = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define OLED_SDA      21
#define OLED_SCL      22

typedef enum {
  SCREEN0,
  SCREEN1,
  SCREEN2,
  SCREEN3,
  SCREEN4,
  SCREEN5,
  SCREEN6,
  SCREEN7,
  SCREEN8,
  SCREEN9,
  SCREEN10,
  SCREEN11,
  SCREEN12,
  SCREEN13
}SCREEN;
int screenOLED = SCREEN0;

bool enableShow1 = DISABLE;
bool enableShow2 = DISABLE;
bool enableShow3 = DISABLE;

bool autoWarning = DISABLE; // Dùng chung: đột nhập + môi trường

// ================== I/O PHẦN CỨNG ==================
// LED
#define LED           33
#define LED_ON        0
#define LED_OFF       1
// RELAY khóa cửa
#define RELAY         25
// BUZZER
#define BUZZER        2
uint32_t timeCountBuzzerWarning = 0;

// Chuỗi hiển thị OLED
String OLED_STRING1 = "Xin chao";
String OLED_STRING2 = "Hi my friend";
String OLED_STRING3 = "Canh bao";

// ================== KEYPAD KHÓA CỬA ==================
#include "Adafruit_Keypad.h"
#define KEYPAD_ROW1   4
#define KEYPAD_ROW2   16
#define KEYPAD_ROW3   17
#define KEYPAD_ROW4   5
#define KEYPAD_COL1   12
#define KEYPAD_COL2   14
#define KEYPAD_COL3   27

const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
    {'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'*', '0', '#'}
};
byte rowPins[ROWS] = {KEYPAD_ROW1, KEYPAD_ROW2, KEYPAD_ROW3, KEYPAD_ROW4};
byte colPins[COLS] = {KEYPAD_COL1, KEYPAD_COL2, KEYPAD_COL3}; 
Adafruit_Keypad myKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);
keypadEvent e;

typedef enum {
   IDLE_MODE,
   WAIT_CLOSE,
   CHANGE_PASSWORD,
   LOCK_1MIN
}MODE;
uint8_t modeRun = IDLE_MODE;
uint8_t allowAccess = 0;
uint8_t countError = 0;
uint8_t countKey = 0;
char passwordEnter[5] = {0}; 
char password[5] = "1111";            // mật khẩu mặc định
char keyMode;
uint32_t timeMillisAUTO=0;
uint8_t notiOne = 0;        

typedef enum {
   ENTER_PASS_1,
   ENTER_PASS_2
}CHANGEPASS;
uint8_t changePassword = ENTER_PASS_1;
char passwordChange1[5] = {0};
char passwordChange2[5] = {0};
char starkey[8];

// ================== BUTTON ==================
#define BUTTON_DOWN_PIN   34
#define BUTTON_UP_PIN     35
#define BUTTON_SET_PIN    32

#define BUTTON1_ID  1
#define BUTTON2_ID  2
#define BUTTON3_ID  3
Button buttonSET;
Button buttonDOWN;
Button buttonUP;

void button_press_short_callback(uint8_t button_id);
void button_press_long_callback(uint8_t button_id);

// ================== TASK HANDLE ==================
TaskHandle_t TaskButton_handle      = NULL;
TaskHandle_t TaskOLEDDisplay_handle = NULL;
TaskHandle_t TaskDHT11_handle       = NULL;
TaskHandle_t TaskDustSensor_handle  = NULL;
TaskHandle_t TaskAutoWarning_handle = NULL;
// === ĐỘ ẨM ĐẤT ===
TaskHandle_t TaskSoilMoisture_handle = NULL;

// ================== CẢM BIẾN MÔI TRƯỜNG ==================
// DHT11
#define DHT11_PIN   26
#define DHTTYPE     DHT11
DHT dht(DHT11_PIN, DHTTYPE);
float tempValue = 0;
int   humiValue = 0;
bool  dht11ReadOK = false;

// GP2Y1010
#define DUST_TRIG    23
#define DUST_ANALOG  36
GP2Y1010AU0F dustSensor(DUST_TRIG, DUST_ANALOG);
int dustValue    = 0;
int dustValueMax = 0;
int countDust    = 0;

// === ĐỘ ẨM ĐẤT ===
#define SOIL_MOISTURE 39      // chân analog đo độ ẩm đất
int soilMoistureValue = 0;    // giá trị % độ ẩm đất (0–100)

// Hàm kiểm tra chất lượng không khí
void check_air_quality_and_send_to_blynk(bool autoMode, float temp, int humi, int dust);

// ================== KHAI BÁO TRƯỚC ==================
void TaskButton(void *pvParameters);
void TaskOLEDDisplay(void *pvParameters);
void TaskKeypad(void *pvParameters);
void TaskDHT11(void *pvParameters);
void TaskDustSensor(void *pvParameters);
void TaskAutoWarning(void *pvParameters);
void TaskBlynk(void *pvParameters);
// === ĐỘ ẨM ĐẤT ===
void TaskSoilMoisture(void *pvParameters);

void connectSTA();
void connectAPMode();
String getJsonData();
void writeEEPROM();
void readEEPROM();
void clearEeprom();
void myTimer();

// ================== SETUP ==================
void setup(){
  Serial.begin(115200);

  EEPROM.begin(512);
  readEEPROM();   // đọc Essid, Epass, Etoken, cấu hình khóa + môi trường

  // RELAY
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, DISABLE);

  // BUZZER
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, DISABLE);

  // KEYPAD
  myKeypad.begin();

  // LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LED_OFF);
  
  // OLED
  oled.begin(i2c_Address, true);
  oled.setTextSize(2);
  oled.setTextColor(SH110X_WHITE);

  // Đọc autotWarning từ EEPROM
  autoWarning = EEPROM.read(210);

  // Đọc mật khẩu từ EEPROM
  savePASStoBUFF();
  EpassDoor = convertPassToNumber();

  // Button
  pinMode(BUTTON_SET_PIN, INPUT_PULLUP);
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  button_init(&buttonSET,   BUTTON_SET_PIN,   BUTTON1_ID);
  button_init(&buttonUP,    BUTTON_UP_PIN,    BUTTON2_ID);
  button_init(&buttonDOWN,  BUTTON_DOWN_PIN,  BUTTON3_ID);
  button_pressshort_set_callback((void *)button_press_short_callback);
  button_presslong_set_callback((void *)button_press_long_callback);

  // Cảm biến môi trường
  dht.begin();
  dustSensor.begin();
  // === ĐỘ ẨM ĐẤT ===
  // GPIO39 là input ADC mặc định, không cần pinMode, nhưng có thể để cho rõ:
  // pinMode(SOIL_MOISTURE, INPUT);

  // Task
  xTaskCreatePinnedToCore(TaskButton,      "TaskButton" ,      1024*10 ,  NULL,  20 ,  &TaskButton_handle       , 1);
  xTaskCreatePinnedToCore(TaskOLEDDisplay, "TaskOLEDDisplay" , 1024*16 ,  NULL,  20 ,  &TaskOLEDDisplay_handle  , 1);
  xTaskCreatePinnedToCore(TaskKeypad,      "TaskKeypad" ,      1024*4  ,  NULL,   5 ,  NULL ,  1);
  xTaskCreatePinnedToCore(TaskDHT11,       "TaskDHT11" ,       1024*6  ,  NULL,  10 ,  &TaskDHT11_handle        , 1);
  xTaskCreatePinnedToCore(TaskDustSensor,  "TaskDustSensor" ,  1024*6  ,  NULL,  10 ,  &TaskDustSensor_handle   , 1);
  xTaskCreatePinnedToCore(TaskAutoWarning, "TaskAutoWarning" , 1024*6  ,  NULL,  10 ,  &TaskAutoWarning_handle  , 1);
  // === ĐỘ ẨM ĐẤT ===
  xTaskCreatePinnedToCore(TaskSoilMoisture,"TaskSoilMoisture", 1024*6  ,  NULL,  10 ,  &TaskSoilMoisture_handle , 1);

  // Kết nối wifi
  connectSTA();
}

void loop() {
  vTaskDelete(NULL);
}

// ======================== LOGIC KHÓA CỬA (giữ nguyên) ========================
void TaskKeypad(void *pvParameters) {
    while(1) {
      switch(modeRun) {
        case IDLE_MODE:
          allowAccess = 0;
          if(notiOne == 1) {
             Blynk.virtualWrite(V0, DISABLE);  
             digitalWrite(LED, LED_OFF);
             Serial.println("Khoa cua");
             controlLock(DISABLE);
             notiOne = 0;
             screenOLED = SCREEN1;
             enableShow3 = DISABLE; enableShow2 = DISABLE; enableShow1 = ENABLE;
             OLED_STRING1 = "Xin chao";
             OLED_STRING2 = "Hi my friend";
             strcpy(starkey ,"        ");
          }
          myKeypad.tick();
          while(myKeypad.available()){
              e = myKeypad.read();
              if(e.bit.EVENT == KEY_JUST_RELEASED) {
                  buzzerBeep(1);
                  if( (char)e.bit.KEY == '#' ) {
                      countKey=0; 
                      modeRun = IDLE_MODE ;
                      Serial.println("cancel");
                      strcpy(starkey ,"        ");
                      notiOne = 1;
                  } else {
                      passwordEnter[countKey] = (char)e.bit.KEY;
                      Serial.print((char)e.bit.KEY);
                      starkey[countKey*2] = '*'; starkey[countKey*2 + 1] = ' ';
                      countKey++ ;
                      screenOLED = SCREEN2;
                      enableShow3 = DISABLE; enableShow2 = ENABLE; enableShow1 = DISABLE;
                      OLED_STRING1 = "Nhap mat khau";
                      OLED_STRING2 = starkey;
                  }
              }  
            }  
            if(countKey > 3) {
              passwordEnter[4] = '\0';
              delay(50);
              Serial.println(passwordEnter);
              timeMillisAUTO = millis();
              if(strcmp(passwordEnter,password) == 0) {
                  buzzerBeep(3);
                  allowAccess = 1;
                  Serial.println("CORRECT PASSWORD");
                  enableShow3 = DISABLE; enableShow2 = ENABLE; enableShow1 = DISABLE;
                  screenOLED = SCREEN2;
                  OLED_STRING1 = "Mat khau dung";
                  OLED_STRING2 = "Moi vao";
                  strcpy(starkey ,"        ");
                  digitalWrite(LED, LED_ON);
                  Serial.println("MO khoa");
                  controlLock(ENABLE);
                  modeRun = WAIT_CLOSE; 
              } else {              
                  enableShow3 = DISABLE; enableShow2 = ENABLE; enableShow1 = DISABLE;
                  screenOLED = SCREEN2;
                  OLED_STRING1 = "Mat khau sai";
                  OLED_STRING2 = "Con " + String((EnumberEnterWrong - countError - 1)) + " lan";
                  buzzerWarningBeep(1000);
                  if(autoWarning == 1)
                     Blynk.logEvent("auto_warning","Canh bao dot nhap");
                  strcpy(starkey ,"        ");
                  countError ++;   
                  notiOne = 1;
                  delay(1000);
                  if(  countError >= EnumberEnterWrong)
                      modeRun = LOCK_1MIN;
              }
              countKey = 0;
            }
          break;
        case WAIT_CLOSE:
          countError=0; notiOne = 1;
          if(millis() - timeMillisAUTO > EtimeOpenDoor*1000) {
                timeMillisAUTO = millis();
                countKey=0; 
                delay(20);
                modeRun = IDLE_MODE; 
          }
          myKeypad.tick();
          while(myKeypad.available()){
                e = myKeypad.read();
                timeMillisAUTO = millis();
                if(e.bit.EVENT == KEY_JUST_RELEASED)  {
                    buzzerBeep(3);
                    keyMode = (char)e.bit.KEY; 
                    Serial.println((char)e.bit.KEY);
                    if(keyMode == '*' ) { 
                      if(EenableChangePass == ENABLE) {
                        Serial.println("mode doi pass");
                        Serial.println("khoa cua");
                        enableShow3 = ENABLE; enableShow2 = DISABLE; enableShow1 = DISABLE;
                        screenOLED = SCREEN3;
                        OLED_STRING1 = "Mat khau moi";
                        OLED_STRING2 = "";
                        controlLock(DISABLE);
                        blinkLED(3);
                        strcpy(starkey ,"        "); 
                        modeRun = CHANGE_PASSWORD;
                      } else {
                        Serial.println("Khong cho phep doi mat khau");
                        enableShow3 = ENABLE; enableShow2 = DISABLE; enableShow1 = DISABLE;
                        screenOLED = SCREEN3;
                        OLED_STRING1 = "Khong the";
                        OLED_STRING2 = "doi mat khau";
                        controlLock(DISABLE);
                        buzzerWarningBeep(1000);
                        strcpy(starkey ,"        "); 
                        delay(1000);
                        modeRun = IDLE_MODE;
                      }
                    } else 
                        modeRun = IDLE_MODE;
                }   
          }      
          break;
        case CHANGE_PASSWORD:
          changePasswordFunc();
          break;
        case LOCK_1MIN:
            notiOne = 1;
            Serial.println("thu lai sau 1 phut");
            buzzerWarningBeep(2000);
            for(int i = EtimeLock; i >=0; i --) {
                enableShow3 = DISABLE; enableShow2 = DISABLE; enableShow1 = ENABLE;
                screenOLED = SCREEN1;
                OLED_STRING1 = "Thu lai sau";
                OLED_STRING2 = String(i) + " giay";
                Serial.print("Thu lai sau ");
                Serial.print(i);
                Serial.println(" s");
                delay(1000);
            }
            countError = 0;
            modeRun = IDLE_MODE;
            break;   
      }
      delay(10);
    }
}

void controlLock(int state) {
    digitalWrite(RELAY, state);
    Blynk.virtualWrite(V0, state);  
}

// Đổi mật khẩu (giữ nguyên)
void changePasswordFunc() {
  switch(changePassword) {
  case  ENTER_PASS_1:
        myKeypad.tick();
        while(myKeypad.available()){
          e = myKeypad.read();
          timeMillisAUTO = millis();
          if(e.bit.EVENT == KEY_JUST_RELEASED) {
            buzzerBeep(1);
            if( (char)e.bit.KEY == '#' ) {
                  countKey = 0; 
                  modeRun = IDLE_MODE ;  
                  Serial.println("cancel");
                  notiOne = 1;
            } else {
                passwordChange1[countKey] = (char)e.bit.KEY; 
                Serial.println((char)e.bit.KEY);
                starkey[countKey*2] = '*'; starkey[countKey*2 + 1] = ' ';
                countKey++ ;
                screenOLED = SCREEN3;
                OLED_STRING1 = "Mat khau moi";
                OLED_STRING2 = starkey;
            }
          }
        }
        if(countKey > 3) {
          delay(200);
          changePassword = ENTER_PASS_2;
          buzzerBeep(2);
          countKey  = 0;
          screenOLED = SCREEN3;
          OLED_STRING1 = "Nhap lai";
          OLED_STRING2 = "";
          memset(starkey, 0, sizeof(starkey)); 
        }
      break;
  case  ENTER_PASS_2:
      myKeypad.tick();
      while(myKeypad.available()){
        e = myKeypad.read();
        timeMillisAUTO = millis();
        if(e.bit.EVENT == KEY_JUST_RELEASED)  {
          buzzerBeep(1);
          if( (char)e.bit.KEY == '#' ) {
            countKey = 0; 
            modeRun = IDLE_MODE ;
            Serial.println("cancel");
            notiOne = 1;
          } else {
            passwordChange2[countKey] = (char)e.bit.KEY; 
            starkey[countKey*2] = '*'; starkey[countKey*2 + 1] = ' ';
            countKey++ ;
            screenOLED = SCREEN3;
            OLED_STRING1 = "Nhap lai";
            OLED_STRING2 =  starkey ;
          }
        }
      }
      if(countKey > 3) {
        memset(starkey, 0, sizeof(starkey)); 
        delay(50);
        if(strcmp(passwordChange1, passwordChange2) == 0) {   
          screenOLED = SCREEN3;
          OLED_STRING1 = "Doi mat khau";
          OLED_STRING2 = "Thanh cong";
          memset(starkey, 0, sizeof(starkey)); 
          buzzerBeep(5);  
          blinkLED(3);   
          passwordChange2[4] = '\0';
          memcpy (password, passwordChange1, 5);
          Serial.print("new pass");
          Serial.println(password);
          Blynk.logEvent("auto_warning","Canh bao doi mat khau : " + String(password));
          uint32_t valueCV =convertPassToNumber();
          EpassDoor = valueCV;
          savePWtoEEP(valueCV);
          savePASStoBUFF();
          Serial.println("CHANGE SUCCESSFUL ");
          delay(2000);
          changePassword = ENTER_PASS_1;
          modeRun = IDLE_MODE ;
        } else {
          blinkLED(1);
          screenOLED = SCREEN3; 
          OLED_STRING1 = "Mat khau sai";
          OLED_STRING2 = "Nhap lai";
          Serial.println("NOT CORRECT");
          buzzerWarningBeep(1000);
          changePassword = ENTER_PASS_1;
          strcpy(starkey ,"        ");
          OLED_STRING1 = "Mat khau moi";
          OLED_STRING2 = "";
        }
        countKey = 0;
      }
      break;
  }     
}

// ======================== OLED HIỂN THỊ ========================
void clearRectangle(int x1, int y1, int x2, int y2) {
   for(int i = y1; i < y2; i++) {
     oled.drawLine(x1, i, x2, i, 0);
   }
}

void clearOLED(){
  oled.clearDisplay();
  oled.display();
}

int countSCREEN9 = 0;

void TaskOLEDDisplay(void *pvParameters) {
  while (1) {
      switch(screenOLED) {
        case SCREEN0:
          for(int j = 0; j < 3; j++) {
            for(int i = 0; i < FRAME_COUNT_loadingOLED; i++) {
              oled.clearDisplay();
              oled.drawBitmap(32, 0, loadingOLED[i], FRAME_WIDTH_64, FRAME_HEIGHT_64, 1);
              oled.display();
              delay(FRAME_DELAY/4);
            }
          }
          screenOLED = SCREEN4;
          break;
        case SCREEN1:
          for(int j = 0; j < 2 && enableShow1 == ENABLE; j++) {
            for(int i = 0; i < FRAME_COUNT_hiOLED && enableShow1 == ENABLE; i++) {
              oled.clearDisplay();
              oled.setTextSize(1);
              oled.setCursor(40, 22);
              oled.print(OLED_STRING1);
              oled.setTextSize(1);
              oled.setCursor(40, 42);
              oled.print(OLED_STRING2);
              clearRectangle(0, 0, 32, 64);
              oled.drawBitmap(0, 16, hiOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY);
            }
            oled.display();
            delay(100);
          }
          break;
        case SCREEN2:
          for(int j = 0; j < 2 && enableShow2 == ENABLE; j++) {
            for(int i = 0; i < FRAME_COUNT_keyOLED && enableShow2 == ENABLE; i++) {
              oled.clearDisplay();
              oled.setTextSize(1);
              oled.setCursor(40, 22);
              oled.print(OLED_STRING1);
              oled.setTextSize(1);
              oled.setCursor(40, 42);
              oled.print(OLED_STRING2);
              clearRectangle(0, 0, 32, 64);
              oled.drawBitmap(0, 16, keyOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY);
            }
            oled.display();
            delay(100);
          }
          break;
        case SCREEN3:
          for(int j = 0; j < 2 && enableShow3 == ENABLE; j++) {
            for(int i = 0; i < FRAME_COUNT_lockOpenOLED && enableShow3 == ENABLE; i++) {
              oled.clearDisplay();
              oled.setTextSize(1);
              oled.setCursor(40, 22);
              oled.print(OLED_STRING1);
              oled.setTextSize(1);
              oled.setCursor(40, 42);
              oled.print(OLED_STRING2);
              clearRectangle(0, 0, 32, 64);
              oled.drawBitmap(0, 16, lockOpenOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY);
            }
            oled.display();
            delay(100);
          }
          break; 
        case SCREEN4:
          oled.clearDisplay();
          oled.setTextSize(1);
          oled.setCursor(40, 5);
          oled.print("WIFI");
          oled.setTextSize(1.5);
          oled.setCursor(40, 17);
          oled.print("Dang ket noi..");
          for(int i = 0; i < FRAME_COUNT_wifiOLED; i++) {
            clearRectangle(0, 0, 32, 32);
            oled.drawBitmap(0, 0, wifiOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.display();
            delay(FRAME_DELAY);
          }
          break;
        case SCREEN5:
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.print("WIFI");
            oled.setTextSize(1.5);
            oled.setCursor(40, 17);
            oled.print("Mat ket noi.");
            oled.drawBitmap(0, 0, wifiOLED[FRAME_COUNT_wifiOLED - 1 ], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.drawLine(31, 0 , 0, 31 , 1);
            oled.drawLine(32, 0 , 0, 32 , 1);
            oled.display();
            delay(2000);
            screenOLED = SCREEN9;
          break;
        case SCREEN6:
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.print("WIFI");
            oled.setTextSize(1.5);
            oled.setCursor(40, 17);
            oled.print("Da ket noi.");
            oled.drawBitmap(0, 0, wifiOLED[FRAME_COUNT_wifiOLED - 1 ], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);

            oled.setTextSize(1);
            oled.setCursor(40, 34);
            oled.print("BLYNK");
            oled.setTextSize(1.5);
            oled.setCursor(40, 51);
            oled.print("Dang ket noi..");
            for(int i = 0; i < FRAME_COUNT_blynkOLED; i++) {
              clearRectangle(0, 32, 32, 64);
              oled.drawBitmap(0, 32, blynkOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY);
            }
          break;
        case SCREEN7:
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.print("WIFI");
            oled.setTextSize(1.5);
            oled.setCursor(40, 17);
            oled.print("Da ket noi.");
            oled.drawBitmap(0, 0, wifiOLED[FRAME_COUNT_wifiOLED - 1 ], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.setTextSize(1);
            oled.setCursor(40, 34);
            oled.print("BLYNK");
            oled.setTextSize(1.5);
            oled.setCursor(40, 51);
            oled.print("Da ket noi.");
            oled.drawBitmap(0, 32, blynkOLED[FRAME_COUNT_wifiOLED/2], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.display();
            delay(2000);
            screenOLED = SCREEN1;
          break;
        case SCREEN8:
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.print("WIFI");
            oled.setTextSize(1.5);
            oled.setCursor(40, 17);
            oled.print("Da ket noi.");
            oled.drawBitmap(0, 0, wifiOLED[FRAME_COUNT_wifiOLED - 1 ], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);

            oled.setTextSize(1);
            oled.setCursor(40, 34);
            oled.print("BLYNK");
            oled.setTextSize(1.5);
            oled.setCursor(40, 51);
            oled.print("Mat ket noi.");
            oled.drawBitmap(0, 32, blynkOLED[FRAME_COUNT_wifiOLED/2], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
            oled.drawLine(31, 32 , 0, 63 , 1);
            oled.drawLine(32, 32 , 0, 64 , 1);
            oled.display();
            delay(2000);
            screenOLED = SCREEN9;
          break;
        case SCREEN9:
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 5);
            oled.setTextSize(1);
            oled.print("Ket noi Wifi:");
            oled.setCursor(40, 17);
            oled.setTextSize(1);
            oled.print("ESP32_IOT");
            oled.setCursor(40, 38);
            oled.print("Dia chi IP:");
            oled.setCursor(40, 50);
            oled.print("192.168.4.1");

            for(int i = 0; i < FRAME_COUNT_settingOLED; i++) {
              clearRectangle(0, 0, 32, 64);
              oled.drawBitmap(0, 16, settingOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY*2);
            }
            countSCREEN9++;
            if(countSCREEN9 > 10) {
              countSCREEN9 = 0;
              screenOLED = SCREEN1;
              enableShow1 = ENABLE;
            }
            break;
          case SCREEN10:
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 20);
            oled.print(OLED_STRING3);
            oled.setTextSize(2);
            oled.setCursor(40, 32);
            oled.print("DISABLE"); 
            for(int i = 0; i < FRAME_COUNT_autoOnOLED; i++) {
              clearRectangle(0, 0, 32, 64);
              oled.drawBitmap(0, 16, autoOnOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY);
            }
            clearRectangle(40, 32, 128, 64);
            oled.setCursor(40, 32);
            oled.print("ENABLE"); 
            oled.display();   
            delay(2000);
            screenOLED = SCREEN1;
            enableShow1 = ENABLE;
            break;
          case SCREEN11:
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 20);
            oled.print(OLED_STRING3);
            oled.setTextSize(2);
            oled.setCursor(40, 32);
            oled.print("ENABLE");
            for(int i = 0; i < FRAME_COUNT_autoOffOLED; i++) {
              clearRectangle(0, 0, 32, 64);
              oled.drawBitmap(0, 16, autoOffOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
              oled.display();
              delay(FRAME_DELAY);
            }
            clearRectangle(40, 32, 128, 64);
            oled.setCursor(40, 32);
            oled.print("DISABLE"); 
            oled.display();    
            delay(2000);
            screenOLED = SCREEN1;  
            enableShow1 = ENABLE;
            break;
          case SCREEN12:
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(40, 20);
            oled.print("Gui du lieu");
            oled.setCursor(40, 32);
            oled.print("den BLYNK"); 
            for(int i = 0; i < FRAME_COUNT_sendDataOLED; i++) {
                clearRectangle(0, 0, 32, 64);
                oled.drawBitmap(0, 16, sendDataOLED[i], FRAME_WIDTH_32, FRAME_HEIGHT_32, 1);
                oled.display();
                delay(FRAME_DELAY);
            } 
            delay(1000);
            screenOLED = SCREEN1; 
            enableShow1 = ENABLE;
            break;
          case SCREEN13:
            oled.clearDisplay();
            oled.setTextSize(1);
            oled.setCursor(0, 20);
            oled.print("Khoi dong lai");
            oled.setCursor(0, 32);
            oled.print("Vui long doi ..."); 
            oled.display();
            break;
          default : 
            delay(500);
            break;
      } 
      delay(10);
  }
}

// ======================== PASSWORD LƯU EEPROM ========================
void savePASStoBUFF()
{
    uint32_t c,d;
    c = EEPROM.read(250);
    d = EEPROM.read(251);
    if(c == 255 && d == 255) {
       memcpy (password, "1111", 4);
    } else {
      password[3] =  (char) (d %10 + 48 ) ;
      password[2] =  (char) (d /10 + 48 ) ;
      password[1] =  (char) (c %10 + 48 ) ;
      password[0] =  (char) (c /10 + 48 ) ;  
    }
    Serial.println(password);
}

uint32_t convertPassToNumber()
{
     uint32_t valuee = ((uint32_t)password[3] - 48) + ((uint32_t)password[2]-48)*10 +
    ((uint32_t)password[1]-48)*100 + ((uint32_t)password[0]-48)*1000;
    Serial.println(valuee);
    return valuee;
}

void savePWtoEEP(uint32_t valuePASS)
{
    uint32_t c,d;
    d = valuePASS / 100;
    c = valuePASS % 100;

    EEPROM.write(250, d);EEPROM.commit();
    EEPROM.write(251, c);EEPROM.commit();
    Serial.println( d);
    Serial.println( c);
}

// ======================== WIFI + AP MODE ========================
void connectSTA() {
      delay(5000);
      enableShow1 = DISABLE;
      if ( Essid.length() > 1 ) {  
      Serial.println(Essid);       
      Serial.println(Epass);       
      Serial.println(Etoken);      
      Etoken = Etoken.c_str();
      WiFi.begin(Essid.c_str(), Epass.c_str());
      int countConnect = 0;
      while (WiFi.status() != WL_CONNECTED) {
          delay(500);   
          if(countConnect++  == 15) {
            Serial.println("Ket noi Wifi that bai");
            Serial.println("Kiem tra SSID & PASS");
            Serial.println("Ket noi Wifi: ESP32 de cau hinh");
            Serial.println("IP: 192.168.4.1");
            screenOLED = SCREEN5;
            digitalWrite(BUZZER, ENABLE);
            delay(2000);
            digitalWrite(BUZZER, DISABLE);
            delay(3000);
            break;
          }
          screenOLED = SCREEN4;
          delay(2000);
      }
      Serial.println("");
      if(WiFi.status() == WL_CONNECTED) {
        Serial.println("Da ket noi Wifi: ");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP()); 
        Serial.println((char*)Essid.c_str());

        screenOLED = SCREEN6;
        delay(2000);
        strcpy(BLYNK_AUTH_TOKEN,Etoken.c_str());
        
        Blynk.config(BLYNK_AUTH_TOKEN);
        blynkConnect = Blynk.connect();
        if(blynkConnect == false) {
            screenOLED = SCREEN8;
            delay(2000);
            connectAPMode(); 
        }
        else {
            Serial.println("Da ket noi BLYNK");
            enableShow1 = ENABLE;
            screenOLED = SCREEN7;
            delay(2000);
            xTaskCreatePinnedToCore(TaskBlynk, "TaskBlynk" , 1024*16 ,  NULL,  20  ,  NULL ,  1); 
            timer.setInterval(1000L, myTimer);  // gửi dữ liệu môi trường
            buzzerBeep(5);  
            return; 
        }
      }
      else {
        digitalWrite(BUZZER, ENABLE);
        delay(2000);
        digitalWrite(BUZZER, DISABLE);
        screenOLED = SCREEN9;
        connectAPMode(); 
      }
        
    }
}

void connectAPMode() {
  WiFi.softAP(ssidAP, passwordAP);  

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/data_before", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getJsonData();
    request->send(200, "application/json", json);
  });

  server.on("/post_data", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "SUCCESS");
    enableShow1 = DISABLE;
    screenOLED = SCREEN13;
    delay(5000);
    ESP.restart();
  }, NULL, getDataFromClient);

  server.begin();
}

// ======================== JSON CẤU HÌNH ========================
void getDataFromClient(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  Serial.print("get data : ");
  Serial.println((char *)data);
  JSONVar myObject = JSON.parse((char *)data);

  if(myObject.hasOwnProperty("ssid"))
    Essid = (const char*) myObject["ssid"];
  if(myObject.hasOwnProperty("pass"))
    Epass = (const char*) myObject["pass"] ;
  if(myObject.hasOwnProperty("token"))
    Etoken = (const char*) myObject["token"];

  // Khóa cửa
  if(myObject.hasOwnProperty("passDoor"))
    EpassDoor = (int) myObject["passDoor"];
  if(myObject.hasOwnProperty("timeOpenDoor"))
    EtimeOpenDoor = (int) myObject["timeOpenDoor"];
  if(myObject.hasOwnProperty("numberEnterWrong")) 
    EnumberEnterWrong = (int) myObject["numberEnterWrong"];
  if(myObject.hasOwnProperty("timeLock")) 
    EtimeLock = (int) myObject["timeLock"];

  // Môi trường
  if(myObject.hasOwnProperty("tempThreshold1"))
    EtempThreshold1 = (int) myObject["tempThreshold1"];
  if(myObject.hasOwnProperty("tempThreshold2")) 
    EtempThreshold2 = (int) myObject["tempThreshold2"];
  if(myObject.hasOwnProperty("humiThreshold1")) 
    EhumiThreshold1 = (int) myObject["humiThreshold1"];
  if(myObject.hasOwnProperty("humiThreshold2")) 
    EhumiThreshold2 = (int) myObject["humiThreshold2"];
  if(myObject.hasOwnProperty("dustThreshold1")) 
    EdustThreshold1 = (int) myObject["dustThreshold1"];
  if(myObject.hasOwnProperty("dustThreshold2")) 
    EdustThreshold2 = (int) myObject["dustThreshold2"];

  writeEEPROM();
  savePWtoEEP(EpassDoor);
  savePASStoBUFF();
}

String getJsonData() {
  JSONVar myObject;
  myObject["ssid"]  = Essid;
  myObject["pass"]  = Epass;
  myObject["token"] = Etoken;

  myObject["passDoor"]        = EpassDoor;
  myObject["timeOpenDoor"]    = EtimeOpenDoor;
  myObject["numberEnterWrong"]= EnumberEnterWrong;
  myObject["timeLock"]        = EtimeLock;

  myObject["tempThreshold1"]  = EtempThreshold1;
  myObject["tempThreshold2"]  = EtempThreshold2;
  myObject["humiThreshold1"]  = EhumiThreshold1;
  myObject["humiThreshold2"]  = EhumiThreshold2;
  myObject["dustThreshold1"]  = EdustThreshold1;
  myObject["dustThreshold2"]  = EdustThreshold2;

  String jsonData = JSON.stringify(myObject);
  return jsonData;
}

// ======================== BLYNK ========================
BLYNK_WRITE(V3) {
  buzzerBeep(2);
  Blynk.logEvent("check_data","Mat khau: " + String(password));
}
BLYNK_WRITE(V2) {
    enableShow1 = DISABLE;
    EenableChangePass = param.asInt();
    buzzerBeep(2);
    EEPROM.write(204, EenableChangePass);  EEPROM.commit();
    Serial.print("EenableChangePass : "); Serial.println(EenableChangePass);
    OLED_STRING3 = "Doi mat khau";
    if(EenableChangePass == 0) screenOLED = SCREEN11;
    else screenOLED = SCREEN10;
}
BLYNK_WRITE(V1) {
    enableShow1 = DISABLE;
    autoWarning = param.asInt();
    buzzerBeep(1);
    EEPROM.write(210, autoWarning);  EEPROM.commit();
    OLED_STRING3 = "Canh bao";
    if(autoWarning == 0) screenOLED = SCREEN11;
    else screenOLED = SCREEN10;
}
BLYNK_WRITE(V0) {
    int state = param.asInt();
    if(state == 1) {
        Serial.println("open door");
        controlLock(ENABLE);
        buzzerBeep(3);
        delay(EtimeOpenDoor*1000);
        controlLock(DISABLE);
    }    
}

// Nút kiểm tra chất lượng không khí – dùng V4
int checkAirQuality = 0;
BLYNK_WRITE(V4) {
    checkAirQuality = param.asInt();
    if(checkAirQuality == 1) {
      buzzerBeep(1);
      check_air_quality_and_send_to_blynk(false, tempValue, humiValue, dustValue);
      screenOLED = SCREEN12;
    } 
}

void TaskBlynk(void *pvParameters) {
    Blynk.virtualWrite(V1, autoWarning); 
    Blynk.virtualWrite(V2, EenableChangePass); 
    Blynk.virtualWrite(V8, soilMoistureValue); // giá trị ban đầu độ ẩm đất
    while(1) {
      Blynk.run();
      timer.run(); 
      delay(10);
    }
}

// ======================== EEPROM ========================
void readEEPROM() {
    Essid  = "";
    Epass  = "";
    Etoken = "";

    for (int i = 0; i < 32; ++i)
        Essid += char(EEPROM.read(i)); 
    for (int i = 32; i < 64; ++i)
        Epass += char(EEPROM.read(i)); 
    for (int i = 64; i < 96; ++i)
        Etoken += char(EEPROM.read(i)); 

    if (Essid.length() == 0 || Essid[0] == 0xFF) Essid = "BLK";

    EtimeOpenDoor     = EEPROM.read(200);
    EnumberEnterWrong = EEPROM.read(201);
    EtimeLock         = EEPROM.read(202) * 100 + EEPROM.read(203);
    EenableChangePass = EEPROM.read(204);
    autoWarning       = EEPROM.read(210);

    uint8_t t1  = EEPROM.read(220);
    uint8_t t2  = EEPROM.read(221);
    uint8_t h1  = EEPROM.read(222);
    uint8_t h2  = EEPROM.read(223);
    uint8_t d1h = EEPROM.read(224);
    uint8_t d1l = EEPROM.read(225);
    uint8_t d2h = EEPROM.read(226);
    uint8_t d2l = EEPROM.read(227);

    bool envBlank = (
      (t1  == 0 || t1  == 0xFF) &&
      (t2  == 0 || t2  == 0xFF) &&
      (h1  == 0 || h1  == 0xFF) &&
      (h2  == 0 || h2  == 0xFF) &&
      (d1h == 0 || d1h == 0xFF) &&
      (d1l == 0 || d1l == 0xFF) &&
      (d2h == 0 || d2h == 0xFF) &&
      (d2l == 0 || d2l == 0xFF)
    );

    if (envBlank) {
        EtempThreshold1 = 20;
        EtempThreshold2 = 32;
        EhumiThreshold1 = 40;
        EhumiThreshold2 = 75;
        EdustThreshold1 = 40;
        EdustThreshold2 = 150;
    } else {
        EtempThreshold1 = t1;
        EtempThreshold2 = t2;
        EhumiThreshold1 = h1;
        EhumiThreshold2 = h2;
        EdustThreshold1 = d1h * 100 + d1l;
        EdustThreshold2 = d2h * 100 + d2l;
    }

    printValueSetup();
}

void clearEeprom() {
    Serial.println("Clearing Eeprom");
    for (int i = 0; i < 300; ++i) 
      EEPROM.write(i, 0);
}

void writeEEPROM() {
    clearEeprom();

    for (int i = 0; i < Essid.length(); ++i)
          EEPROM.write(i, Essid[i]);  
    for (int i = 0; i < Epass.length(); ++i)
          EEPROM.write(32+i, Epass[i]);
    for (int i = 0; i < Etoken.length(); ++i)
          EEPROM.write(64+i, Etoken[i]);

    EEPROM.write(200, EtimeOpenDoor);
    EEPROM.write(201, EnumberEnterWrong);
    EEPROM.write(202, EtimeLock / 100);
    EEPROM.write(203, EtimeLock % 100);
    EEPROM.write(204, EenableChangePass);

    // Ngưỡng môi trường
    EEPROM.write(220, EtempThreshold1);
    EEPROM.write(221, EtempThreshold2);
    EEPROM.write(222, EhumiThreshold1);
    EEPROM.write(223, EhumiThreshold2);
    EEPROM.write(224, EdustThreshold1 / 100);
    EEPROM.write(225, EdustThreshold1 % 100);
    EEPROM.write(226, EdustThreshold2 / 100);
    EEPROM.write(227, EdustThreshold2 % 100);

    EEPROM.commit();

    Serial.println("write eeprom");
    delay(500);
}

void printValueSetup() {
    Serial.println("===== EEPROM SETUP =====");
    Serial.print("ssid = ");
    Serial.println(Essid);
    Serial.print("pass = ");
    Serial.println(Epass);
    Serial.print("token = ");
    Serial.println(Etoken);

    Serial.print("passDoor = ");
    Serial.println(EpassDoor);
    Serial.print("timeOpenDoor = ");
    Serial.println(EtimeOpenDoor);
    Serial.print("numberEnterWrong = ");
    Serial.println(EnumberEnterWrong);
    Serial.print("timeLock = ");
    Serial.println(EtimeLock);
    Serial.print("enableChangePass = ");
    Serial.println(EenableChangePass);

    Serial.print("autoWarning = ");
    Serial.println(autoWarning);

    Serial.print("tempThreshold1 = ");
    Serial.println(EtempThreshold1);
    Serial.print("tempThreshold2 = ");
    Serial.println(EtempThreshold2);

    Serial.print("humiThreshold1 = ");
    Serial.println(EhumiThreshold1);
    Serial.print("humiThreshold2 = ");
    Serial.println(EhumiThreshold2);

    Serial.print("dustThreshold1 = ");
    Serial.println(EdustThreshold1);
    Serial.print("dustThreshold2 = ");
    Serial.println(EdustThreshold2);
    Serial.println("========================");
}

// ======================== TASK BUTTON ========================
void TaskButton(void *pvParameters) {
    while(1) {
      handle_button(&buttonSET);
      handle_button(&buttonUP);
      handle_button(&buttonDOWN);
      delay(10);
    }
}

void button_press_short_callback(uint8_t button_id) {
    switch(button_id) {
      case BUTTON1_ID :  
        buzzerBeep(1);
        Serial.println("btSET press short");
        break;
      case BUTTON2_ID :
        buzzerBeep(1);
        Serial.println("btUP press short");
        break;
      case BUTTON3_ID :
        Serial.println("btDOWN press short");
        digitalWrite(RELAY, ENABLE);
        buzzerBeep(3);
        delay(EtimeOpenDoor*1000);
        digitalWrite(RELAY, DISABLE);
        break;  
    } 
} 

void button_press_long_callback(uint8_t button_id) {
  switch(button_id) {
    case BUTTON1_ID :
      buzzerBeep(2);  
      enableShow1 = DISABLE;
      Serial.println("btSET press long");
      screenOLED = SCREEN9;
      clearOLED();
      connectAPMode(); 
      break;
    case BUTTON2_ID :
      buzzerBeep(2);
      Serial.println("btUP press short");
      break;
    case BUTTON3_ID :
      buzzerBeep(2);
      Serial.println("btDOWN press short");
      OLED_STRING3 = "Canh bao";
      enableShow1 = DISABLE;
      autoWarning = 1 - autoWarning;
      EEPROM.write(210, autoWarning);  EEPROM.commit();
      Blynk.virtualWrite(V1, autoWarning); 
      if(autoWarning == 0) screenOLED = SCREEN11;
      else screenOLED = SCREEN10;
      break;  
  } 
} 

// ======================== BUZZER + LED ========================
void buzzerBeep(int numberBeep) {
  for(int i = 0; i < numberBeep; ++i) {
    digitalWrite(BUZZER, ENABLE);
    delay(100);
    digitalWrite(BUZZER, DISABLE);
    delay(100);
  }  
}
void buzzerWarningBeep(int time) {
    digitalWrite(BUZZER, ENABLE);
    delay(time);
    digitalWrite(BUZZER, DISABLE);
}
void blinkLED(int numberBlink) {
  for(int i = 0; i < numberBlink; ++i) {
    digitalWrite(LED, DISABLE);
    delay(300);
    digitalWrite(LED, ENABLE);
    delay(300);
  }  
}

// ======================== TASK MÔI TRƯỜNG ========================
void TaskDHT11(void *pvParameters) { 
    while(1) {
      int humi =  dht.readHumidity();
      float temp =  dht.readTemperature();
      if (isnan(humi) || isnan(temp) ) {
          Serial.println(F("Failed to read from DHT sensor!"));
          dht11ReadOK = false;
      }
      else if(humi <= 100 && temp < 100) {
          dht11ReadOK = true;
          humiValue = humi;
          tempValue = temp;
          Serial.print(F("Humidity: "));
          Serial.print(humiValue);
          Serial.print(F("%  Temperature: "));
          Serial.print(tempValue);
          Serial.print(F("°C "));
          Serial.println();
      }
      delay(3000);
    }
}

void TaskDustSensor(void *pvParameters) {
    while(1) {
      int dustValueTemp = dustSensor.read();
      if(dustValueTemp > 500)  dustValueTemp = 500;

      if(dustValueTemp >= dustValueMax) {
        dustValueMax = dustValueTemp;
        dustValue = dustValueMax;
      }
      countDust ++;
      if(countDust > 30) {
        countDust = 0;
        dustValueMax = 0;
      }
      Serial.print("Dust Density = ");
      Serial.print(dustValue);
      Serial.println(" ug/m3");
      delay(200);
    }
}

// === TASK ĐỘ ẨM ĐẤT (SOIL MOISTURE) ===
void TaskSoilMoisture(void *pvParameters) {
    while(1) {
      int raw = analogRead(SOIL_MOISTURE);
      soilMoistureValue = 4095 - raw;
      soilMoistureValue = map(soilMoistureValue, 0, 4095, 0, 100);
      Serial.print("Soil moisture = ");
      Serial.print(soilMoistureValue);
      Serial.println(" %");
      delay(1000);
    }
}

void TaskAutoWarning(void *pvParameters)  {
    delay(20000); // đợi hệ thống ổn định
    while(1) {
      if(autoWarning == 1) {
          check_air_quality_and_send_to_blynk(true, tempValue, humiValue, dustValue);
      }
      delay(10000); // 10s
    }
}

// Gửi dữ liệu cảm biến lên Blynk
void myTimer() {
    Blynk.virtualWrite(V5, tempValue);  
    Blynk.virtualWrite(V6, humiValue);
    Blynk.virtualWrite(V7, dustValue);
    Blynk.virtualWrite(V8, soilMoistureValue);// Độ ẩm đất (%)
}

/**
 * @brief Kiểm tra chất lượng không khí và gửi lên BLYNK
 *
 * @param autoMode = true: gửi event auto_warning (chỉ khi có vấn đề)
 *                     = false: gửi event check_data (báo cáo đầy đủ)
 */
void check_air_quality_and_send_to_blynk(bool autoMode, float temp, int humi, int dust) {
  if(!dht11ReadOK) return;

  String notifications = "";
  int tempIndex = 0;
  int dustIndex = 0;
  int humiIndex = 0;

  if(autoMode == false) {
    // ===== CHẾ ĐỘ THỦ CÔNG (nhấn V4) =====
    // Nhiệt độ
    if(temp < EtempThreshold1 ) tempIndex = 1;
    else if(temp >= EtempThreshold1 && temp <=  EtempThreshold2)  tempIndex = 2;
    else tempIndex = 3;
    
    // Độ ẩm không khí
    if(humi < EhumiThreshold1 ) humiIndex = 1;
    else if(humi >= EhumiThreshold1 && humi <= EhumiThreshold2)   humiIndex = 2;
    else humiIndex = 3;

    // Bụi: vẫn chia 3 mức để báo cáo khi bấm V4
    if(dust < EdustThreshold1 ) dustIndex = 1;
    else if(dust >= EdustThreshold1 && dust <= EdustThreshold2)   dustIndex = 2;
    else dustIndex = 3;
    
    // 👉 Thông báo đầy đủ + thêm độ ẩm đất
    notifications = snTemp[tempIndex] + String(temp,1) + "*C . " + 
                    snHumi[humiIndex] + String(humi) + "% . " + 
                    snDust[dustIndex] + String(dust) + "ug/m3 . " +
                    "Do am dat: " + String(soilMoistureValue) + "% . ";

    Blynk.logEvent("check_data",notifications);
  } 
  else {
    // ===== CHẾ ĐỘ TỰ ĐỘNG (TaskAutoWarning) – chỉ cảnh báo bụi > ngưỡng 2 =====
    if(temp < EtempThreshold1 )tempIndex = 1;
    else if(temp >= EtempThreshold1 && temp <=  EtempThreshold2)  tempIndex = 0;
    else tempIndex = 3;
    
    if(humi < EhumiThreshold1 ) humiIndex = 1;
    else if(humi >= EhumiThreshold1 && humi <= EhumiThreshold2)   humiIndex = 0;
    else humiIndex = 3;

    // Bụi: chỉ cảnh báo khi vượt ngưỡng 2
    dustIndex = 0;
    if(dust > EdustThreshold2) {
      dustIndex = 3;   // dùng snDust[3] = "Bui CAO: "
    }

    if(tempIndex == 0 && humiIndex == 0 && dustIndex == 0) {
      notifications = "";
    } else {
      if(tempIndex != 0)
        notifications += snTemp[tempIndex] + String(temp,1) + "*C . ";
      if(humiIndex != 0)
        notifications += snHumi[humiIndex] + String(humi) + "% . " ;
      if(dustIndex != 0)
        notifications += snDust[dustIndex] + String(dust) + "ug/m3 . " ;
      Blynk.logEvent("auto_warning",notifications);
    }
  }

  Serial.println(notifications);
}
