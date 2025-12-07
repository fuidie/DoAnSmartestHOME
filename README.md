BƯỚC 1:

tải tất cả các file và foulder thư viện

file TEST2 chạy riêng biệt

file TEST1 chạy chung với data_config.h, icon.h, index_html.h, mybutton.h

BƯỚC 2:

cài các thư viện cần thiết cho TEST1 bằng cách:

Tools → Manage Libraries

BƯỚC 3:

sửa bên trong file PIRandGAS1.ino ở phần đầu chỉnh sửa các thành phần:

#define BLYNK_TEMPLATE_ID "XXXX"

#define BLYNK_TEMPLATE_NAME "XXXX"

#define BLYNK_AUTH_TOKEN "XXXX"

và 

char ssid[] = "YOUR_WIFI";

char pass[] = "YOUR_PASSWORD";

BƯỚC 4: 

Mở project trong Arduino IDE

Vào Tools → Board → ESP32 Arduino → ESP32 Dev Module

Chọn cổng COM của ESP32

Nhấn Upload (→)

Đợi compile & upload hoàn tất
