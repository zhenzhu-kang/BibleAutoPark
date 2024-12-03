#include "WiFiEsp.h"
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
include <LiquidCrystal_I2C.h>  
SoftwareSerial Serial1(2, 3); // RX, TX

#endif
LiquidCrystal_I2C lcd(0x27, 16, 2);  
const int LED[2] = {5, 6};
char carNum[5] = {};
int status = 0; // 0(기본 화면), 1(입차), 2(출차), 3(사용 중)
int cursor = 0; //현재 글자 출력위치
int parkingSpot[1] = {}; 
int cars[1] = {}; //주차자리
char inputKey; //키보드 입력  
const int sensor = 9; // 센서 핀 번호
int light = -1; // 초기값 설정

// 와이파이 설정
char ssid[] = "zhenzhu"; // your network SSID (name)
char pass[] = "66666666"; // your network password
int wifiStatus = WL_IDLE_STATUS; // the Wifi radio's status

WiFiEspServer server(80);

void setup() {
  // LED 출력
  for (int i = 0; i < 2; i++) pinMode(LED[i], OUTPUT);
  for(int i=0;i<2;i++) analogWrite(LED[i],0);//LED 초기화

  //자동차 번호 초기화
  for(int i=0;i<4;i++) carNum[i]=' ';
  pinMode(sensor, INPUT); // 센서 입력

  // initialize serial for debugging
  Serial.begin(9600);

  // initialize serial for ESP module
  Serial1.begin(9600);

  // initialize ESP module
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);
  }

  // attempt to connect to WiFi network
  while (wifiStatus != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    wifiStatus = WiFi.begin(ssid, pass);
  }

  Serial.println("You're connected to the network");
  printWifiStatus();

  // start the web server on port 80
  server.begin();

  //lcd 설정  
  lcd.begin(16, 2); //글자 출력 설정
  lcd.init(); //초기화  
  lcd.backlight(); //배경 라이트 on
  lcd.clear(); //lcd 보드 초기화 
}

void loop() {
  //센서 감지
  sensing();

  //키보드 입력 값 받기
  inPut();
  
  if(status == 0) wellCome(); 
  //입차 선택(1선택 후 A로 결정)시 실행
  if(carNum[0] == '1' && carNum[1] == 'A' && status == 0) inCar();
  //입차-자동차 번호 입력시 실행
  if(status == 1 && inputKey =='D') inCar2(); 
  //출차 선택(2선택 후 A로 결정)시 실행
  if(carNum[0] == '2' && carNum[1] == 'A' && status == 0) outCar();
  //출차-자동차 번호 입력시 실행
  if(status == 2 && inputKey =='D') outCar2();


  WiFiEspClient client = server.available();
  if (client) {
    Serial.println("New client");
    boolean currentLineIsBlank = true;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n' && currentLineIsBlank) {
          Serial.println("Sending response");
          // send a standard http response header
          client.print(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"  
            "Refresh: 20\r\n"        // refresh the page automatically every 20 sec
            "\r\n");
          client.print("<!DOCTYPE HTML>\r\n");
          client.print("<html>\r\n");
          client.print("<h1>LIGHT MONITORING</h1>\r\n");
          client.print("LIGHT: ");
          client.print(light);
          client.print("<br>\r\n");
          client.print("</html>\r\n");
          break;
        }

        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(10);
    
    // close the connection:
    client.stop();
    Serial.println("Client disconnected");
  }
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.println();
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
  Serial.println();
}

//센서 감지
void sensing() {
  int val = digitalRead(sensor); // 센서 값 읽기
  if (val == HIGH) {
    digitalWrite(LED[0], LOW); // LED 0 끔
    digitalWrite(LED[1], HIGH); // LED 1 켜짐
    Serial.println("위험 감지");
  } else {
    digitalWrite(LED[0], HIGH); // LED 0 켜짐
    digitalWrite(LED[1], LOW); // LED 1 끔
    Serial.println("위험 없음");
  }
  delay(2000); // 1초 지연
}

//기본 화면
void wellCome(){
	lcd.setCursor(0,0);
    lcd.print("choise menu");
    lcd.setCursor(0,1);
    lcd.print("1:in  2:out");
}

//입차 선택(1선택 후 A로 결정)시 실행
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

//키보드 입력 값 받기
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