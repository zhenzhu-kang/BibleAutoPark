#include <WiFiEsp.h>
#include <SoftwareSerial.h>

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

RingBuffer buf(64);

SoftwareSerial EspSerial(2, 3); // RX, TX 핀 설정
char ssid[] = "zhenzhu"; // WiFi SSID
char password[] = "66666666"; // WiFi 비밀번호
WiFiEspServer server(80); // HTTP 서버


void connectToWiFi() {
  // WiFi 연결
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi 모듈을 찾을 수 없습니다.");
    while (true);
  }

  Serial.print("WiFi에 연결 중...");
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi 연결 성공!");
  Serial.print("IP 주소: ");
  Serial.println(WiFi.localIP());

  server.begin(); // 서버 시작
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
        carspot -=1;
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
      digitalWrite(LED[0],1);
      delay(500);
      digitalWrite(LED[0],0);
      delay(500);
  	}
  	
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
         carspot +=1;
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
      digitalWrite(LED[0],1);
      delay(500);
      digitalWrite(LED[0],0);
      delay(500);
  	}
  	
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
    Serial.println(inputKey);
    for(int i=0;i<4;i++) {carNum[i]=' ';}
    cursor = 0;  
    status = 0;
    lcd.clear();
  }
  
  //번호 한칸 지우기
  if(inputKey =='C'){
    Serial.println(inputKey);
    cursor--;
    carNum[cursor]=' ';
    lcd.setCursor(cursor,1);
    lcd.print(' '); 
  } 
}
//                                                                                        192.168.237.216
void setup() {
 // LED 핀을 출력 모드로 설정
  for (int i = 0; i < 3; i++) pinMode(LED[i], OUTPUT);
  for (int i = 0; i < 3; i++) analogWrite(LED[i], 0); // LED 초기화 (꺼진 상태)

  // 차량 번호 초기화 (공백으로 설정)
  for (int i = 0; i < 4; i++) carNum[i] = ' ';
  
  // 센서 설정
  pinMode(sensor, INPUT); 

  //Wifi 연결 세팅
  Serial.begin(9600); // 디버깅을 위한 시리얼 통신 초기화
  EspSerial.begin(9600);
  WiFi.init(&EspSerial);   // ESP-01 초기화

  // WiFi 연결
  connectToWiFi();

  server.begin();

  // LCD 초기화
  lcd.begin(16, 2); // LCD 크기 설정 (16x2)
  lcd.init();
  lcd.backlight();
  lcd.clear();
  buf.init();  
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

  WiFiEspClient client = server.available();
  if (client) {
    Serial.println("클라이언트 연결");

    while (client.available()) {
      char c = client.read();               // read a byte, then
      buf.push(c);
      Serial.println(c);
    }
    

    // 요청 처리
    if (buf.endsWith("GET /open") >= 0) {
      digitalWrite(LED[2], HIGH);
    } 
    
    else if (buf.endsWith("GET /close") >= 0) {
      digitalWrite(LED[2], LOW);
    }

    // HTML 응답
    client.print(
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n");

    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head><title>Parking Tower</title></head>");
    client.println("<body>");
    client.println("<h1>Parking Tower</h1>");
	  client.println("<h2>Monitoring</h2>");
    client.println("<p>Parked vehicle: " + String(maxspot - carspot) + "ea</p>");
    client.println("<p>Parking available: " + String(carspot) + "ea</p>");
    client.println("<p>situation: " + String(val == HIGH ? "Emergency" : "normal") + "</p>");
    client.println("<h2>Door control</h2>");
    client.println("<p><a href=\"/open\"><button>Door Open</button></a></p>");
    client.println("<p><a href=\"/close\"><button>Door Close</button></a></p>");
    client.println("</body>");
    client.println("</html>");

    delay(1);
    client.stop();
    Serial.println("클라이언트 연결 종료");
  }
}