#include "WiFiEsp.h"
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(2, 3); // RX, TX
#endif

const int LED[2] = {5, 6};
char carNum[5] = {};
int status = 0; // 0(기본 화면), 1(입차), 2(출차), 3(사용 중)
int cursor = 0;
int parkingSpot[1] = {};
int cars[1] = {};
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
}

void loop() {
  sensing();
  
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