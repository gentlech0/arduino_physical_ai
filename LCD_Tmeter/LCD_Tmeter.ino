//--------------------------------------------
// ライブラリ定義
//--------------------------------------------
// 【LCD】
#include <Adafruit_GFX.h>     // グラフィック
#include <Adafruit_ST7735.h>  // LCDドライバ
#include <SPI.h>              // SPI

#include "fine.h"            // 快適
#include "cold.h"            // 寒い
#include "hot.h"             // 暑い
#include "hum.h"             // 湿気
#include "dry.h"             // 乾燥

// 【WiFi + 温度センサ】
#include "WiFiS3.h"           // WiFi
#include "arduino_secrets.h"  // ネットワーク設定
#include <DHT.h>              // DHT22
// #include <dht_nonblocking.h>  // DHT11

//--------------------------------------------
// 宣言
//--------------------------------------------
// 【LCD】
#define TFT_CS 10     // デバイス選択(L:アクティブ)
#define TFT_RST 9     // ハードウェアリセット(L：リセット)
#define TFT_DC 8      // データ切替(H:データ、L:コマンド)

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// 【WiFi + 温度センサ】
// 温度湿度センサ「DHT22」
constexpr uint8_t dhtPin = 2;       // データピン番号
constexpr uint8_t dhtType = DHT22;  // センサ型番

DHT dht(dhtPin, dhtType); // DHT22
// DHT_nonblocking dht(DHT_SENSOR_PIN, DHT_SENSOR_TYPE); // DHT11

float temperature = 24.0;   // 温度
float humidity = 50.0;    // 湿度
unsigned long lastRead = 0;

// WiFi設定
char ssid[] = SECRET_WIFI_SSID; // SSID
char pass[] = SECRET_WIFI_PASS; // password
int keyIndex = 0;               // key index number (needed only for WEP)

int ipLastOctet;

// int led =  LED_BUILTIN;       // 内部LED(13ピン)
int status = WL_IDLE_STATUS;  // アイドル状態
WiFiServer server(80);        // インスタンス、ポート80番を使用

// プロトタイプ宣言--------
void printWifiStatus();
void drawLabels();
void drawValues(float temperature, float humidity);
//--------------------------

void setup() {

  //【LCDセットアップ】--------------------------
  SPI.begin();          // SPI初期化
  Serial.begin(115200); // シリアル通信初期化  
  // pinMode(led, OUTPUT); // 内部LEDを出力

  // ハードリセット
  // pinMode(TFT_RST, OUTPUT);
  // digitalWrite(TFT_RST, LOW);
  // delay(50);
  // digitalWrite(TFT_RST, HIGH);
  // delay(150);  
  
  // 初期化
  tft.initR(INITR_144GREENTAB);     // 初期化コマンド
  tft.fillScreen(ST7735_WHITE);     // 背景色

  // 90度回転
  // tft.setRotation(3);

  //【WiFiセットアップ】--------------------------

  // WiFiモジュールの確認:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }  

  // ファームバージョン確認
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }  

  // 接続のトライ
  while (status != WL_CONNECTED) {
    Serial.print("Connecting to: ");
    Serial.println(ssid);       // SSIDの表示;
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }  

  server.begin();      // 接続先のデータを読み取る
  printWifiStatus();   // 接続状態 

  drawLabels();                     // ラベルの描画
  dht.begin();                      // DHT22    

}

//---------------------------------------------------------------------
// メインループ
//---------------------------------------------------------------------
void loop() {

  // 2秒に一回更新
  if (millis() - lastRead > 2000ul) {
      temperature = dht.readTemperature();  // ℃
      humidity = dht.readHumidity();      // %

      // 読取失敗
      if (isnan(temperature) || isnan(humidity)) {
          Serial.println("Read failed");
      } else {
          lastRead = millis();
          Serial.print("temperature: "); Serial.println(temperature);
          Serial.print("humidity: "); Serial.println(humidity);

          // LCD描画にも反映
          // drawValues(temperature, humidity);
      }
  }

  drawValues(temperature, humidity);  // 数値更新
  delay(5000);            // インターバル

  // クライアント準備状態
  WiFiClient client = server.available();
  if(!client) return;

  String currentLine = "";

  // クライアント接続状態
  while (client.connected()) {
    if(!client.available()) continue; // 準備待ち

    char c = client.read();   // 1文字読み出し
    Serial.write(c);          // シリアル表示

    // 改行文字の場合
    if (c == '\n') { 

      // ==============================
      // ① 温度API  (/temperature)
      // ==============================
      if (currentLine.startsWith("GET /temperature")) {

        // while (!dht_sensor.measure(&temperature, &humidity)) {
        //   delay(1000);
        // }

        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/plain");
        client.println();
        client.println(temperature);
        break;
      }      

      // ==============================
      // ② グラフページ (/)
      // ==============================
      if (currentLine.length() == 0) {

        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println();

        // 改行(\n)とダブルクォーテーション(\)の変換不要
        client.println(R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Temperature Realtime</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
<h2>Temperature Realtime Graph</h2>
<canvas id="chart"></canvas>

<script>
let temperatures = [];
let labels = [];

const ctx = document.getElementById('chart').getContext('2d');

const chart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: labels,
    datasets: [{
      label: 'temperature (C)',
      data: temperatures,
      tension: 0.2
    }]
  },
  options: {
    animation: false,
    scales: {
      y: { beginAtZero: false }
    }
  }
});

async function updatetemperature() {
  try {
    const res = await fetch('/temperature'); // 「/temperature」の返答を受信
    const t = await res.text();       // 数値を抽出

    const now = new Date().toLocaleTimeString();  // 時刻

    temperatures.push(parseFloat(t));
    console.log(temperatures);
    labels.push(now);

    if (temperatures.length > 30) {
      temperatures.shift();
      labels.shift();
    }

    chart.update();

  } catch (e) {
    console.log("fetch error");
  }
}

setInterval(updatetemperature, 5000);
</script>

</body>
</html>
)rawliteral");

        break;
      }

      currentLine = "";
    }
    else if (c != '\r') {
      currentLine += c;
    }
  }      

  // 接続を切る
  client.stop();
  Serial.println("client disconnected");  
  
}

// ラベルの描画(省略)
void drawLabels(){
  tft.setTextColor(ST7735_BLACK);
  tft.setTextSize(2);

  // 上段ラベル
  tft.setCursor(10,10);
  tft.print("TEMP(");
  tft.print(ipLastOctet);
  tft.print(")");
  Serial.println(ipLastOctet);  
  // tft.drawRGBBitmap(10, 10, kanji_on, 12, 16);
  // tft.drawRGBBitmap(34, 10, kanji_do, 24, 32);

  // 下段ラベル
  tft.setCursor(10,70);
  tft.print("HUMID");
  // tft.drawRGBBitmap(10, 70, kanji_situ, 24, 32);
  // tft.drawRGBBitmap(34, 70, kanji_do, 24, 32);

  tft.drawLine(0, 58, 128, 58, ST7735_BLACK);  
}

// 数値の描画
void drawValues(float temperature, float humidity){

  // 中心(x,y)=(10,30)、矩形(w,h)=(100,20)
  tft.fillRect(10, 30, 100, 20, ST7735_WHITE);
  // 中心(x,y)=(10,90)、矩形(w,h)=(100,20)
  tft.fillRect(10, 90, 100, 20, ST7735_WHITE);

  tft.setTextColor(ST7735_BLACK);
  tft.setTextSize(3);

  // テスト用
  // temperature = 22.0;   // 温度
  // humidity = 50.0;    // 湿度  

  // 温度
  tft.setCursor(10, 30);
  tft.print(temperature,1);
  tft.print("C");

  // 湿度
  tft.setCursor(10, 90);
  tft.print(humidity,0);
  tft.print("%");

  // わらびの表情
  if(temperature > 23){
    tft.drawRGBBitmap(68, 60, hot, 70, 70);
  } else if(temperature < 20){
    tft.drawRGBBitmap(68, 60, cold, 70, 70);
  } else if(humidity > 60){
    tft.drawRGBBitmap(68, 60, hum, 70, 70);
  } else if(humidity < 40){
    tft.drawRGBBitmap(68, 60, dry, 70, 70);
  } else{
    tft.drawRGBBitmap(68, 60, fine, 70, 70);
  }
}

// ネットワーク状態の表示
void printWifiStatus() {
  
  // SSIDの表示
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // IPアドレスの表示
  IPAddress ip = WiFi.localIP();
  ipLastOctet = ip[3];
  Serial.print("IP Address: ");
  Serial.println(ip);

  // 説明
  Serial.print("Open a browser: http://");
  Serial.println(ip);
}
