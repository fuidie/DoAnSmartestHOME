const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<meta charset="utf-8">
  <head>
    <title>🔒 KHÓA CỬA & 🌿 GIÁM SÁT KHÔNG KHÍ</title>
    <style>
      body {
        background-color: rgb(241, 241, 241);
        padding: 10px;
      }
      .container {     
        display: flex;
        justify-content: center; 
        align-items: center; 
        flex-direction: column;
        padding-top: 10px;
        font-family: Monospace;
      }
      .button {
        background-color: #04AA6D;
        border: none;
        color: white;
        padding: 15px 32px;
        text-align: center;
        text-decoration: none;
        display: inline-block;
        font-size: x-large;
        margin: 10px 2px;
        cursor: pointer;
        width: 30%;
        border-radius: 5px;
      }
      input[type=text], select {
        width: 100%;
        padding: 12px 20px;
        margin: 8px 0;
        display: inline-block;
        border: 1px solid #ccc;
        border-radius: 4px;
        box-sizing: border-box;
        font-size: xx-large;
        height : 85px;
      }
      input[type=number] {
        width: 30%;
        padding: 12px 20px;
        margin: 8px 2px;
        display: inline-block;
        border: 1px solid #ccc;
        border-radius: 4px;
        box-sizing: border-box;
        font-size: 30px;
        height : 85px;
      }

      .submit {
        width: 100%;
        background-color:#04AA6D;
        color: black;
        padding: 14px 20px;
        margin: 8px 0;
        border: none;
        border-radius: 4px;
        cursor: pointer;
      }

      .submit:hover, .button:hover {
        background-color: #989b98;
      }
      .container-2 {
        display: flex;
        margin-bottom: 20px;
        justify-content: space-between;
        gap : 10px;
      }

      h1 {
        text-align: center;
        margin-bottom: 40px;
        font-size: 40px;
      }
      h2 {
        font-size: 30px;
      }
      h4 {
        font-size: 20px;
      }
      p {
        font-size: 14px;
      }
      @media (min-width: 300px) and (max-width: 900px) {
        .container {     
          margin-right: 0%;
          margin-left: 0%;
        }
        body {
          background-color: aliceblue;
        }
      }
    </style>
  </head>

  <body>
    <div class="container">
      <h1>🔒 KHÓA CỬA THÔNG MINH & 🌿 TRẠM GIÁM SÁT KHÔNG KHÍ</h1>
      <div>
        <hr>
        <h2>Cấu hình WIFI</h2>
        <div>
          <h4>Tên WIFI </h4>
          <input type="text" id="ssid" name="ssid" placeholder="Your ssid.">
      
          <h4>Mật khẩu</h4>
          <input type="text" id="pass" name="pass" placeholder="Your password .">

          <h4>Mã Token Blynk</h4>
          <input type="text" id="token" name="token" placeholder="Your Token Blynk .">

          <hr>

          <!-- PHẦN 1: CẤU HÌNH KHÓA CỬA -->
          <h2>🔒 Cấu hình khóa cửa</h2>
          <div class="container-2">
            <h4>Mật khẩu 4 số</h4>
            <input type="number" id="passDoor" name="passDoor" placeholder="0~9">
          </div>
          <div class="container-2">
            <h4>Thời gian mở cửa (giây)</h4>
            <input type="number" id="timeOpenDoor" name="timeOpenDoor" min="1" placeholder="Giây">
          </div>
          <div class="container-2">
            <h4>Số lần cho phép nhập sai</h4>
            <input type="number" id="numberEnterWrong" name="numberEnterWrong" min="1" placeholder="Số lần">
          </div>
          <div class="container-2">
            <h4>Thời gian khóa khi nhập sai N lần</h4>
            <input type="number" id="timeLock" name="timeLock" min="1" placeholder="Giây">
          </div>

          <hr>

          <!-- PHẦN 2: CẤU HÌNH NGƯỠNG MÔI TRƯỜNG -->
          <h2>🌿 Cấu hình ngưỡng môi trường</h2>

          <h4>Ngưỡng nhiệt độ môi trường (*C)</h4>
          <p>🌞 Ngưỡng 1 &lt; Khoảng an toàn &lt; Ngưỡng 2</p>
          <div class="container-2">
            <input type="number" id="tempThreshold1" name="tempThreshold1" min="10" max="100" step="1" placeholder="Ngưỡng 1">
            <input type="number" id="tempThreshold2" name="tempThreshold2" min="10" max="100" step="1" placeholder="Ngưỡng 2">
          </div>

          <h4>Ngưỡng độ ẩm không khí (%)</h4>
          <p>🌱 Ngưỡng 1 &lt; Khoảng an toàn &lt; Ngưỡng 2</p>
          <div class="container-2">
            <input type="number" id="humiThreshold1" name="humiThreshold1" min="10" max="100" step="1" placeholder="Ngưỡng 1">
            <input type="number" id="humiThreshold2" name="humiThreshold2" min="10" max="100" step="1" placeholder="Ngưỡng 2">
          </div>

          <h4>Ngưỡng cảm biến bụi (ug/m3)</h4>
          <p>🍁 Khoảng an toàn &lt; Ngưỡng 1 &lt; Ngưỡng 2</p>
          <div class="container-2">
            <input type="number" id="dustThreshold1" name="dustThreshold1" placeholder="Ngưỡng 1">
            <input type="number" id="dustThreshold2" name="dustThreshold2" placeholder="Ngưỡng 2">
          </div>

          <div class="container-2">
            <button class="submit" id="btnDefauld"><h4 style="font-size: 25px;">Chọn mặc định</h4></button>
            <button class="submit" id="btnSubmit"><h4 style="font-size: 25px;">Gửi</h4></button>
          </div>
        </div>
      </div>
    </div>

    <script>
      var data = {
        ssid   : "",
        pass   : "",
        token  : "",
        passDoor : "",
        timeOpenDoor : "",
        numberEnterWrong : "",
        timeLock : "",
        tempThreshold1 : "",
        tempThreshold2 : "",
        humiThreshold1 : "",
        humiThreshold2 : "",
        dustThreshold1 : "",
        dustThreshold2 : ""
      };

      const ssid   = document.getElementById("ssid");
      const pass   = document.getElementById("pass");
      const token  = document.getElementById("token");

      const passDoor        = document.getElementsByName("passDoor")[0];
      const timeOpenDoor    = document.getElementsByName("timeOpenDoor")[0];
      const numberEnterWrong= document.getElementsByName("numberEnterWrong")[0];
      const timeLock        = document.getElementsByName("timeLock")[0];

      const tempThreshold1  = document.getElementsByName("tempThreshold1")[0];
      const tempThreshold2  = document.getElementsByName("tempThreshold2")[0];
      const humiThreshold1  = document.getElementsByName("humiThreshold1")[0];
      const humiThreshold2  = document.getElementsByName("humiThreshold2")[0];
      const dustThreshold1  = document.getElementsByName("dustThreshold1")[0];
      const dustThreshold2  = document.getElementsByName("dustThreshold2")[0];

      const btnDefauld = document.getElementById("btnDefauld");

      // Lấy data ban đầu từ ESP32
      var xhttp = new XMLHttpRequest();
      xhttp.open("GET","/data_before", true);
      xhttp.send();
      xhttp.onreadystatechange = function() {
        if(xhttp.readyState == 4 && xhttp.status == 200) {
          const obj = JSON.parse(this.responseText);

          ssid.value  = obj.ssid  || "";
          pass.value  = obj.pass  || "";
          token.value = obj.token || "";

          if(obj.passDoor        !== undefined) passDoor.value        = obj.passDoor;
          if(obj.timeOpenDoor    !== undefined) timeOpenDoor.value    = obj.timeOpenDoor;
          if(obj.numberEnterWrong!== undefined) numberEnterWrong.value= obj.numberEnterWrong;
          if(obj.timeLock        !== undefined) timeLock.value        = obj.timeLock;

          if(obj.tempThreshold1  !== undefined) tempThreshold1.value  = obj.tempThreshold1;
          if(obj.tempThreshold2  !== undefined) tempThreshold2.value  = obj.tempThreshold2;
          if(obj.humiThreshold1  !== undefined) humiThreshold1.value  = obj.humiThreshold1;
          if(obj.humiThreshold2  !== undefined) humiThreshold2.value  = obj.humiThreshold2;
          if(obj.dustThreshold1  !== undefined) dustThreshold1.value  = obj.dustThreshold1;
          if(obj.dustThreshold2  !== undefined) dustThreshold2.value  = obj.dustThreshold2;

          if(!tempThreshold1.value) {
            tempThreshold1.value = 20;
            tempThreshold2.value = 32;
            humiThreshold1.value = 40;
            humiThreshold2.value = 75;
            dustThreshold1.value = 40;
            dustThreshold2.value = 150;
          }
        }
      }

      btnDefauld.addEventListener("click", function(event) {
        passDoor.value        = "0000";
        timeOpenDoor.value    = 3;
        numberEnterWrong.value= 5;
        timeLock.value        = 60;

        tempThreshold1.value  = 20;
        tempThreshold2.value  = 32;
        humiThreshold1.value  = 40;
        humiThreshold2.value  = 75;
        dustThreshold1.value  = 40;
        dustThreshold2.value  = 150;
      });

      var xhttp2 = new XMLHttpRequest();
      const btnSubmit = document.getElementById("btnSubmit"); 
      btnSubmit.addEventListener("click", () => { 
        var regex = /^\d{4}$/;

        if (regex.test(passDoor.value)) {
          data = {
            ssid   : ssid.value,
            pass   : pass.value,
            token  : token.value,
            passDoor : Number(passDoor.value),
            timeOpenDoor : Number(timeOpenDoor.value),
            numberEnterWrong : Number(numberEnterWrong.value),
            timeLock : Number(timeLock.value),
            tempThreshold1 : Number(tempThreshold1.value),
            tempThreshold2 : Number(tempThreshold2.value),
            humiThreshold1 : Number(humiThreshold1.value),
            humiThreshold2 : Number(humiThreshold2.value),
            dustThreshold1 : Number(dustThreshold1.value),
            dustThreshold2 : Number(dustThreshold2.value)
          };

          xhttp2.open("POST","/post_data", true);
          xhttp2.send(JSON.stringify(data));
          xhttp2.onreadystatechange = function() {
            if(xhttp2.readyState == 4 && xhttp2.status == 200) {
              alert("Cài đặt thành công");
            }
          }
        } else {
          alert("Mật khẩu không hợp lệ! Hãy nhập đúng 4 ký tự số từ 0 đến 9.");
        }
      });
    </script>
  </body>
</html>

)rawliteral";
