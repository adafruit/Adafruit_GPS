/***********************************
This is our GPS library

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, check license.txt for more information
All text above must be included in any redistribution
****************************************/
#if defined(SPARK)
  #include "math.h"
#else
  #ifdef __AVR__
    // Only include software serial on AVR platforms (i.e. not on Due).
    #include <SoftwareSerial.h>
  #endif
#endif
#include <Adafruit_GPS.h>
// how long are max NMEA lines to parse?
#define MAXLINELENGTH 120

// we double buffer: read one line in and leave one for the main program
volatile char line1[MAXLINELENGTH];
volatile char line2[MAXLINELENGTH];
// our index into filling the current line
volatile uint8_t lineidx=0;
// pointers to the double buffers
volatile char *currentline;
volatile char *lastline;
volatile boolean recvdflag;
volatile boolean inStandbyMode;


boolean Adafruit_GPS::parse(char *nmea) {
  // do checksum check

  // first look if we even have one
  if (nmea[strlen(nmea)-4] == '*') {
    uint16_t sum = parseHex(nmea[strlen(nmea)-3]) * 16;
    sum += parseHex(nmea[strlen(nmea)-2]);

    // check checksum
    for (uint8_t i=1; i < (strlen(nmea)-4); i++) {
      sum ^= nmea[i];
    }
    if (sum != 0) {
      // bad checksum :(
      return false;
    }
  }
  int32_t degree;
  long minutes;
  char degreebuff[10];
  // look for a few common sentences
  if (strstr(nmea, "$GPGGA")) {
    // found GGA
    char *p = nmea;
    // get time
    p = strchr(p, ',')+1;
    float timef = atof(p);
    uint32_t time = timef;
    hour = time / 10000;
    minute = (time % 10000) / 100;
    seconds = (time % 100);

    milliseconds = fmod(timef, 1.0) * 1000;

    // parse out latitude
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      strncpy(degreebuff, p, 2);
      p += 2;
      degreebuff[2] = '\0';
      degree = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      minutes = 50 * atol(degreebuff) / 3;
      latitude_fixed = degree + minutes;
      latitude = degree / 100000 + minutes * 0.000006F;
      latitudeDegrees = (latitude-100*int(latitude/100))/60.0;
      latitudeDegrees += int(latitude/100);
    }

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      if (p[0] == 'S') latitudeDegrees *= -1.0;
      if (p[0] == 'N') lat = 'N';
      else if (p[0] == 'S') lat = 'S';
      else if (p[0] == ',') lat = 0;
      else return false;
    }

    // parse out longitude
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      strncpy(degreebuff, p, 3);
      p += 3;
      degreebuff[3] = '\0';
      degree = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      minutes = 50 * atol(degreebuff) / 3;
      longitude_fixed = degree + minutes;
      longitude = degree / 100000 + minutes * 0.000006F;
      longitudeDegrees = (longitude-100*int(longitude/100))/60.0;
      longitudeDegrees += int(longitude/100);
    }

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      if (p[0] == 'W') longitudeDegrees *= -1.0;
      if (p[0] == 'W') lon = 'W';
      else if (p[0] == 'E') lon = 'E';
      else if (p[0] == ',') lon = 0;
      else return false;
    }

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      fixquality = atoi(p);
    }

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      satellites = atoi(p);
    }

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      HDOP = atof(p);
    }

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      altitude = atof(p);
    }

    p = strchr(p, ',')+1;
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      geoidheight = atof(p);
    }
    return true;
  }
  if (strstr(nmea, "$GPRMC")) {
   // found RMC
    char *p = nmea;

    // get time
    p = strchr(p, ',')+1;
    float timef = atof(p);
    uint32_t time = timef;
    hour = time / 10000;
    minute = (time % 10000) / 100;
    seconds = (time % 100);

    milliseconds = fmod(timef, 1.0) * 1000;

    p = strchr(p, ',')+1;
    // Serial.println(p);
    if (p[0] == 'A')
      fix = true;
    else if (p[0] == 'V')
      fix = false;
    else
      return false;

    // parse out latitude
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      strncpy(degreebuff, p, 2);
      p += 2;
      degreebuff[2] = '\0';
      long degree = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      long minutes = 50 * atol(degreebuff) / 3;
      latitude_fixed = degree + minutes;
      latitude = degree / 100000 + minutes * 0.000006F;
      latitudeDegrees = (latitude-100*int(latitude/100))/60.0;
      latitudeDegrees += int(latitude/100);
    }

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      if (p[0] == 'S') latitudeDegrees *= -1.0;
      if (p[0] == 'N') lat = 'N';
      else if (p[0] == 'S') lat = 'S';
      else if (p[0] == ',') lat = 0;
      else return false;
    }

    // parse out longitude
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      strncpy(degreebuff, p, 3);
      p += 3;
      degreebuff[3] = '\0';
      degree = atol(degreebuff) * 10000000;
      strncpy(degreebuff, p, 2); // minutes
      p += 3; // skip decimal point
      strncpy(degreebuff + 2, p, 4);
      degreebuff[6] = '\0';
      minutes = 50 * atol(degreebuff) / 3;
      longitude_fixed = degree + minutes;
      longitude = degree / 100000 + minutes * 0.000006F;
      longitudeDegrees = (longitude-100*int(longitude/100))/60.0;
      longitudeDegrees += int(longitude/100);
    }

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      if (p[0] == 'W') longitudeDegrees *= -1.0;
      if (p[0] == 'W') lon = 'W';
      else if (p[0] == 'E') lon = 'E';
      else if (p[0] == ',') lon = 0;
      else return false;
    }
    // speed
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      speed = atof(p);
    }

    // angle
    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      angle = atof(p);
    }

    p = strchr(p, ',')+1;
    if (',' != *p)
    {
      uint32_t fulldate = atof(p);
      day = fulldate / 10000;
      month = (fulldate % 10000) / 100;
      year = (fulldate % 100);
    }
    // we dont parse the remaining, yet!
    return true;
  }

  // Trap for 001 PMTK_ACK (PMTK command acknowledgement) packets and
  // update lastMTKAcknowledged and lastMTKStatus
  if (strstr(nmea, "$PMTK001")) {
    char *p = nmea;
    p = strchr(p, ',') + 1;
    lastMTKAcknowledged = atoi(p);
    p = strchr(p, ',') + 1;
    lastMTKStatus = atoi(p);
    return true;
  }

  // Trap for 707 PMTK_DT_EPO_INFO (EPO data info) responses
  if (strstr(nmea, "$PMTK707")) {
    char *p = nmea;
    p = strchr(p, ',') + 1;
    p = strchr(p, ',') + 1;
    int gpsStartWeek = atoi(p);
    p = strchr(p, ',') + 1;
    int gpsStartSec = atoi(p);
    p = strchr(p, ',') + 1;
    int gpsEndWeek = atoi(p);
    p = strchr(p, ',') + 1;
    int gpsEndSec = atoi(p);
    epoStartUTC = gpsTimeToUTC(gpsStartWeek, gpsStartSec);
    epoEndUTC = gpsTimeToUTC(gpsEndWeek, gpsEndSec);

    return true;
  }

  return false;
}

// Sometimes you need to flush the input buffer
void Adafruit_GPS::flush(void) {
  #if defined(SPARK)
    Serial1.flush();
  #else
    #ifdef __AVR__
      if (gpsSwSerial) {
        gpsSwSerial->flush();
      } else
    #endif
    {
      if (gpsHwSerial) {
        gpsHwSerial->flush();
      }
    }
  #endif
}

char Adafruit_GPS::read(void) {
  char c = 0;

  if (paused) return c;
#if defined(SPARK)
  if (!Serial1.available()) return c;
  c = Serial1.read();
#else
#ifdef __AVR__
  if(gpsSwSerial) {
    if(!gpsSwSerial->available()) return c;
    c = gpsSwSerial->read();
  } else
#endif
  {
    if(!gpsHwSerial->available()) return c;
    c = gpsHwSerial->read();
  }
#endif
  //Serial.print(c);


  if (c == '$') {         //please don't eat the dollar sign - rdl 9/15/14
    currentline[lineidx] = 0;  // ^^^   ...but still needs to cycle to a new line!
                               // If a crlf byte gets lost, a new command will
                               // always begin with $. -- jyc 3/29/16

    if (currentline == line1) {
      currentline = line2;
      lastline = line1;
    } else {
      currentline = line1;
      lastline = line2;
    }
    lineidx = 0;
  }

  if (c == '\n') {
    currentline[lineidx] = 0;

    if (currentline == line1) {
      currentline = line2;
      lastline = line1;
    } else {
      currentline = line1;
      lastline = line2;
    }

    //Serial.println("----");
    //Serial.println((char *)lastline);
    //Serial.println("----");
    lineidx = 0;
    recvdflag = true;
  }

  currentline[lineidx++] = c;
  if (lineidx >= MAXLINELENGTH)
    lineidx = MAXLINELENGTH-1;

  return c;
}

#if defined(SPARK)
Adafruit_GPS::Adafruit_GPS() {
  common_init();
}
#else
  #ifdef __AVR__
    // Constructor when using SoftwareSerial or NewSoftSerial
    #if ARDUINO >= 100
      Adafruit_GPS::Adafruit_GPS(SoftwareSerial *ser)
    #else
      Adafruit_GPS::Adafruit_GPS(NewSoftSerial *ser)
    #endif
    {
      common_init();     // Set everything to common state, then...
      gpsSwSerial = ser; // ...override gpsSwSerial with value passed.
    }

    // Constructor when using HardwareSerial
    Adafruit_GPS::Adafruit_GPS(HardwareSerial *ser) {
      common_init();  // Set everything to common state, then...
      gpsHwSerial = ser; // ...override gpsHwSerial with value passed.
    }
  #endif
#endif

// Initialization code used by all constructor types
void Adafruit_GPS::common_init(void) {
  #if defined(SPARK)
  #else
    #ifdef __AVR__
      gpsSwSerial = NULL; // Set both to NULL, then override correct
    #endif
    gpsHwSerial = NULL;
  #endif
  recvdflag   = false;
  paused      = false;
  lineidx     = 0;
  currentline = line1;
  lastline    = line2;

  hour = minute = seconds = year = month = day =
    fixquality = satellites = 0; // uint8_t
  lat = lon = mag = 0; // char
  fix = false; // boolean
  milliseconds = 0; // uint16_t
  latitude = longitude = geoidheight = altitude =
    speed = angle = magvariation = HDOP = 0.0; // float
}

void Adafruit_GPS::begin(uint32_t baud)
{
  // Need to save serial baudrate for EPO NMEA/Binary mode setting
  serial_baud = baud;
#if defined(SPARK)
  Serial1.begin(baud);
#else
  #ifdef __AVR__
    if(gpsSwSerial)
      gpsSwSerial->begin(baud);
    else
      gpsHwSerial->begin(baud);
  #endif
#endif
  delay(10);
}

void Adafruit_GPS::sendCommand(const char *str) {
  lastMTKStatus = PMTK_ACK_NO_RESPONSE;
#if defined(SPARK)
  Serial1.println(str);
#else
  #ifdef __AVR__
    if(gpsSwSerial)
      gpsSwSerial->println(str);
    else
  #endif
    gpsHwSerial->println(str);
#endif
}

// Appends a PMTK checksum to a PMTK command. Format should be:
// $PMTK...*
// The checksum will be appended after the *, so there needs to be
// space for at least two characters and an additional null terminating 0
void Adafruit_GPS::writePmtkChecksum(char* command) {
  int pos = 1;
  uint8_t sum = 0;
  while (command[pos] != '*' && command[pos] != 0) {
    sum ^= (uint8_t)(command[pos]);
    pos++;
  }
  pos++;
  sprintf(&command[pos], "%02X", sum);
}

boolean Adafruit_GPS::newNMEAreceived(void) {
  return recvdflag;
}

void Adafruit_GPS::pause(boolean p) {
  paused = p;
}

char *Adafruit_GPS::lastNMEA(void) {
  recvdflag = false;
  return (char *)lastline;
}

// read a Hex value and return the decimal equivalent
uint8_t Adafruit_GPS::parseHex(char c) {
    if (c < '0')
      return 0;
    if (c <= '9')
      return c - '0';
    if (c < 'A')
       return 0;
    if (c <= 'F')
       return (c - 'A')+10;
    // if (c > 'F')
    return 0;
}

boolean Adafruit_GPS::waitForSentence(const char *wait4me, uint8_t max) {
  char str[20];

  uint8_t i=0;
  while (i < max) {
    if (newNMEAreceived()) {
      char *nmea = lastNMEA();
      strncpy(str, nmea, 20);
      str[19] = 0;
      i++;

      if (strstr(str, wait4me))
	return true;
    }
  }

  return false;
}

boolean Adafruit_GPS::LOCUS_StartLogger(void) {
  sendCommand(PMTK_LOCUS_STARTLOG);
  recvdflag = false;
  return waitForSentence(PMTK_LOCUS_STARTSTOPACK);
}

boolean Adafruit_GPS::LOCUS_StopLogger(void) {
  sendCommand(PMTK_LOCUS_STOPLOG);
  recvdflag = false;
  return waitForSentence(PMTK_LOCUS_STARTSTOPACK);
}

boolean Adafruit_GPS::LOCUS_ReadStatus(void) {
  sendCommand(PMTK_LOCUS_QUERY_STATUS);

  if (! waitForSentence("$PMTKLOG"))
    return false;

  char *response = lastNMEA();
  uint16_t parsed[10];
  uint8_t i;

  for (i=0; i<10; i++) parsed[i] = -1;

  response = strchr(response, ',');
  for (i=0; i<10; i++) {
    if (!response || (response[0] == 0) || (response[0] == '*'))
      break;
    response++;
    parsed[i]=0;
    while ((response[0] != ',') &&
	   (response[0] != '*') && (response[0] != 0)) {
      parsed[i] *= 10;
      char c = response[0];
      if (isDigit(c))
        parsed[i] += c - '0';
      else
        parsed[i] = c;
      response++;
    }
  }
  LOCUS_serial = parsed[0];
  LOCUS_type = parsed[1];
  if (isAlpha(parsed[2])) {
    parsed[2] = parsed[2] - 'a' + 10;
  }
  LOCUS_mode = parsed[2];
  LOCUS_config = parsed[3];
  LOCUS_interval = parsed[4];
  LOCUS_distance = parsed[5];
  LOCUS_speed = parsed[6];
  LOCUS_status = !parsed[7];
  LOCUS_records = parsed[8];
  LOCUS_percent = parsed[9];

  return true;
}

// Standby Mode Switches
boolean Adafruit_GPS::standby(void) {
  if (inStandbyMode) {
    return false;  // Returns false if already in standby mode, so that you do not wake it up by sending commands to GPS
  }
  else {
    inStandbyMode = true;
    sendCommand(PMTK_STANDBY);
    //return waitForSentence(PMTK_STANDBY_SUCCESS);  // don't seem to be fast enough to catch the message, or something else just is not working
    return true;
  }
}

boolean Adafruit_GPS::wakeup(void) {
  if (inStandbyMode) {
   inStandbyMode = false;
    sendCommand("");  // send byte to wake it up
    return waitForSentence(PMTK_AWAKE);
  }
  else {
      return false;  // Returns false if not in standby mode, nothing to wakeup
  }
}

// Prepares GPS for new EPO data to be uploaded to flash
// GPS will operate in binary mode after this call, so plaintext PMTK
// commands will be ignored
bool Adafruit_GPS::startEpoUpload() {
  // Clear EPO data
  sendCommand("$PMTK127*36");
  // Wait for acknowledgement of cleared data... for up to 10 seconds
  long start = millis();
  bool found = false;
  while ((millis() - start < 10000) && !found) {
    char c = read();
    if (newNMEAreceived()) parse(lastNMEA());
    if (lastMTKAcknowledged == 127 && lastMTKStatus == PMTK_ACK_SUCCEEDED) {
      found = true;
    }
    delay(1);
  }
  // Sets output to binar mode.
  set_output_format(PMTK_OUTPUT_FORMAT_BINARY);
  delay(500);
  return true;
}

// Adds 60 bytes of EPO data to the send buffer.
// This method will return true for two successive calls, and then
// initiate a packet transfer once three satellites worth of
// data are added. At this point, it will return true if all three satellites
// data were transfered successfully, or false if they require retransmitting.
// Once the first packet is received by the GPS unit, it will be in
// "EPO upload mode" and will not send any updates until a final packet is sent
bool Adafruit_GPS::sendEpoSatellite(char* data) {
  char empty_buffer[182] = { 0 };
  if (satellite_number == 0) {
    // initialize an empty satelite data packet
    format_packet(722, empty_buffer, 182, packet_buffer);
  }
  memcpy(&packet_buffer[EPO_SATELLITE_OFFSET + satellite_number * 60], data, 60);
  satellite_number++;
  if (satellite_number == 3) {
    // if the packet is full then write sequence number, compute checksum,send, then wait for acknowledgement
    if (!flush_epo_packet()) {
      return false;
    }
    epo_sequence_number++;
  }
  return true;
}

// Finishes EPO uploading and sets the GPS back to NMEA mode
bool Adafruit_GPS::endEpoUpload(void) {
  if (satellite_number != 0) {
    // Serial.println("Sending last valid satellite data");
    if (!flush_epo_packet()) return false;
  }
  // make final packet
  char empty_buffer[182] = { 0 };
  empty_buffer[0] = 0xFF;
  empty_buffer[1] = 0xFF;
  epo_sequence_number = 0xFFFF;
  format_packet(722, empty_buffer, 182, packet_buffer);
  if (!flush_epo_packet()) return false;
  // Serial.println("Sent final packet");
  set_output_format(PMTK_OUTPUT_FORMAT_NMEA);
  return true;
}

// Converts GPS Week and Time of Week data to a UTC long. Does NOT
// account for leap seconds!
long Adafruit_GPS::gpsTimeToUTC(long gpsWeek, long timeOfWeek) {
  return 315964800 + gpsWeek * 7 * 60 * 60 * 24 + timeOfWeek;
}

// Request EPO Data Info
void Adafruit_GPS::sendEpoDataRequest() {
  epoStartUTC = -1;
  epoEndUTC = -1;
  sendCommand("$PMTK607*33");
  delay(500);
}

// Sends a hint to the GPS as to what the time is, to improve TTFF
// Must be within 3 seconds of current UTC time.
void Adafruit_GPS::sendTimeHint(int YYYY, int MM, int DD, int hh, int mm, int ss) {
  lastMTKAcknowledged = 0;
  lastMTKStatus = PMTK_ACK_NO_RESPONSE;
  memset(packet_buffer, 0, EPO_PACKET_LENGTH);
  sprintf(packet_buffer, "$PMTK740,%u,%u,%u,%u,%u,%u", YYYY, MM, DD, hh, mm, ss);
  char sum = checksum(packet_buffer, 1, strlen(packet_buffer));
  sprintf(&packet_buffer[strlen(packet_buffer)], "*%02X", sum);
  sendCommand(packet_buffer);
  delay(500);
}

// Sends a hint to the GPS of the approximate location, to improve TTFF
// Must be within 30 kiliometers of actual location.
void Adafruit_GPS::sendLocationHint(float lat, float lng, int altitude, int YYYY, int MM, int DD, int hh, int mm, int ss) {
  lastMTKAcknowledged = 0;
  lastMTKStatus = PMTK_ACK_NO_RESPONSE;
  memset(packet_buffer, 0, EPO_PACKET_LENGTH);
  sprintf(packet_buffer, "$PMTK741,%f,%f,%u,%u,%u,%u,%u,%u,%u", lat, lng, altitude, YYYY, MM, DD, hh, mm, ss);
  char sum = checksum(packet_buffer, 1, strlen(packet_buffer));
  sprintf(&packet_buffer[strlen(packet_buffer)], "*%02X", sum);
  sendCommand(packet_buffer);
  delay(500);
}

// Returns a PMTK_ACK constant about the response status of an EPO info request
// Updated whenever parse() is called
int Adafruit_GPS::getEpoDataRequestResponse() {
  return (epoStartUTC > -1 && epoEndUTC > -1) ? PMTK_ACK_SUCCEEDED : PMTK_ACK_NO_RESPONSE;
}

// Returns a PMTK_ACK constant about the response status of a location hint
// Updated whenever parse() is called
int Adafruit_GPS::getLocationHintStatus() {
  if (lastMTKAcknowledged == 741) return lastMTKStatus;
  return PMTK_ACK_NO_RESPONSE;
}

// Returns a PMTK_ACK constant about the response status of a time hint
// Updated whenever parse() is called
int Adafruit_GPS::getTimeHintStatus() {
  if (lastMTKAcknowledged == 740) return lastMTKStatus;
  return PMTK_ACK_NO_RESPONSE;
}

// Sets the output format of the GPS to either PMTK_OUTPUT_FORMAT_BINARY or
// PMTK_OUTPUT_FORMAT_NMEA
void Adafruit_GPS::set_output_format(int format) {
  char pmtk[30] = { 0 };
  if (format == PMTK_OUTPUT_FORMAT_BINARY) {
    strcpy(pmtk, "$PMTK253,1,");
    sprintf(&pmtk[11], "%u", (int)serial_baud);
    pmtk[strlen(pmtk)] = '*';
    writePmtkChecksum(pmtk);
    sendCommand(pmtk);
  } else {
    char data[5] = { 0 };
    uint32_t baud = serial_baud;
    for (int i = 1; i < 5;i ++) {
      data[i] = (char)(baud & 0xFF);
      baud = baud >> 8;
    }
    send_binary_command(253, data, 5);
  }
}

// Computes an XOR checksum of a buffer from start (inclusive) to finish (exclusive)
char Adafruit_GPS::checksum(char* buffer, int start, int finish) {
  char sum = 0;
  for (int i = start; i < finish; i++) {
    sum ^= buffer[i];
  }
  return sum;
}

// Formats a binary packet with a command and data buffer, stores the result in buffer
// The destination buffer needs to be at least 10 bytes greater than the length of the
// input data buffer
void Adafruit_GPS::format_packet(uint16_t cmd, char* data, int datalength, char* buffer) {
  buffer[0] = 0x04;
  buffer[1] = 0x24;
  uint16_t length = 9 + datalength;
  buffer[2] = (char)(length & 0xFF);
  buffer[3] = (char)(length >> 8);
  buffer[4] = (char)(cmd & 0xFF);
  buffer[5] = (char)(cmd >> 8);
  /*
  Serial.print("Formatting data: ");
  for (int i = 0; i < datalength; i++) {
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();
  */
  memcpy(&buffer[6], data, datalength);
  buffer[6 + datalength] = checksum(buffer, 2, 6 + datalength);
  buffer[6 + datalength + 1] = 0x0D;
  buffer[6 + datalength + 2] = 0x0A;
}

// Sends a binary command packet with data
void Adafruit_GPS::send_binary_command(uint16_t cmd, char* data, int datalength) {
  char packet[9 + datalength];
  format_packet(cmd, data, datalength, packet);
  /*
  Serial.println("Packet: ");
  for (int i = 0; i < 9 + datalength; i++) {
    Serial.printf("%02X ", packet[i]);
  }
  Serial.println();
  */
  send_buffer(packet, 9 + datalength);
}

// Formats a packet to be a "command succeeded" response packet, for comparison
void Adafruit_GPS::format_acknowledge_packet(char* buffer, uint16_t command) {
  buffer[0] = 0x04;
  buffer[1] = 0x24;
  buffer[2] = 0x0C;
  buffer[3] = 0x00;
  buffer[4] = 0x01;
  buffer[5] = 0x00;
  buffer[6] = (char)(command & 0xFF);
  buffer[7] = (char)(command >> 8);
  buffer[8] = 0x03;
  buffer[9] = checksum(buffer, 2, 9);
  buffer[10] = 0x0D;
  buffer[11] = 0x0A;
}

// Blocks for a specific binary command packet of length bytes for timeout milliseconds
bool Adafruit_GPS::waitForPacket(char* packet, int length, long timeout) {
  long start = millis();
  int pos = 0;
  char c;
  int state = 0;
  char expected_checksum = 0;
  uint16_t temp;
  while((long)(millis() - start) < timeout) {
    while (available()) {
      c = read_byte();
      // Serial.printf("byte: %02X", c);
      switch (state) {
        case 0 :  // No packet detected
                  if (c == 0x04) state = 1; break;
        case 1 :  // Possible packet
                  if (c == 0x24) {
                    // Serial.println("Packet started");
                    state = 2;
                    memset(packet_buffer, 0, EPO_PACKET_LENGTH);
                    packet_buffer[0] = 0x04;
                    packet_buffer[1] = 0x24;
                  } else {
                    state = 0;
                  }
                  break;
        case 2 :  // Packet has started, read length LSB
                  length = (uint16_t)c;
                  packet_buffer[2] = c;
                  expected_checksum = c;
                  state = 3;
                  break;
        case 3 :  // Read length MSB
                  temp = c;
                  length = (uint16_t)(temp << 8 | length);
                  // Serial.print(" Packet length: ");
                  // Serial.print(length);
                  packet_buffer[3] = c;
                  expected_checksum ^= c;
                  pos = 4;
                  state = 4;
                  break;
        case 4 :  // Read packet data
                  packet_buffer[pos] = c;
                  expected_checksum ^= c;
                  // Serial.print(" position: ");
                  // Serial.print(pos);
                  pos++;
                  if (pos == length - 3) {
                    state = 5;
                  }
                  break;
        case 5 :  // Read checksum byte
                  if (c == expected_checksum) {
                    packet_buffer[pos] = c;
                    pos++;
                    state = 6;
                  } else {
                    // Got bad checksum
                    state = 0;
                  }
                  break;
        case 6 :  // Read end word LSB
                  if (c == 0x0D) {
                    packet_buffer[pos] = c;
                    pos++;
                    state = 7;
                  } else {
                    // Can't find endword!
                    state = 0;
                  }
                  break;
        case 7 :  // Read end word MSB
                  if (c == 0x0A) {
                    packet_buffer[pos] = c;
                    pos++;
                    // Pos should equal packet length now
                    if (pos != length) {
                      state = -1;
                      break;
                    }

                    /*
                    Serial.println("Complete packet received");
                    Serial.print("Packet: ");
                    for (int i = 0; i < pos; i++) {
                      Serial.printf("%02X ", packet_buffer[i]);
                    }
                    Serial.println();
                    */

                    if (strncmp(packet_buffer, packet, length) == 0) return true;
                    state = 0;
                  } else {
                    // Couldn't finish end word!
                    state = 0;
                  }
                  break;
        case -1 : // Something's wrong!!!!
                  // Serial.println("Error parsing packet");
                  break;
        default : break;
      }
      // Serial.print(" State: ");
      // Serial.print(state);
      // Serial.println();
    }
    delay(1);
  }
  // Timed out
  return false;
}

// Debugging method for dumping binary data in the buffer
void Adafruit_GPS::dump_binary_packet() {
  Serial.println("binary packet dump");
  char buff[1024] = { 0 };
  int pos = 0;
  while (Serial1.available()) {
    buff[pos] = read_byte();
    sprintf(buff, "%02X", buff[pos]);
    Serial.print(buff);
    Serial.print(" ");
  }
  Serial.println();
}

// Returns true if the GPS serial buffer has available data
bool Adafruit_GPS::available() {
  #if defined(SPARK)
    return Serial1.available();
  #else
    #ifdef __AVR__
      if (gpsSwSerial) {
        return gpsSwSerial->available();
      }
    #endif
    {
      return gpsHwSerial->available();
    }
  #endif
  return false;
}

// Returns a byte from the GPS serial buffer without processing it
// as part of an NMEA
char Adafruit_GPS::read_byte(void) {
  #if defined(SPARK)
    return Serial1.read();
  #else
    #ifdef __AVR__
      if(gpsSwSerial) {
        return gpsSwSerial->read();
      } else
    #endif
    {
      return gpsHwSerial->read();
    }
  #endif
  return 0;
}

// Sends a buffer of length bytes to the GPS serial
bool Adafruit_GPS::send_buffer(char* buffer, int length) {
  #if defined(SPARK)
    return Serial1.write(reinterpret_cast<uint8_t *>(buffer), length) == length;
  #else
    #ifdef __AVR__
      if (gpsSwSerial) {
        return gpsSwSerial->write(buffer, length) == length;
      } else
    #endif
    {
      if (gpsHwSerial) {
        return gpsHwSerial->write(buffer, length) == length;
      }
    }
  #endif
}

// Flushes an partial EPO packets
bool Adafruit_GPS::flush_epo_packet() {
  char sequence_buffer[3] = { 0 };
  sequence_buffer[0] = (char)(epo_sequence_number & 0xFF);
  sequence_buffer[1] = (char)(epo_sequence_number >> 8);
  sequence_buffer[2] = 0x01;
  packet_buffer[EPO_SEQUENCE_OFFSET] = sequence_buffer[0];
  packet_buffer[EPO_SEQUENCE_OFFSET + 1] = sequence_buffer[1];
  packet_buffer[EPO_CHECKSUM_OFFSET] = checksum(packet_buffer, 2, EPO_CHECKSUM_OFFSET);

  /*
  Serial.println("EPO Packet: ");
  for (int i = 0; i < EPO_PACKET_LENGTH; i++) {
    Serial.printf("%02X ",  packet_buffer[i]);
  }
  Serial.println();
  */

  satellite_number = 0;
  send_buffer(packet_buffer, EPO_PACKET_LENGTH);
  char expected_packet[12] = { 0 };
  format_packet(2, sequence_buffer, 3, expected_packet);

  /*
  Serial.print("Expected EPO acknowledgement: ");
  for (int i = 0; i < 12; i++) {
    Serial.printf("%02X ", expected_packet[i]);
  }
  Serial.println();
  */

  if (!waitForPacket(expected_packet, 12, 5000)) {
    // Serial.println("EPO packet was rejected");
    return false;
  }
  // Serial.println("EPO packet was accepted");
  return true;
}
