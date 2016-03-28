#include "ParticleFtpClient.h"
#include "Adafruit_GPS.h"

using namespace particleftpclient;

Adafruit_GPS gps = Adafruit_GPS();
ParticleFtpClient ftp = ParticleFtpClient();

String hostname = "ftp.gtop-tech.com";
String username = "gtopagpsenduser01";
String password = "enduser080807";
String MTKfilename = "MTK7d.EPO";
int timeout = 8000;
long lastTimeout = 0;
bool streamEpo() {
  Serial.println("Connecting to FTP...");
  if (!ftp.open(hostname, timeout)) return false;
  Serial.println("Logging in with username...");
  if (!ftp.user(username)) return false;
  Serial.println("Sending password...");
  if (!ftp.pass(password)) return false;
  Serial.println("Retrieving EPO file...");
  if (!ftp.retr(MTKfilename)) return false;
  Serial.println("Setting binary mode on GPS");
  gps.startEpoUpload();
  Serial.println("Streaming data");
  char satellite_buffer[60] = { 0 };
  int pos = 0;
  int byte_count = 0;
  while (ftp.data.connected()) {
    while (ftp.data.available()) {
      lastTimeout = millis();
      satellite_buffer[pos] = ftp.data.read();
      pos++;
      byte_count++;
      if (pos == 60) {
        Serial.print("Received satellite, ");
        Serial.print(byte_count);
        Serial.println(" bytes");
        if (!gps.sendEpoSatellite(satellite_buffer)) {
          Serial.println("Couldn't write satellite");
        };
        delay(10);
        pos = 0;
      }
      if (byte_count == 53760) {
        // Downloaded enough data
        ftp.data.stop();
      }
    }
    if (millis() - lastTimeout > timeout) {
      Serial.println("FTP timed out");
      ftp.data.stop();
    }
  }
  Serial.println("EPO disconnected");
  if (byte_count != 53760) {
    Serial.println("File seems incomplete...");
  }
  Serial.println("Ending EPO Upload");
  if (!gps.endEpoUpload()) return false;
  Serial.println("Ended EPO upload");
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello friend");

  gps.begin(9600);
  delay(500);
  streamEpo();
  delay(500);

  long now = Time.now();
  if (gps.isEPOCurrent(now)) {
    Serial.println("EPO is current");
  } else {
    Serial.println("EPO is not current");
  }
  delay(500);

  gps.hint(30.29128, -97.73858, 149, Time.year(now), Time.month(now),
          Time.day(now), Time.hour(now), Time.minute(now), Time.second(now));
  delay(500);
}

void loop() {
  char c = gps.read();
  if (gps.newNMEAreceived()) {
    char* nmea = gps.lastNMEA();
    gps.parse(nmea);
    Serial.println(nmea);
    Serial.print(gps.latitude);
    Serial.print(", ");
    Serial.println(gps.longitude);
  }
  delay(1);
}
