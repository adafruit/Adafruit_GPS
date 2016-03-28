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

char satelite_1[60] = {
  0xE0, 0x18, 0x04, 0x1D, 0x49, 0x01, 0x00, 0x00, 0x18, 0x00, 0x30, 0x2A, 0x66, 0x2E, 0x92,
  0xF9, 0x70, 0x18, 0xA5, 0xF9, 0x25, 0x00, 0x30, 0x2A, 0x8A, 0x13, 0x0A, 0x00, 0xE3, 0xB8,
  0x04, 0x80, 0xFF, 0xAA, 0xFF, 0x31, 0x3F, 0x77, 0x3A, 0x30, 0x2E, 0xD4, 0x6A, 0x01, 0x4E,
  0x89, 0x0D, 0xA1, 0x17, 0xA6, 0x95, 0x7A, 0x7B, 0xD0, 0x2A, 0x27, 0x93, 0xF8, 0xDD, 0xCC
};

char nmea_baud[5] = { 0x00, 0x80, 0x25, 0x00, 0x00 };

bool test_format_acknowledge_packet() {
  char expected_packet[12] = {
    0x04, 0x24, 0x0C, 0x00, 0x01, 0x00, 0xFD, 0x00, 0x03, 0xF3, 0x0D, 0x0A
  };
  char ack_packet[12] = { 0 };
  gps.format_acknowledge_packet(expected_packet, 253);
  return strncmp(expected_packet, ack_packet, 12) == 0;
}

bool test_format_packet() {
  char packet_data[5] = { 0x00, 0x80, 0x25, 0x00, 0x00 };
  char expected_packet[14] = {
    0x04, 0x24, 0x0E, 0x00, 0xFD, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x56, 0x0D, 0x0A
  };
  char packet[14] = { 0 };
  gps.format_packet(253, packet_data, 5, packet);
  return strncmp(expected_packet, packet, 14) == 0;
}

bool test_waitForPacket() {
  char expected_packet[12] = { 0 };
  gps.format_acknowledge_packet(expected_packet, 253);

  Serial.println("Send command 253");
  gps.send_binary_command(253, nmea_baud, 5);

  Serial.println("Waiting for packet: ");
  if (!gps.waitForPacket(expected_packet, 12, 5000)) {
    Serial.println("Never got packet");
    return false;
  } else {
    Serial.println("Found packet");
  }
  return true;
}

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
  if (gps.isEPOCurrent(Time.now())) {
    Serial.println("EPO is current");
  } else {
    Serial.println("EPO is not current");
  }
  gps.hint(30.29128, -97.73858, 149, 2016, 3, 28, 3, 43, 36);
  delay(500);
}

void loop() {
  char c = gps.read();
  if (gps.newNMEAreceived()) {
    Serial.println(gps.lastNMEA());
//    gps.sendCommand("$PMTK607*33");
  }
  delay(1);
}
