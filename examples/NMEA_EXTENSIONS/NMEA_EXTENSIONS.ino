/**************************************************************************/
/*!
  @file NMEA_EXTENSIONS.ino

  @section intro Introduction

  An Arduino sketch for testing the NMEA_EXTENSIONS to the library. Does 
  not require any GPS hardware. Boat is a data only object we can use to 
  represent the actual data and build sentences from. GPS is a data only 
  object that parses the sentences and saves the results, the same way you 
  would with a communicating GPS object

  Only some of the data values will have added history. Note that history 
  is stored as integers, scaled and offset from the float values to save 
  memory. The AWA (Apparent Wind Angle) is recorded as three components, 
  so that sin and cos parts can be accurately time averaged. onList() allows 
  testing sentences against a list to see if they should be passed on to 
  another listener, allowing your sketch to act as an NMEA multiplexer.

  Although it will just barely compile for an UNO with the NMEA_EXTENSIONS,
  defining two GPS objects pushes the limits of the UNO data space and
  should probably be avoided.

  @section author Author

  Written by Rick Sellens.

  @section license License

  CCBY license
*/
/**************************************************************************/
#include "Adafruit_GPS.h"
Adafruit_GPS GPS;  // The results obtained from the instruments -- no comms
Adafruit_GPS Boat; // The state of the boat used to create some simulated sentences

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 10000); // Wait for monitor to be ready.
  Serial.print("\n\nNMEA_EXTENSIONS Example v 0.1.1\n\n");
  #ifdef NMEA_EXTENSIONS
    addHistory(&GPS);
  #else
    Serial.print("NMEA_EXTENSIONS not #defined, so there will be no action.\n");
  #endif
}

char latestBoat[200] = "";

const char *senList[] = {"GGA", "GLL", "DBT", "HDM", "MWV", "ZZ"}; // sentence list
const char *passList[] = {"GGA", "DBT", "ZZ"}; // short list

void loop() {
  static unsigned long lastPrint = 0;
  updateBoat();
  // stop keeping AWA history after 30 seconds, just as a demonstration
  #ifdef NMEA_EXTENSIONS
    if(millis() > 30000) GPS.removeHistory(NMEA_AWA);
  #endif
  if (millis() - lastPrint > 300 || lastPrint == 0) {
    lastPrint = millis();
#ifdef NMEA_EXTENSIONS
    Serial.print("\nSentences built from Boat and parsed by GPS ");
    Serial.print("(Only a few get passed on to the ST network.):\n\n");
    for (int i = 0;strncmp(senList[i],"ZZ",2);i++){
      if (GPS.parse(Boat.build(latestBoat, "II", senList[i]))){ 
        if (GPS.onList(latestBoat,passList)) 
          Serial.print("Pass to ST: ");
        else Serial.print("   No Pass: ");
        Serial.print(latestBoat);
      } else { 
        Serial.print("Couldn't build and parse a ");
        Serial.print(senList[i]);
        Serial.print(" sentence, maybe because sprintf() doesn't work with %f.");
      }
    }
    
    Serial.print("\nSome of the resulting data stored in GPS:\n\n");
    GPS.showDataValue(NMEA_LAT);
    GPS.showDataValue(NMEA_LON);
    GPS.showDataValue(NMEA_AWA, 20); // show more history values, if history on
    GPS.showDataValue(NMEA_AWA_SIN);
    GPS.showDataValue(NMEA_AWA_COS);
    GPS.showDataValue(NMEA_AWS);
    GPS.showDataValue(NMEA_HDG);
    GPS.showDataValue(NMEA_DEPTH);

    Serial.print("\nThe AWA is: ");
    Serial.print(GPS.get(NMEA_AWA));
    Serial.print(" while the smoothed value is: ");
    Serial.println(GPS.getSmoothed(NMEA_AWA));
    
#endif // NMEA_Extensions
  }
}

void updateBoat() { // Fill up the boat values with
                    // some test data to use in build()
  nmea_float_t t = millis() / 1000.;
  nmea_float_t theta = t / 100.;   // slow
  nmea_float_t gamma = theta * 10; // faster

  // add some data to the old Adafruit_GPS variables
  Boat.latitude = 4400 + sin(theta) * 60;
  Boat.lat = 'N';
  Boat.longitude = 7600 + cos(theta) * 60;
  Boat.lon = 'W';
  Boat.fixquality = 2;
  Boat.speed = 3 + sin(gamma);
  Boat.hour = abs(cos(theta)) * 24;
  Boat.minute = 30 + sin(theta / 2) * 30;
  Boat.seconds = 30 + sin(gamma) * 30;
  Boat.milliseconds = 500 + sin(gamma) * 500;
  Boat.year = 1 + abs(sin(theta)) * 25;
  Boat.month = 1 + abs(sin(gamma)) * 11;
  Boat.day = 1 + abs(sin(gamma)) * 26;
  Boat.satellites = abs(cos(gamma)) * 10;
#ifdef NMEA_EXTENSIONS
  // add some data to the new NMEA data values
  Boat.newDataValue(NMEA_AWS, 10 + cos(theta));
  Boat.newDataValue(NMEA_AWA, 180 * sin(gamma));
  Boat.newDataValue(NMEA_VTW, Boat.speed + cos(gamma) / 3);
  Boat.newDataValue(NMEA_DEPTH, 10 + cos(gamma) * 5);
  Boat.newDataValue(NMEA_HDG, 180 * sin(gamma) + 180);
  Boat.newDataValue(NMEA_HDT, 180 * cos(gamma) + 180);
  Boat.newDataValue(NMEA_VMG, sin(gamma) * 3);
  Boat.newDataValue(NMEA_VMGWP, cos(gamma) * 5);
#endif // NMEA_EXTENSIONS
}

#ifdef NMEA_EXTENSIONS
void addHistory(Adafruit_GPS *nmea) {
  // Record integer history for HDOP, scaled by 10.0, offset by 0.0, 
  // every 15 seconds for the most recent 20 values.
  nmea->initHistory(NMEA_HDOP, 10.0, 0.0, 15, 20);
  nmea->initHistory(NMEA_COG, 10.0, 0.0, 1); 
  nmea->initHistory(NMEA_AWA, 10.0, 0.0, 1);
  nmea->initHistory(NMEA_HDG, 10.0, 0.0, 3);
  // Record pressure every 10 minutes, in Pa relative to 1 bar
  nmea->initHistory(NMEA_BAROMETER, 1.0, -100000.0, 600);
  nmea->initHistory(NMEA_DEPTH, 10.0, 0.0, 3);
}
#endif              // NMEA_EXTENSIONS
