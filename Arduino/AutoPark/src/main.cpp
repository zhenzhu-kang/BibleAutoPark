// ESP8266 WiFi 모듈 라이브러리
#include "WiFiEsp.h"

// 하드웨어 시리얼이 포함되어 있지 않다면 #endif 전까지의 코드를 실행
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(2, 3); // RX(수신), TX(송신) 핀을 2번과 3번으로 설정하여 소프트웨어 시리얼 초기화
#endif

// LCD 제어 라이브러리
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 16, 2); // I2C 통신을 사용하는 16x2 LCD 디스플레이 객체 초기화 (I2C 주소: 0x27)

// 전역 변수
const int LED[3] = {5, 6, 7}; // LED 핀 설정 (5: 입출차 완료 LED, 6: 비상 LED, 7: 문 제어 LED)
char carNum[5] = {}; // 차량 번호 저장 배열
int status = 0; // 현재 프로그램 상태 (0: 기본 화면, 1: 입차, 2: 출차)
int cursor = 0; // 차량 번호 입력 시 커서 위치
int cars[10] = {}; // 최대 10대 차량 정보를 저장
int maxspot = 10; // 주차장 최대 수용량
int carspot = 10; // 현재 남은 주차 자리 수
char inputKey; // 키보드 입력값 저장
const int sensor = 9; // 적외선 센서 핀
int val = 0; // 센서 값

int wifiStatus = WL_IDLE_STATUS; // WiFi 상태
WiFiEspServer server(80); // HTTP 서버
RingBuffer buf(8); //링버퍼

char ssid[] = "zhenzhu"; // WiFi SSID
char pass[] = "66666666"; // WiFi 비밀번호

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("웹 페이지를 보려면 브라우저에서 IP 주소를 입력하세요.");
}

void connectToWiFi() {
   // Wi-Fi 쉴드의 존재 확인
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);
  }

  // Wi-Fi 연결 시도
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
  }

  Serial.println("WiFi 연결 성공");
  printWifiStatus();
}

void displayEmergencyMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("비상 감지");
  lcd.setCursor(0, 1);
  lcd.print("확인 필요");
}

void sensing() {
  val = digitalRead(sensor);
  if (val == HIGH) {
    digitalWrite(LED[1], HIGH);
    Serial.println("비상 감지");
    displayEmergencyMessage();
  } else {
    digitalWrite(LED[1], LOW);
    Serial.println("정상 상태");
  }
  delay(1000);
}


//LCD_메뉴선택 함수
void wellCome(){
	lcd.setCursor(0,0);
    lcd.clear();

  	lcd.setCursor(0,0);
  	lcd.print("Car In Spot:");
  	lcd.print(carspot);

  	lcd.setCursor(0,1);
  	lcd.print("Spare spot:");
  	lcd.print(10-carspot);
  	delay(3000);  

    lcd.clear();

	  lcd.setCursor(0,0);
    lcd.print("choise menu");
    lcd.setCursor(0,1);
    lcd.print("1:in 2:out");
  	delay(3000); 
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
    
    for(int i=0; i<maxspot; i++){
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

    
    digitalWrite(LED[0],0);
  	for(int i=0;i<5;i++){
      digitalWrite(LED[1],1);
      delay(500);
      digitalWrite(LED[1],0);
  	}
  	digitalWrite(LED[1],1);
  	carspot -=1;
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
     
     for(int i=0; i<maxspot; i++){ 
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
    for(int i=0;i<5;i++){
      digitalWrite(LED[1],1);
      delay(500);
      digitalWrite(LED[1],0);
  	}
  	digitalWrite(LED[1],1);
  	carspot +=1;
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

  // WiFi 연결
  connectToWiFi();

  server.begin();

  // LCD 초기화
  lcd.begin(16, 2); // LCD 크기 설정 (16x2)
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void sendHtml(WiFiEspClient client, String url){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  String html = "";

  html += "<html><script src=\"http://www.w3schools.com/lib/w3data.js\"></script>";
  html += "<body><div w3-include-html=\""+url+"\"></div>";
  html += "<script>w3IncludeHTML();</script></body></html>";

  client.println(html);
  client.println();

}

void handleHTTP() {
  WiFiEspClient client = server.available();
  if (client) {
    Serial.println("New client");             // print a message out the serial port
    buf.init();                               // initialize the circular buffer
    while (client.connected()) {              // loop while the client's connected
      if (client.available()) {               // if there's bytes to read from the client,
        char c = client.read();               // read a byte, then
        buf.push(c); 
        if (buf.endsWith("\r\n\r\n") || buf.endsWith("GET /H")) {
          sendHtml(client,"https://raw.githubusercontent.com/zhenzhu-kang/BibleAutoPark/refs/heads/main/code/index.html");
          break;
        }



        if (buf.endsWith("GET /M")) {

            client.println("<html><body><h1>LIGHT MONITORING</h1>"); 

            light = analogRead(LIGHT_PIN);

            client.println("<p>light="+String(light)+"</p>"); 

            client.println("<p><a href=\"/H\">HOME</a></p></body></html>");

            break;

            

        }

        

        if (buf.endsWith("GET /L")) {

            client.println("<html><body><h1>Led on/off</h1>"); 

            if(ledStatus) client.println("<p><a href=\"/F\">LED ON</a></p>"); 

            else  client.println("<p><a href=\"/N\">LED OFF</a></p>"); 

            client.println("<p><a href=\"/H\">HOME</a></p></body></html>");

            break;

        }



        if (buf.endsWith("GET /N")) {

          Serial.println("Turn led ON");

          ledStatus = HIGH;

          digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)

          client.println("<html><body><h1>Led on/off</h1>"); 

          client.println("<p><a href=\"/F\">LED ON</a></p>"); 

          client.println("<p><a href=\"/H\">HOME</a></p></body></html>");

            break;

        }

       if (buf.endsWith("GET /F")) {

          Serial.println("Turn led OFF");

          ledStatus = LOW;

          digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW

           client.println("<html><body><h1>Led on/off</h1>"); 

            client.println("<p><a href=\"/N\">LED OFF</a></p>"); 

            client.println("<p><a href=\"/H\">HOME</a></p></body></html>");

            break;
        }
      }
    }
    // close the connection
    client.stop();
    Serial.println("Client disconnected");
  }
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

  // 상태별 동작
  if (status == 0) wellCome();
  if (status == 1 && inputKey == 'D') inCar2();
  if (status == 2 && inputKey == 'D') outCar2();


  handleHTTP();
}