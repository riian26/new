#include <SoftwareSerial.h>                // Library untuk Komunikasi dengan mySerial
#include <Wire.h>
#include "RTClib.h"
#include <TimeLib.h>
#include <TimeAlarms.h>

String api = "HHOB9O81M0S05HX7";

//Pin Sensor HC-SR04
#define echoPin 2 // attach pin D2 Arduino to pin Echo of HC-SR04
#define trigPin 3 //attach pin D3 Arduino to pin Trig of HC-SR04

//interval waktu
#define interval 5 //menit

// Init the DS3231 using the hardware interface
RTC_DS3231 rtc;
DateTime nows;

// Pin RX TX mySerial - arduino
SoftwareSerial mySerial(7, 8);     //RX,TX

// defines variables
long duration; // variable for the duration of sound wave travel
int distance; // variable for the distance measurement

//Global Variable
int i;
char g;
byte a, b, c;
String filename, y, TextForSms;
float h, t;
unsigned long mulai;

void setup() {
  //Komunikasi Serial
  mySerial.begin(9600);
  Serial.begin(9600);

  // Set pin 5V HCSR 04 Sementara
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);

  //Inisialisasi HCSR04
  // Define inputs and outputs:
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //pin led
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  //rtc
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    digitalWrite(13, 1);
    while (1);
  }
  Serial.println("RTC OK!");

  //Tampilkan Waktu
  nows = rtc.now();
  Serial.print (nows.day());
  Serial.print ("/");
  Serial.print (nows.month());
  Serial.print ("/");
  Serial.print (nows.year());
  Serial.print (",");
  Serial.print (nows.hour());
  Serial.print (":");
  Serial.print (nows.minute());
  Serial.print (":");
  Serial.println (nows.second());

  //CONNECT AT
  for (a = 0; a < 10; a++) {
    b = ConnectAT(F("AT"), 200);
    if (b == 8) {
      Serial.println(F("GSM MODULE OK!!"));
      break;
    }
    if (b < 8) {
      Serial.println(F("GSM MODULE ERROR"));

      if (a == 9) {
        Serial.println(F("CONTACT CS!!!"));
        while (1) {
          digitalWrite(13, 1);
          delay(500);
          digitalWrite(13, 0);
          delay(500);
        }
      }
    }
  }

  //check operator
  ceksim();

  nows = rtc.now();
  setTime(nows.hour(), nows.minute(), nows.second(), nows.month(), nows.day(), nows.year());
  Alarm.timerRepeat(60 * interval, ambil);
  ambil();


}

void loop() {
  Alarm.delay(0);
}

void ambil() {
  nows = rtc.now();
  Serial.println("ambil data");
  //HCSR 04
  ultraread();

  //Munculkan di serial monitor
  Serial.print(nows.year());
  Serial.print('/');
  Serial.print(nows.month());
  Serial.print('/');
  Serial.print(nows.day());
  Serial.print(' ');
  Serial.print(nows.hour());
  Serial.print(':');
  Serial.print(nows.minute());
  Serial.print(':');
  Serial.print(nows.second());
  Serial.print(',');
  Serial.print(F("Distance: "));
  Serial.print(h);
  Serial.println(F(" cm"));
  Serial.flush();

  sendit();

  alarmlevel();


  if (mySerial.available())
    Serial.write(mySerial.read());
}


//Program Pengiriman
void sendit() {
  //cek gsm shield
  unsigned long waktu = millis();
  mySerial.println(F("AT+CPIN?")); //Memeriksa Pin pada GSM
  bacaserial(200);
  mySerial.println(F("AT+CREG?")); //Memeriksa Registrasi Jaringan
  bacaserial(200);
  mySerial.println(F("AT+CGATT?")); //Lampirkan atau lepas dari Layanan GPRS
  bacaserial(200);

  //memulai koneksi
  mySerial.println(F("AT+CIPSHUT")); //Nonaktifkan Koneksi
  bacaserial(200);
  mySerial.println(F("AT+CIPSTATUS")); //Menanyakan Status koneksi Saat Ini
  bacaserial(200);
  mySerial.println(F("AT+CIPMUX=0")); // Memulai Koneksi Single IP
  bacaserial(200);
  mySerial.println(F("AT+CSTT=\"internet\"")); //Memulai proses dan mengatur nama APN,
  bacaserial(500);
  mySerial.println(F("AT+CIICR"));             //Membangun Koneksi Wireless
  mySerial.flush();
  a = 0;
  while (1) {
    while (mySerial.available() > 0) {
      g = mySerial.read();
      Serial.print(g);
      if ((char)g == 'K') a = 1; break;
    }
    if (a == 1) break;
  }
  mySerial.flush();
  Serial.flush();
  delay(500);
  mySerial.println(F("AT+CIFSR"));             //Mendapatkan Alamat IP lokal
  bacaserial(5000);
  mySerial.flush();
  Serial.flush();
  mySerial.println(F("AT+CIPSPRT=0")); //0 Parameter numerik yang mengindikasikan apakah akan ada Echo setelah AT+CIPSEND
  bacaserial(200);
  mySerial.println(F("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"")); //start up the connection  api.thingspeak.com
  bacaserial(5000);
  mySerial.flush();
  Serial.flush();

  //data yang akan dikirim
  y = "";
  y = "GET https://api.thingspeak.com/update?api_key=" + api;
  y += "&field1=" + String (distance, 1);

  //pengiriman data
  //  Serial.println(y);
  mySerial.println(F("AT+CIPSEND"));        //begin send data to remote server
  delay(200);
  mySerial.println(y);                        //begin send data to remote server
  delay(200);
  mySerial.println((char)26);                   // Mengirim
  mySerial.flush();
  Serial.flush();
  bacaserial(5000);
  mySerial.flush();
  Serial.flush();
  Serial.println("\r\nSudah dikirim");
  mySerial.flush();
  Serial.flush();
  mySerial.println(F("AT+CIPSHUT"));               //Mematikan koneksi
  bacaserial(200);
  Serial.print("waktu yang dibutuhkan GSM ");
  Serial.print(millis() - waktu);
  Serial.println(" milidetik");
}

//Program Keceparan angin
void ultraread() {
  // Clears the trigPin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
}



byte ConnectAT(String cmd, int d) {
  i = 0;
  while (1) {
    mySerial.println(cmd);
    while (mySerial.available()) {
      if (mySerial.find("OK"))
        i = 8;
    }
    delay(d);
    if (i > 5) {
      break;
    }
    i++;
  }
  return i;
}

void ceksim() {
  c = 0;
cops:
  filename = "";
  Serial.println(F("AT+CREG=1"));
  mySerial.println(F("AT+CREG=1"));
  bacaserial(200);
  delay(100);

  Serial.println(F("AT+COPS?"));
  mySerial.println(F("AT+COPS?"));
  Serial.flush();
  mySerial.flush();
  delay(200);
  while (mySerial.available() > 0) {
    if (mySerial.find("+COPS:")) {
      while (mySerial.available() > 0) {
        g = mySerial.read();
        filename += g;
      }
    }
  }
  Serial.flush();
  mySerial.flush();

  a = filename.indexOf('"');
  b = filename.indexOf('"', a + 1);
  y = filename.substring(a + 1, b);
  if (y == "51089") y = "THREE";
  filename = y;
  Serial.println(y);
  Serial.flush();
  delay(100);

  //option if not register at network
  if (filename == "")  {
    c++;
    if (c == 20) {
      Serial.println(F("NO OPERATOR FOUND"));
      while (1) {
        digitalWrite(13, 1);
        delay(1000);
        digitalWrite(13, 0);
        delay(1000);
      }
    }
    goto cops;
  }
}

void bacaserial(int wait) {
  mulai = millis();
  while ((mulai + wait) > millis()) {
    while (mySerial.available() > 0) {
      g = mySerial.read();
      Serial.print(g);
    }

  }
  Serial.flush();
  mySerial.flush();
}

void alarmlevel() {
  if (mySerial.available() == 0)
  {
    // Wait a few seconds between measurements.
    delay(2000);

    //Munculkan di serial monitor
    Serial.print(F("Tinggi air: "));
    Serial.print(distance); //Masih salah belum di Install ketinggian(masih Jarak)
    Serial.println(F(" Cm"));
    Serial.flush();

    int h = distance;

    TextForSms = TextForSms + "tinggi: ";
    TextForSms.concat(h);
    TextForSms = TextForSms + " cm";
    Serial.println(TextForSms);
    delay(2000);
    TextForSms = " ";

    if ( h == 40 )
    {
      Serial.println("LOW");
      TextForSms = "Water Tank Low";
      sendSMS(TextForSms);
      delay(5000);
      TextForSms = "";
    }

    if ( h == 200 )
    {
      Serial.println("MIDDLE");
      TextForSms = "Water Tank Middle";
      sendSMS(TextForSms);
      delay(5000);
      TextForSms = "";
    }

    if ( h == 400 )
    {
      Serial.println("FULL");
      TextForSms = "Water tank full";
      sendSMS(TextForSms);
      delay(5000);
      TextForSms = "";
    }

  }
}

void sendSMS(String message)
{
  mySerial.println("AT+CMGF=1\r");                     // AT command to send SMS message
  delay(1000);
  mySerial.println("AT+CMGS = \"+6281371731161\"");  // recipient's mobile number, in international format

  delay(1000);
  mySerial.println(message);                         // message to send
  delay(1000);
  mySerial.println((char)26);                        // End AT command with a ^Z, ASCII code 26
  delay(1000);
  mySerial.println();
  delay(1000);                                     // give module time to send SMS
  // turn off module
}
