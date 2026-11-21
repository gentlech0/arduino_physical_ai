//--------------------------------------------
// ライブラリ定義
//--------------------------------------------

// 【LCD】
#include <Adafruit_GFX.h>     // グラフィック
#include <Adafruit_ST7735.h>  // LCDドライバ
#include <SPI.h>              // SPI
#include "fine.h"            // 笑顔

// 【WiFi + 温度センサ】
#include "WiFiS3.h"           // WiFi
#include "arduino_secrets.h"  // ネットワーク設定
#include <dht_nonblocking.h>  // DHTセンサをdelay無しで読むライブラリ
#include <DHT.h>

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
constexpr uint8_t dhtPin = 2;       // DHTセンサーが刺さっているピン
constexpr uint8_t dhtType = DHT22;  // 今回使うのはDHT22

// DHT_nonblocking dht(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);
DHT dht(dhtPin, dhtType);

// 温度、湿度
float temperature;
float humidity;
unsigned long lastRead = 0;

// WiFi設定
char ssid[] = SECRET_WIFI_SSID; // SSID
char pass[] = SECRET_WIFI_PASS; // password
int keyIndex = 0;               // key index number (needed only for WEP)

int led =  LED_BUILTIN;       // 内部LED(13ピン)
int status = WL_IDLE_STATUS;  // アイドル状態
WiFiServer server(80);        // インスタンス、ポート80番を使用

// プロトタイプ宣言
void printWifiStatus();

void setup() {

  Serial.begin(115200); // シリアル通信初期化  

  // 1.44インチ(128x128)、特定のrow/colオフセット
  SPI.begin();
  // SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));

  // ハードリセット
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);
  delay(50);
  digitalWrite(TFT_RST, HIGH);
  delay(150);  
  
  // 初期化
  tft.initR(INITR_144GREENTAB);     // 初期化コマンド
  tft.fillScreen(ST7735_WHITE);     // 背景色

  drawLabels(); // ラベル

  // DHT22 初期化
  dht.begin();  
}

void loop() {

  if (millis() - lastRead > 2500ul) {
      float temperature = dht.readTemperature(); // ℃
      float humidity    = dht.readHumidity();    // %

      if (isnan(temperature) || isnan(humidity)) {
          Serial.println("Read failed");
      } else {
          lastRead = millis();
          Serial.print("Temperature: "); Serial.println(temperature);
          Serial.print("Humidity: "); Serial.println(humidity);

          // LCD描画にも反映
          drawValues(temperature, humidity);
      }
  }

  float temp = 23.5;  // 温度
  float hum = 48.2;   // 湿度

  // 描画とインターバル
  drawValues(temp, hum);
  delay(5000);
}

// ラベルの描画(省略)
void drawLabels(){
  tft.setTextColor(ST7735_BLACK);
  tft.setTextSize(2);

  // 上段ラベル
  tft.setCursor(10,10);
  tft.print("TEMP");
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
void drawValues(float temp, float hum){

  // 中心(x,y)=(10,30)、矩形(w,h)=(100,20)
  tft.fillRect(10, 30, 100, 20, ST7735_WHITE);
  // 中心(x,y)=(10,90)、矩形(w,h)=(100,20)
  tft.fillRect(10, 90, 100, 20, ST7735_WHITE);

  tft.setTextColor(ST7735_BLACK);
  tft.setTextSize(3);

  // 温度
  tft.setCursor(10, 30);
  tft.print(temp,1);
  tft.print("C");

  // 湿度
  tft.setCursor(10, 90);
  tft.print(hum,0);
  tft.print("%");

  // わらびの表情
  tft.drawRGBBitmap(68, 60, fine, 70, 70);
}
