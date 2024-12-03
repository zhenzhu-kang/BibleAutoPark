// ESP8266 WiFi 모듈 라이브러리
#include "WiFiEsp.h"

// 하드웨어 시리얼이 포함되어 있지 않다면 #endif 전까지의 코드를 실행
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(2, 3); // RX(수신), TX(송신) 핀을 2번과 3번으로 설정하여 소프트웨어 시리얼 초기화
#endif

// LCD 제어 라이브러리
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C 통신을 사용하는 16x2 LCD 디스플레이 객체 초기화 (I2C 주소: 0x27)

// 전역 변수
const int LED[3] = {5, 6, 7}; // LED 핀 설정 (5: 입출차 완료 LED, 6: 비상 LED, 7: 문 제어 LED)
char carNum[5] = {}; // 차량 번호 저장 배열
int status = 0; // 현재 프로그램 상태 (0: 기본 화면, 1: 입차, 2: 출차, 3: 사용 중)
int cursor = 0; // 차량 번호 입력 시 커서 위치
int cars[10] = {}; // 최대 10대 차량 정보를 저장
int maxspot = 10; // 주차장 최대 수용량
int carspot = 10; // 현재 남은 주차 자리 수
char inputKey; // 키보드 입력값 저장
const int sensor = 9; // 적외선 센서 핀
int val = 0; // 센서 값
int wifiStatus = WL_IDLE_STATUS; // WiFi 상태
WiFiEspServer server(80); // HTTP 서버

char ssid[] = "zhenzhu"; // WiFi SSID
char pass[] = "66666666"; // WiFi 비밀번호

void setup() {
  // LED 설정
  for (int i = 0; i < 3; i++) pinMode(LED[i], OUTPUT);
  for (int i = 0; i < 3; i++) digitalWrite(LED[i], LOW);

  // 센서 설정
  pinMode(sensor, INPUT);

  // WiFi 초기화
  Serial.begin(9600); // 디버깅용 시리얼 통신
  Serial1.begin(9600); // ESP8266 통신
  WiFi.init(&Serial1);

  // WiFi 연결
  unsigned long startAttemptTime = millis();
  while (wifiStatus != WL_CONNECTED) {
    if (millis() - startAttemptTime > 10000) {
      Serial.println("WiFi 연결 실패");
      break;
    }
    wifiStatus = WiFi.begin(ssid, pass);
  }
  if (wifiStatus == WL_CONNECTED) {
    Serial.println("WiFi 연결 성공");
    printWifiStatus();
  }
  server.begin();

  // LCD 초기화
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void loop() {
  sensing(); // 센서 상태 확인
  inPut(); // 사용자 입력 처리

  // 상태별 동작
  if (status == 0) wellCome();
  if (status == 1 && inputKey == 'D') inCar2();
  if (status == 2 && inputKey == 'D') outCar2();

  // HTTP 요청 처리
  WiFiEspClient client = server.available();
  if (client) {
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;

        if (c == '\n' && request.endsWith("\r\n\r\n")) {
          if (request.indexOf("GET /open") != -1) {
            Serial.println("문 열림 요청 수신");
            digitalWrite(LED[2], HIGH); // 문 열림 LED 켜기
          }
          if (request.indexOf("GET /close") != -1) {
            Serial.println("문 닫힘 요청 수신");
            digitalWrite(LED[2], LOW); // 문 닫힘 LED 끄기
          }
          sendHTMLResponse(client);
          break;
        }
      }
    }
    delay(10);
    client.stop();
    Serial.println("클라이언트 연결 종료");
  }
}

void sensing() {
  val = digitalRead(sensor);
  if (val == HIGH) {
    digitalWrite(LED[1], HIGH);
    Serial.println("비상 감지");
  } else {
    digitalWrite(LED[1], LOW);
    Serial.println("정상 상태");
  }
  delay(1000);
}

void sendHTMLResponse(WiFiEspClient client) {
  client.print("HTTP/1.1 301 Moved Permanently\r\n");
  client.print("Location: https://username.github.io/ParkingTower/\r\n"); // GitHub Pages URL
  client.print("Connection: close\r\n");
  client.print("\r\n");
}


// JSON 응답을 보내는 함수
void sendJSONResponse(WiFiEspClient client) {
  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: application/json\r\n");
  client.print("Connection: close\r\n");
  client.print("\r\n");

  client.print("{");
  client.print("\"maxspot\": ");
  client.print(maxspot);
  client.print(", ");
  client.print("\"carspot\": ");
  client.print(carspot);
  client.print(", ");
  client.print("\"status\": \"");

  // 비상 상태에 따른 조건
  if (val == 1) {
    client.print("비상");  // 비상 상태
  } else {
    client.print("정상");  // 정상 상태
  }

  client.print("\"");
  client.print("}");
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("웹 페이지를 보려면 브라우저에서 IP 주소를 입력하세요.");
}

void wellCome() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Car In Spot:");
  lcd.print(maxspot - carspot); 
  lcd.setCursor(0, 1);
  lcd.print("Spare spot:");
  lcd.print(carspot);
  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Choose menu");
  lcd.setCursor(0, 1);
  lcd.print("1: In  2: Out");
  delay(3000);
}

void inPut() {
  if (Serial.available() > 0) {
    inputKey = Serial.read();
    if (cursor < 4 && '0' <= inputKey && inputKey <= '9') {
      carNum[cursor++] = inputKey;
    }
    if (inputKey == 'A') status = 1; // 입차 선택
    if (inputKey == 'B') status = 2; // 출차 선택
    if (inputKey == 'C') { // 초기화
      for (int i = 0; i < 4; i++) carNum[i] = ' ';
      cursor = 0;
      status = 0;
      lcd.clear();
    }
  }
}

void inCar2() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wellcome");
  carspot--;
  delay(3000);
  status = 0;
}

void outCar2() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Bye Bye");
  carspot++;
  delay(3000);
  status = 0;
}