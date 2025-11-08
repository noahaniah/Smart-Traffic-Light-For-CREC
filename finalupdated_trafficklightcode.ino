// ===== Smart Traffic Light + ThingSpeak + SMS Control =====
// Board: LilyGO T-Call (ESP32 + SIM800L)
// ----------------------------------------------------------
// Lane1 = Ambulance (Emergency)
// Lane2 = VIP Convoy
// Lane3 = Normal Lane
// ----------------------------------------------------------

#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#include <HardwareSerial.h>

// ---------- CONFIG ----------
const char THINGSPEAK_API_KEY[] = "EYIIG1H5GYMVJLX6";  // Replace with your key
const char THINGSPEAK_HOST[] = "api.thingspeak.com";

const char APN[] = "internet.ng.airtel.com"; // Your APN
const char APN_USER[] = "";
const char APN_PASS[] = "";

const char ADMIN_NUMBER[] = "+2349010130398"; // Optional for reply SMS

// ---------- MODEM (T-Call) PINS ----------
#define MODEM_TX_PIN 27  // ESP32 TX -> SIM800L RX
#define MODEM_RX_PIN 26  // ESP32 RX <- SIM800L TX
#define MODEM_PWRKEY 4
#define MODEM_POWER_ON 23
#define MODEM_RST 5

HardwareSerial SerialAT(1);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

// ---------- TIMING ----------
const unsigned long TS_INTERVAL_MS = 20000UL;
unsigned long lastTsUpload = 0;

// ---------- LED PINS ----------
const int L1_R = 12, L1_Y = 13, L1_G = 14;  // Lane 1 (Ambulance)
const int L2_R = 15, L2_Y = 18, L2_G = 19;  // Lane 2 (VIP)
const int L3_R = 25, L3_Y = 32, L3_G = 33;  // Lane 3 (Normal)

// ---------- ULTRASONIC ----------
const int TRIG1 = 16, ECHO1 = 17;
const int TRIG2 = 21, ECHO2 = 22;
const int TRIG3 = 2,  ECHO3 = 36;

// ---------- IR SENSORS ----------
const int IR1 = 34;
const int IR2 = 35;
const int IR3 = 39;

// ---------- TIMINGS ----------
const unsigned long GREEN_BASE_MS = 15000UL;
const unsigned long YELLOW_MS = 3000UL;
const unsigned long EMERGENCY_GREEN_MS = 10000UL;

// ---------- STATES ----------
enum Mode { MODE_NORMAL=0, MODE_EMERGENCY, MODE_VIP };
volatile Mode currentMode = MODE_NORMAL;
volatile int modeLane = 0;

// ---------- HELPERS ----------
void powerOnModem() {
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_POWER_ON, HIGH);
  delay(1000);

  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(300);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(5000); // Allow SIM800L to boot
}

long readUltrasonic(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 30000);
  if (duration <= 0) return 999;
  return (long)(duration * 0.034 / 2.0);
}

void setLaneLights(int lane, bool red, bool yellow, bool green) {
  if (lane==1) { digitalWrite(L1_R, red); digitalWrite(L1_Y, yellow); digitalWrite(L1_G, green); }
  if (lane==2) { digitalWrite(L2_R, red); digitalWrite(L2_Y, yellow); digitalWrite(L2_G, green); }
  if (lane==3) { digitalWrite(L3_R, red); digitalWrite(L3_Y, yellow); digitalWrite(L3_G, green); }
}

void allRed() {
  setLaneLights(1, true,false,false);
  setLaneLights(2, true,false,false);
  setLaneLights(3, true,false,false);
}

// ---------- THINGSPEAK UPLOAD ----------
bool updateThingSpeak6(long d1,long d2,long d3,int ir1,int ir2,int ir3){
  if (!modem.isNetworkConnected()) modem.waitForNetwork();
  if (!client.connect(THINGSPEAK_HOST, 80)) {
    Serial.println("ThingSpeak connect failed");
    return false;
  }

  String path = String("/update?api_key=") + THINGSPEAK_API_KEY +
    "&field1=" + String(d1) + "&field2=" + String(d2) + "&field3=" + String(d3) +
    "&field4=" + String(ir1) + "&field5=" + String(ir2) + "&field6=" + String(ir3);

  client.print(String("GET ") + path + " HTTP/1.1\r\n" +
               "Host: " + THINGSPEAK_HOST + "\r\n" +
               "Connection: close\r\n\r\n");

  unsigned long t0 = millis();
  while (client.connected() && millis() - t0 < 4000) {
    while (client.available()) client.read();
  }
  client.stop();
  return true;
}

// ---------- SMS CHECK ----------
void checkIncomingSMS() {
  SerialAT.println("AT+CMGF=1");
  delay(120);
  SerialAT.println("AT+CMGL=\"REC UNREAD\"");
  delay(300);

  while (SerialAT.available()) {
    String line = SerialAT.readStringUntil('\n');
    line.trim();
    if (line.startsWith("+CMGL:")) {
      String body = SerialAT.readStringUntil('\n');
      body.trim();
      String up = body; up.toUpperCase();
      Serial.println("SMS Body: " + up);

      if (up.indexOf("EMERGENCY") >= 0 || up.indexOf("AMB") >= 0) {
        currentMode = MODE_EMERGENCY; modeLane = 1;
        Serial.println("Mode -> EMERGENCY (Lane 1)");
      } else if (up.indexOf("VIP") >= 0 || up.indexOf("CONVOY") >= 0) {
        currentMode = MODE_VIP; modeLane = 2;
        Serial.println("Mode -> VIP (Lane 2)");
      } else if (up.indexOf("NORMAL") >= 0 || up.indexOf("RESUME") >= 0) {
        currentMode = MODE_NORMAL; modeLane = 0;
        Serial.println("Mode -> NORMAL");
      } else {
        Serial.println("SMS: unrecognized command");
      }
      SerialAT.println("AT+CMGD=1,3");
      delay(200);
    }
  }
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== Smart Traffic Light (T-Call) ===");

  int lights[] = {L1_R,L1_Y,L1_G, L2_R,L2_Y,L2_G, L3_R,L3_Y,L3_G};
  for (int i=0;i<9;i++) pinMode(lights[i], OUTPUT);
  allRed();

  pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
  pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
  pinMode(TRIG3, OUTPUT); pinMode(ECHO3, INPUT);
  pinMode(IR1, INPUT); pinMode(IR2, INPUT); pinMode(IR3, INPUT);

  Serial.println("Powering modem...");
  powerOnModem();
  SerialAT.begin(9600, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
  delay(5000); // Allow modem boot time

  Serial.println("Initializing modem...");
  if (!modem.restart()) {
    Serial.println("⚠️  Modem restart failed - check SIM/power");
  } else {
    Serial.print("Modem Info: "); Serial.println(modem.getModemInfo());
  }

  Serial.println("Waiting for network...");
  if (modem.waitForNetwork(60000L)) Serial.println("✅ Network connected");
  else Serial.println("❌ Network NOT found - SMS/data may fail");

  if (modem.gprsConnect(APN, APN_USER, APN_PASS))
    Serial.println("✅ GPRS connected");
  else
    Serial.println("❌ GPRS connect failed (but SMS still works)");
}

// ---------- MAIN LOOP ----------
void loop() {
  long d1 = readUltrasonic(TRIG1, ECHO1);
  long d2 = readUltrasonic(TRIG2, ECHO2);
  long d3 = readUltrasonic(TRIG3, ECHO3);
  int ir1 = digitalRead(IR1);
  int ir2 = digitalRead(IR2);
  int ir3 = digitalRead(IR3);

  Serial.printf("Distances: L1=%ldcm L2=%ldcm L3=%ldcm  |  IR: %d %d %d\n", d1,d2,d3, ir1,ir2,ir3);

  if (millis() - lastTsUpload >= TS_INTERVAL_MS) {
    Serial.println("Uploading to ThingSpeak...");
    bool ok = updateThingSpeak6(d1,d2,d3, ir1,ir2,ir3);
    if (ok) Serial.println("✅ ThingSpeak upload OK");
    else Serial.println("❌ ThingSpeak upload failed");
    lastTsUpload = millis();
  }

  checkIncomingSMS();

  if (currentMode == MODE_EMERGENCY) {
    Serial.println("🚑 EMERGENCY: Lane 1 GREEN");
    allRed();
    setLaneLights(1, false,false,true);
    delay(EMERGENCY_GREEN_MS);
  } else if (currentMode == MODE_VIP) {
    Serial.println("🚓 VIP: Lane 2 GREEN");
    allRed();
    setLaneLights(2, false,false,true);
    delay(EMERGENCY_GREEN_MS);
  } else {
    Serial.println("Lane1 GREEN");
    allRed(); setLaneLights(1,false,false,true); delay(GREEN_BASE_MS);
    Serial.println("Lane1 YELLOW");
    setLaneLights(1,false,true,false); delay(YELLOW_MS);
    setLaneLights(1,true,false,false);

    Serial.println("Lane2 GREEN");
    setLaneLights(2,false,false,true); delay(GREEN_BASE_MS);
    Serial.println("Lane2 YELLOW");
    setLaneLights(2,false,true,false); delay(YELLOW_MS);
    setLaneLights(2,true,false,false);

    Serial.println("Lane3 GREEN");
    setLaneLights(3,false,false,true); delay(GREEN_BASE_MS);
    Serial.println("Lane3 YELLOW");
    setLaneLights(3,false,true,false); delay(YELLOW_MS);
    setLaneLights(3,true,false,false);
  }

  delay(200);
}
