#ifndef _MY_CONFIG_H_
#define _MY_CONFIG_H_

// WiFi AP mode để cấu hình
const char* ssidAP     = "ESP32_IOT";     // Tên WiFi AP Mode
const char* passwordAP = "";             // Mật khẩu AP Mode

// Thông tin WiFi & Blynk lưu trong EEPROM
String  Essid  = "";   // Tên WiFi nhà bạn
String  Epass  = "";   // Mật khẩu WiFi nhà bạn
String  Etoken = "";   // Token Blynk

// ============ CẤU HÌNH KHÓA CỬA ============
int  EtimeOpenDoor     = 3;     // Thời gian mở khóa (s)
int  EnumberEnterWrong = 5;     // Số lần nhập sai tối đa
int  EtimeLock         = 60;    // Thời gian khóa khi nhập sai N lần
int  EenableChangePass = 1;     // Cho phép đổi mật khẩu từ keypad
int  EpassDoor         = 1111;  // Mật khẩu 4 số mặc định

// ============ CẤU HÌNH NGƯỠNG MÔI TRƯỜNG ============
int  EtempThreshold1 = 20;   // Ngưỡng nhiệt độ thấp (*C)
int  EtempThreshold2 = 32;   // Ngưỡng nhiệt độ cao (*C)

int  EhumiThreshold1 = 40;   // Ngưỡng độ ẩm thấp (%)
int  EhumiThreshold2 = 75;   // Ngưỡng độ ẩm cao (%)

int  EdustThreshold1 = 40;   // Ngưỡng bụi 1 (ug/m3)
int  EdustThreshold2 = 150;  // Ngưỡng bụi 2 (ug/m3)

// Chuỗi mô tả chất lượng môi trường
String snTemp[4] = {
    "",
    "Nhiet do THAP: ",
    "Nhiet do PHU HOP: ",
    "Nhiet do CAO: "
};

String snHumi[4] = {
    "",
    "Do am THAP: ",
    "Do am PHU HOP: ",
    "Do am CAO: "
};

String snDust[4] = {
    "",
    "Bui THAP: ",
    "Bui TRUNG BINH: ",
    "Bui CAO: "
};

#endif
