#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <FirebaseESP32.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <RTClib.h>
#define FIREBASE_HOST "smart-barrier-dbfea-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "cgVjj9MAKOijpOfGOx63xdY9wHijQEpnEQAaacH5"


// #define WIFI_SSID "TIK MIPA"  // Change the name of your WIFI
// #define WIFI_PASSWORD "1s@mp4i8"

#define WIFI_SSID "untan"  // Change the name of your WIFI
#define WIFI_PASSWORD ""

#define servo_pin 16
#define SS_PIN 21
#define RST_PIN 22
#define LED_G 27  //define green LED pin
#define LED_R 5   //define red LED
#define IR_PIN 15
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.
const long utcOffsetInSeconds = 25200;
Servo myServo;
FirebaseData firebase;
FirebaseJson json;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org");

// Offset waktu dalam detik (misalnya +5 jam = 5 * 3600 detik)
int offsetHours = 6;
time_t timeOffset = offsetHours * 3600;
RTC_DS3231 rtc;

String formattedDate;
String dayStamp;
String timeStamp;
String dateTime;
bool boolValue;


void logData(String name) {
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  formattedDate = timeClient.getFormattedDate();

  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);

  // Adjust time with offset
  time_t adjustedTime = timeClient.getEpochTime() + timeOffset;

  // Convert adjusted time to formatted time
  struct tm *adjustedTimeStruct = localtime(&adjustedTime);
  char formattedTime[20];
  strftime(formattedTime, sizeof(formattedTime), "%H:%M:%S", adjustedTimeStruct);

  dateTime = dayStamp + " " + String(formattedTime);
  String path = "history/";

  json.set("/datetime", dateTime);
  json.set("/name", name);

  Firebase.RTDB.pushJSON(&firebase, path, &json);
}



void setup() {
  Serial.begin(9600);  // Initiate a serial communication
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  timeClient.begin();
  timeClient.update();
  rtc.begin();
  // rtc.adjust(DateTime(F(_DATE), F(TIME_)));
  rtc.adjust(DateTime(2019, 8, 21,
                      timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds()));

  timeClient.setTimeOffset(3600);
  Serial.println();
  Serial.print("Connected ");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);


  SPI.begin();                // Initiate  SPI bus
  mfrc522.PCD_Init();         // Initiate MFRC522
  myServo.attach(servo_pin);  //servo pin
  myServo.write(90);           //servo start position
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);
  Serial.println("Put your card to the reader...");
  Serial.println();
}


void loop() {
  timeClient.update();
  // Serial.println(Firebase.RTDB.getBool(&firebase, "barrier"));
  // delay(1000);




  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    if (Firebase.RTDB.getBool(&firebase, "barrier")) {
      if (firebase.dataType() == "boolean") {
        boolValue = firebase.boolData();
        if (boolValue) {
          Serial.println("Authorized access");
          Serial.println();
          delay(500);
          digitalWrite(LED_G, HIGH);

          if (Firebase.RTDB.getBool(&firebase, "barrier")) {
            myServo.write(0);
            Firebase.RTDB.setBool(&firebase, "barrier", true);
            logData("Manual");
          }
          while (true) {
            if (digitalRead(IR_PIN) == HIGH) {
              delay(5000);
              Serial.println("masih low");
              if (digitalRead(IR_PIN) == HIGH) {
                Serial.println("Objek tidak terdeteksi selama 5 detik.");

                if (Firebase.RTDB.getBool(&firebase, "barrier")) {
                  myServo.write(90);
                  Firebase.RTDB.setBool(&firebase, "barrier", false);
                }
                digitalWrite(LED_G, LOW);
                break;
              }
            }
          }
        }
      }
    }
    return;
  }
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {

    return;
  }

  //Show UID on serial monitor
  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();

  content.toUpperCase();


  if (content.substring(1) == "91 83 8F 1B")  //change the UID of the card/cards that you want to give access
  {
    Serial.println("Authorized access");
    Serial.println();
    delay(500);
    digitalWrite(LED_G, HIGH);

    if (Firebase.RTDB.getBool(&firebase, "barrier")) {
      myServo.write(0);
      Firebase.RTDB.setBool(&firebase, "barrier", true);
      logData("Syaziman");
    }
    while (true) {
      if (digitalRead(IR_PIN) == HIGH) {
        delay(5000);
        Serial.println("masih low");
        if (digitalRead(IR_PIN) == HIGH) {
          Serial.println("Objek tidak terdeteksi selama 5 detik.");

          if (Firebase.RTDB.getBool(&firebase, "barrier")) {
            myServo.write(90);
            Firebase.RTDB.setBool(&firebase, "barrier", false);
          }
          digitalWrite(LED_G, LOW);
          break;
        }
      }
    }
  } else if (content.substring(1) == "93 AD 9B 18") {
    Serial.println("Authorized access");
    Serial.println();
    delay(500);
    digitalWrite(LED_G, HIGH);

    if (Firebase.RTDB.getBool(&firebase, "barrier")) {
      myServo.write(0);
      Firebase.RTDB.setBool(&firebase, "barrier", true);
      logData("Cindy");
    }
    while (true) {
      if (digitalRead(IR_PIN) == HIGH) {
        delay(5000);
        Serial.println("masih low");
        if (digitalRead(IR_PIN) == HIGH) {
          Serial.println("Objek tidak terdeteksi selama 5 detik.");

          if (Firebase.RTDB.getBool(&firebase, "barrier")) {
            myServo.write(90);
            Firebase.RTDB.setBool(&firebase, "barrier", false);
          }
          digitalWrite(LED_G, LOW);
          break;
        }
      }
    }
  } else {
    Serial.println(" Access denied");
    digitalWrite(LED_R, HIGH);
    delay(1000);
    digitalWrite(LED_R, LOW);
  }
}