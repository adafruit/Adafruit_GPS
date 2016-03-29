// This sketch is designed for Particle.io platforms
// and is designed to demonstrate how to improve Time to First Fix
// using EPO data and time/location hinting

// It is designed to check the GPS flash for valid EPO data
// If the data is invalid, it will download a 7-day EPO file
// from an FTP server (apporoximately 53 kilobytes) and stream it
// directly to the GPS unit

// After that, it sends the time and location hints

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

#define STATE_SPINNING_UP -1
#define STATE_GET_EPO_STATUS 0
#define STATE_WAIT_FOR_EPO_STATUS 1
#define STATE_STREAM_EPO 2
#define STATE_SEND_TIME_HINT 3
#define STATE_WAIT_FOR_TIME_HINT_SUCCESS 4
#define STATE_SEND_LOCATION_HINT 5
#define STATE_WAIT_FOR_LOCATION_HINT_SUCCESS 6
#define STATE_NOW_PROFIT_JUST_KIDDING 7
#define NMEA_UPDATES_BEFORE_RETRY 5

int state = STATE_SPINNING_UP;
int counter = 0;

// Connect with ParticleFtpClient
bool streamEpo() {
  Serial.println("Streaming EPO data from FTP directly to GPS");
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

// Send a time hint
void sendTimeHint() {
  counter = 0;
  long now = Time.now();
  Serial.println("Sending time hint");
  gps.sendTimeHint(Time.year(now), Time.month(now),
          Time.day(now), Time.hour(now), Time.minute(now), Time.second(now));
  delay(100);
}

// Send a location hint
void sendGpsHint() {
  counter = 0;
  long now = Time.now();
  Serial.println("Sending GPS hint");
  gps.sendLocationHint(30.29128, -97.73858, 149, Time.year(now), Time.month(now),
          Time.day(now), Time.hour(now), Time.minute(now), Time.second(now));
  delay(100);
}

// Request the starting and ending time of the EPO data currently in flash
void sendEpoRequest() {
  counter = 0;
  Serial.println("Sending EPO data request");
  gps.sendEpoDataRequest();
  delay(100);
}

// The program runs using a state machine. Since the GPS will only send
// acknowledgements occassionally (if at all) and only during an NMEA update,
// the state machine will wait for a number of NMEA updates to pass without
// getting the desired response before retrying the current request.
// The sequence of requests are:
// - allow the GPS to boot
// - request EPO info
// - stream EPO data to the GPS from FTP if the EPO is out of date
// - send a time hint
// - send a location hint
void runStateMachine() {
  switch(state) {
    case STATE_SPINNING_UP :
      Serial.print("Allowing GPS to boot: ");
      Serial.println(counter);
      if (counter > NMEA_UPDATES_BEFORE_RETRY) {
        counter = 0;
        state = STATE_GET_EPO_STATUS;
      }
      break;
    case STATE_GET_EPO_STATUS :
      sendEpoRequest();
      state = STATE_WAIT_FOR_EPO_STATUS;
      break;
    case STATE_WAIT_FOR_EPO_STATUS :
      Serial.print("Awaiting EPO status response: ");
      Serial.println(counter);
      if (gps.getEpoDataRequestResponse() == PMTK_ACK_SUCCEEDED) {
        Serial.println("Got EPO Status Response");
        if (gps.epoEndUTC < Time.now()) {
          state = STATE_STREAM_EPO;
        } else {
          state = STATE_SEND_TIME_HINT;
        }
      } else if (counter > NMEA_UPDATES_BEFORE_RETRY) {
        state = STATE_GET_EPO_STATUS;
      }
      break;
    case STATE_STREAM_EPO :
      streamEpo();
      state = STATE_SEND_TIME_HINT;
      break;
    case STATE_SEND_TIME_HINT :
      sendTimeHint();
      state = STATE_WAIT_FOR_TIME_HINT_SUCCESS;
      break;
    case STATE_WAIT_FOR_TIME_HINT_SUCCESS :
      Serial.print("Awaiting Time Hint acknowledgement: ");
      Serial.println(counter);
      if (gps.getTimeHintStatus() == PMTK_ACK_SUCCEEDED) {
        Serial.println("Successfully sent time hint");
        state = STATE_SEND_LOCATION_HINT;
      } else if (gps.getTimeHintStatus() == PMTK_ACK_FAILED) {
        state = STATE_NOW_PROFIT_JUST_KIDDING;
      } else if (counter > NMEA_UPDATES_BEFORE_RETRY) {
        Serial.println("Retrying Time Hint");
        state = STATE_SEND_TIME_HINT;
      }
      break;
    case STATE_SEND_LOCATION_HINT :
      sendGpsHint();
      state = STATE_WAIT_FOR_LOCATION_HINT_SUCCESS;
      break;
    case STATE_WAIT_FOR_LOCATION_HINT_SUCCESS :
      Serial.print("Awaiting Location Hint acknowledgement ");
      Serial.println(counter);
      if (gps.getLocationHintStatus() == PMTK_ACK_SUCCEEDED) {
        Serial.println("Successfully sent location hint");
        state = STATE_NOW_PROFIT_JUST_KIDDING;
      } else if (gps.getLocationHintStatus() == PMTK_ACK_FAILED) {
        state = STATE_NOW_PROFIT_JUST_KIDDING;
      } else if (counter > NMEA_UPDATES_BEFORE_RETRY) {
        Serial.println("Retrying Location Hint");
        state = STATE_SEND_LOCATION_HINT;
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing GPS");
  gps.begin(9600);
  delay(500);
  gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  delay(500);
  gps.sendCommand(PMTK_SET_NMEA_UPDATE_100_MILLIHERTZ);
  delay(500);
  gps.sendCommand(PGCMD_NOANTENNA);
  delay(500);

  // Clear EPO data. Also, this is a demonstration of how to use the
  // library to compute a PMTK command checksum
  char clearEpoCommand[16] = { 0 };
  sprintf(clearEpoCommand, "$PMTK127*");
  gps.writePmtkChecksum(clearEpoCommand);
  Serial.print("PMTK_CMD_CLEAR_EPO (should have checksum of 36): ");
  Serial.println(clearEpoCommand);
  gps.sendCommand(clearEpoCommand);
  delay(500);
  Serial.println("Running state loop.");
}

void loop() {
  // Read data as fast as possible
  char c = gps.read();
  // If data forms a complete NMEA message...
  if (gps.newNMEAreceived()) {
    // Get the last valid NMEA
    char* nmea = gps.lastNMEA();
    // Attempt to parse the NMEA. Don't skip this!
    if (gps.parse(nmea)) Serial.print("Recognized NMEA: ");
    Serial.println(nmea);
    // If we have a location fix, print it
    if (strstr(nmea, "GPGGA") && gps.latitude != 0.0 && gps.longitude != 0.0) {
      Serial.print("Location: ");
      Serial.print(gps.latitude);
      Serial.print(", ");
      Serial.println(gps.longitude);
    }

    // Update the state machine
    counter++;
    runStateMachine();
  }
  delay(1);
}
