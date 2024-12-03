// ESP8266 WiFi 모듈 라이브러리
#include "WiFiEsp.h"

//하드웨어시리얼이 포함되어 있지 않다면 #endif 전까지의 코드를 실행
#ifndef HAVE_HWSERIAL1 
// 소프트웨어 시리얼 통신을 사용하기 위한 라이브러리 포함
#include "SoftwareSerial.h"  
SoftwareSerial Serial1(2, 3); // RX(수신), TX(송신) 핀을 2번과 3번 핀으로 설정하여 소프트웨어 시리얼 초기화
#endif

// LCD 제어 라이브러리
#include <LiquidCrystal_I2C.h>  
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C 통신을 16x2 LCD 디스플레이 객체 초기화 (I2C 주소: 0x27)

// 전역변수 영역
// LED 관련 변수
const int LED[2] = {5, 6}; // LED 핀 설정 (5: 차량 입출차 완료 LED, 6: 비상 상황 LED)

// LCD 관련 변수
char carNum[5] = {}; // 차량 번호를 저장할 배열 (최대 4자리와 끝 문자를 위한 공간)
int status = 0; // 프로그램 상태를 저장하는 변수 (0: 기본 화면, 1: 입차, 2: 출차, 3: 사용 중)
int cursor = 0; // 차량 번호 입력 시 LCD의 커서 위치를 나타내는 변수
int parkingSpot[1] = {}; // 주차 자리 상태를 저장하는 배열 (1개의 주차 자리만 처리)
int cars[1] = {}; // 주차된 차량 정보를 저장하는 배열 (1개의 차량 정보만 처리)
char inputKey; // 키보드로부터 입력된 키 값을 저장하는 변수

// 적외선 센서 관련 변수
const int sensor = 9; // 센서가 연결된 핀 번호

//WiFi 관련 변수
char ssid[] = "zhenzhu"; // WiFi 네트워크의 SSID
char pass[] = "66666666"; // WiFi 네트워크의 비밀번호
int wifiStatus = WL_IDLE_STATUS; // WiFi 상태를 저장하는 변수, 초기 상태는 WL_IDLE_STATUS
WiFiEspServer server(80); // 포트 80을 사용하는 웹 서버 객체 생성

void setup() {
  // LED 핀을 출력 모드로 설정
  for (int i = 0; i < 2; i++) pinMode(LED[i], OUTPUT);
  for (int i = 0; i < 2; i++) analogWrite(LED[i], 0); // LED 초기화 (꺼진 상태)

  // 차량 번호 초기화 (공백으로 설정)
  for (int i = 0; i < 4; i++) carNum[i] = ' ';
  
  // 센서 설정
  pinMode(sensor, INPUT); 

  //Wifi 연결 세팅
  Serial.begin(9600); // 디버깅을 위한 시리얼 통신 초기화
  Serial1.begin(9600); // ESP 모듈과 통신을 위한 시리얼 초기화
  WiFi.init(&Serial1); // ESP 모듈 초기화
  //Wifi 연결 실패 타임아웃 처리
  while (wifiStatus != WL_CONNECTED) {
      if (millis() - startAttemptTime > 10000) { // 10초 동안 시도
          Serial.println("WiFi connection failed.");
          break; // 타임아웃 후 루프 종료
      }
      wifiStatus = WiFi.begin(ssid, pass);
  }
	//Wifi 연결
  while (wifiStatus != WL_CONNECTED) { // WiFi 연결 시도
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid); // 연결 중인 SSID 출력
    wifiStatus = WiFi.begin(ssid, pass); // WiFi 연결
  }
  Serial.println("You're connected to the network"); // WiFi 연결 성공 메시지 출력
  printWifiStatus(); // WiFi 연결 상태 출력
  server.begin(); // 웹 서버 시작

  //lcd 세팅
  lcd.begin(16, 2); // LCD 크기 설정 (16x2)
  lcd.init(); // LCD 초기화
  lcd.backlight(); // LCD 백라이트 켜기
  lcd.clear(); // LCD 화면 초기화
}

void loop() {

  inPut(); // 사용자 입력 처리

  if (status == 0) wellCome(); // 기본 화면 표시
  
  // 입차 선택 상태 (차량 번호의 첫 번째 문자와 두 번째 문자가 조건에 부합할 경우)
  if (carNum[0] == '1' && carNum[1] == 'A' && status == 0) inCar();
  
  // 차량 번호 입력 후 입차 처리
  if (status == 1 && inputKey == 'D') inCar2();
  
  // 출차 선택 상태 (차량 번호의 첫 번째 문자와 두 번째 문자가 조건에 부합할 경우)
  if (carNum[0] == '2' && carNum[1] == 'A' && status == 0) outCar();
  
  // 차량 번호 입력 후 출차 처리
  if (status == 2 && inputKey == 'D') outCar2();

  sensing(); // 센서 상태 확인

  //Wifi 연결
  WiFiEspClient client = server.available(); // 클라이언트 연결 대기
  if (client) { // 클라이언트가 연결된 경우
    Serial.println("New client"); // 새 클라이언트 연결 메시지 출력
    boolean currentLineIsBlank = true;

    while (client.connected()) { // 클라이언트와 연결 상태 유지
      if (client.available()) { // 클라이언트 데이터가 수신 가능한 경우
        char c = client.read(); // 데이터를 한 글자씩 읽음
        Serial.write(c); // 읽은 데이터를 시리얼로 출력
        if (c == '\n' && currentLineIsBlank) { // 요청 끝을 감지
          Serial.println("Sending response"); // 응답을 보내는 중 메시지 출력
          client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\nRefresh: 20\r\n\r\n");
          client.print("<!DOCTYPE HTML>\r\n<html>\r\n<h1>LIGHT MONITORING</h1>\r\n");
          client.print("LIGHT: ");
          client.print("<br>\r\n</html>\r\n");
          break;
        }
        if (c == '\n') currentLineIsBlank = true; // 현재 줄이 비어 있음 표시
        else if (c != '\r') currentLineIsBlank = false; // 줄이 비어 있지 않음 표시
      }
    }
    delay(10); // 브라우저가 데이터를 받을 시간을 제공
    client.stop(); // 클라이언트 연결 종료
    Serial.println("Client disconnected"); // 클라이언트 종료 메시지 출력
  }
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID()); // 현재 연결된 WiFi SSID 출력

  IPAddress ip = WiFi.localIP(); // 현재 장치의 IP 주소 확인
  Serial.print("IP Address: ");
  Serial.println(ip); // IP 주소 출력

  Serial.println("To see this page in action, open a browser to http://");
  Serial.println(ip); // 브라우저에서 접속할 URL 출력
}

void sensing() {
  int val = digitalRead(sensor); // 센서 값 읽기
  if (val == HIGH) { // 센서가 HIGH 상태인 경우 (감지됨)
    digitalWrite(LED[1], HIGH); // LED[1] 켜기
    Serial.println("위험 감지"); // 경고 메시지 출력
  } else { // 센서가 LOW 상태인 경우 (감지되지 않음)
    digitalWrite(LED[1], LOW); // LED[1] 끄기
    Serial.println("위험 없음"); // 상태 메시지 출력
  }
  delay(2000); // 2초 지연
}

//LCD_메뉴선택 함수
void wellCome(){
	lcd.setCursor(0,0);
    lcd.print("choise menu");
    lcd.setCursor(0,1);
    lcd.print("1:in  2:out");
}

//입차 선택(1선택 후 A로 결정) 함수
void inCar(){
	lcd.clear();
    
    lcd.setCursor(0,0);
    lcd.print("input carNum");
    
    for(int i =0 ;i<2 ;i++){
      cursor--;
      carNum[i]=' ';
    }
    delay(100);
    
    for(int i =0 ;i<4 ;i++){
      Serial.println(carNum[i]);
    }
	
    status = 1;  
}

//입차-자동차 번호 입력시 실행
void inCar2(){
	carNum[4]='\0';//문자열 처리
    Serial.println(carNum); 
    
    lcd.clear(); 

    lcd.setCursor(0,0);
    lcd.print("WellCome");
    
    for(int i=0; i<1;i++){
      if(!cars[i]){
        cars[i] = i+1;
        break;
      }
      
      else {
        lcd.setCursor(0,0);
        lcd.print("spot full");

        status = 3;
      }
    }
    
    lcd.setCursor(0,1);
    lcd.print("Back: B");

    
    digitalWrite(LED[0],1);
    digitalWrite(LED[1],0);
    status = 3;
}

//출차 선택(2선택 후 A로 결정)시 실행
void outCar(){
	lcd.clear(); 
    
    lcd.setCursor(0,0);
    lcd.print("choise car");
    
    for(int i =0 ;i<2 ;i++){
      cursor--;
      carNum[i]=' ';
    }
    status = 2;
}

//출차-자동차 번호 입력시 실행
void outCar2(){
	carNum[4]='\0';//문자열 처리    
     Serial.println(carNum);   	
     lcd.clear(); 
     
     for(int i=0; i<1;i++){ 
       if(cars[i] == i+1){    
         cars[i] = 0; 
         lcd.setCursor(0,0);
         lcd.print("Bye Bye");
         break;      
       }    
       else {       
         lcd.setCursor(0,0);        
         lcd.print("nothing car");            
       }
     }
    lcd.setCursor(0,1);
    lcd.print("Back: B");
    
    
    digitalWrite(LED[0],0);
    digitalWrite(LED[1],1);
    status = 3;
}

//키보드 입력 값 받는 함수
void inPut(){
  inputKey = Serial.read();

  //자동차 번호 입력 
  if('0' <=inputKey && inputKey <='9'){
    Serial.println(inputKey);
    carNum[cursor] = inputKey;
    cursor++;
    for(int i =0; i < 5; i++)Serial.println(carNum[i]);
  }     
  
  //입차, 출차 선택
  if(inputKey =='A'){
    Serial.println(inputKey);
    carNum[cursor]='A';
    cursor++;
  }
	
  //초기화 (처음화면으로 돌아가기)
  if(inputKey =='B' && status == 3){
    for(int i=0;i<4;i++) {carNum[i]=' ';}
    cursor = 0;  
    status = 0;
    lcd.clear();
  }
  
  //번호 한칸 지우기
  if(inputKey =='C'){
    cursor--;
    carNum[cursor]=' ';
    lcd.setCursor(cursor,1);
    lcd.print(' '); 
  } 
}