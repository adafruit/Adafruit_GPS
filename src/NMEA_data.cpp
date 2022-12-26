/**************************************************************************/
/*!
  @file NMEA_data.cpp

  Code for tracking values that change with time so that history can be
  examined for recent trends in real time. This code will only generate the
  stubs for newDataValue() and data_init(), adding essentially nothing to
  the memory footprint unless NMEA_EXTENSIONS is defined.

  This is code intended to complement the Adafruit GPS library and process
  data for many additional NMEA sentences, mostly of interest to sailors.

  The parse function can be a direct substitute for the Adafruit_GPS
  function of the same name, updating the same variables within an NMEA
  object. A simple use case would involve:

  Define an Adafruit_GPS object and use it to collect and parse sentences
  from a serial port. The GPS object will be updated and can be used exactly
  as usual.

  Define an NMEA object and use it to parse the same sentences. It will
  succeed on more sentences than the GPS object and keep more detailed data
  records. It updates all the same variables as the GPS object, so you could
  skip the GPS parsing step.

  @author Rick Sellens.

  @copyright CCBY license
*/
/**************************************************************************/

#include "Adafruit_GPS.h"

/**************************************************************************/
/*!
    @brief Update the value and history information with a new value. Call
    whenever a new data value is received. The function does nothing if the
    NMEA extensions are not enabled.
    @param idx The data index for which a new value has been received
    @param v The new value received
    @return none
*/
/**************************************************************************/
void Adafruit_GPS::newDataValue(nmea_index_t idx, nmea_float_t v) {
#ifdef NMEA_EXTENSIONS
  //  Serial.println();Serial.print(idx);Serial.print(", "); Serial.println(v);
  val[idx].latest = v; // update the value

  // update the smoothed verion
  if (isCompoundAngle(idx)) { // angle with sin/cos component recording
    newDataValue((nmea_index_t)(idx + 1), sin(v / (nmea_float_t)RAD_TO_DEG));
    newDataValue((nmea_index_t)(idx + 2), cos(v / (nmea_float_t)RAD_TO_DEG));
  }
  // weighting factor for smoothing depends on delta t / tau
  nmea_float_t w =
      min((nmea_float_t)1.0,
          (nmea_float_t)(millis() - val[idx].lastUpdate) / val[idx].response);
  // default smoothing
  val[idx].smoothed = (1.0f - w) * val[idx].smoothed + w * v;
  // special smoothing for some angle types
  if (val[idx].type == NMEA_COMPASS_ANGLE_SIN)
    val[idx].smoothed =
        compassAngle(val[idx + 1].smoothed, val[idx + 2].smoothed);
  if (val[idx].type == NMEA_BOAT_ANGLE_SIN)
    val[idx].smoothed = boatAngle(val[idx + 1].smoothed, val[idx + 2].smoothed);
  // some types just don't make sense to smooth -- use latest
  if (val[idx].type == NMEA_BOAT_ANGLE)
    val[idx].smoothed = val[idx].latest;
  if (val[idx].type == NMEA_COMPASS_ANGLE)
    val[idx].smoothed = val[idx].latest;
  if (val[idx].type == NMEA_DDMM)
    val[idx].smoothed = val[idx].latest;
  if (val[idx].type == NMEA_HHMMSS)
    val[idx].smoothed = val[idx].latest;

  val[idx].lastUpdate = millis(); // take a time stamp
  if (val[idx].hist) {            // there's a history struct for this tag
    unsigned long seconds = (millis() - val[idx].hist->lastHistory) / 1000;
    // do an update if the time has come, or if this is the first time through
    if (seconds >= val[idx].hist->historyInterval ||
        val[idx].hist->lastHistory == 0) {

      // move the old history back in time by one step
      for (unsigned i = 0; i < (val[idx].hist->n - 1); i++)
        val[idx].hist->data[i] = val[idx].hist->data[i + 1];

      // Create the new entry, scaling and offsetting the value to fit into an
      // integer, and based on the smoothed value.
      val[idx].hist->data[val[idx].hist->n - 1] =
          val[idx].hist->scale * (val[idx].smoothed - val[idx].hist->offset);
      val[idx].hist->lastHistory = millis();
    }
  }
#endif // NMEA_EXTENSIONS
}

/**************************************************************************/
/*!
    @brief    Initialize the object. Build a val[] matrix of data values for
    all of the enumerated values, including the extra values for the compound
    angle types. The initializer shold probably leave it up to the user
    sketch to decide which data values should carry the extra memory burden
    of history.
    @return   none
*/
/**************************************************************************/
void Adafruit_GPS::data_init() {
#ifdef NMEA_EXTENSIONS
  // fill all the data values with nothing
  static char c[] = "NUL";
  for (int i = 0; i < (int)NMEA_MAX_INDEX; i++) {
    initDataValue((nmea_index_t)i, c, NULL, NULL, 0, (nmea_value_type_t)0);
  }

  // fill selected data values with the relevant information and pointers
  static char BoatSpeedfmt[] = "%6.2f";
  static char WindSpeedfmt[] = "%6.1f";
  static char Speedunit[] = "knots";
  static char Anglefmt[] = "%6.0f";
  static char BoatAngleunit[] = "Degrees";
  static char TrueAngleunit[] = "Deg True";
  static char MagAngleunit[] = "Deg Mag";

  static char HDOPlabel[] = "HDOP";
  initDataValue(NMEA_HDOP, HDOPlabel);

  static char LATlabel[] = "Lat";
  static char LATfmt[] = "%9.4f";
  static char LATunit[] = "DDD.dddd";
  initDataValue(
      NMEA_LAT, LATlabel, LATfmt, LATunit, 0,
      NMEA_BOAT_ANGLE); // angle from -180 to 180, or actually -90 to 90 for lat

  static char LONlabel[] = "Lon";
  initDataValue(NMEA_LON, LONlabel, LATfmt, LATunit, 0,
                NMEA_BOAT_ANGLE); // angle from -180 to 180

  static char LATWPlabel[] = "WP Lat";
  initDataValue(NMEA_LATWP, LATWPlabel, LATfmt, LATunit, 0, NMEA_BOAT_ANGLE);

  static char LONWPlabel[] = "WP Lon";
  initDataValue(NMEA_LONWP, LONWPlabel, LATfmt, LATunit, 0, NMEA_BOAT_ANGLE);

  static char SOGlabel[] = "SOG";
  initDataValue(NMEA_SOG, SOGlabel, BoatSpeedfmt, Speedunit);

  static char COGlabel[] = "COG";
  // types with sin/cos need two extra spots in the values matrix!
  initDataValue(NMEA_COG, COGlabel, Anglefmt, TrueAngleunit, 0,
                NMEA_COMPASS_ANGLE_SIN); // type: 0-360 angle with sin/cos 11

  static char COGWPlabel[] = "WP COG";
  initDataValue(NMEA_COGWP, COGWPlabel, Anglefmt, TrueAngleunit, 0,
                NMEA_COMPASS_ANGLE); // type: angle 0-360 1

  static char XTElabel[] = "XTE";
  static char XTEfmt[] = "%6.2f";
  static char XTEunit[] = "NM";
  initDataValue(NMEA_XTE, XTElabel, XTEfmt, XTEunit);

  static char DISTWPlabel[] = "WP Dist";
  initDataValue(NMEA_DISTWP, DISTWPlabel, XTEfmt, XTEunit);

  static char AWAlabel[] = "AWA";
  initDataValue(NMEA_AWA, AWAlabel, Anglefmt, BoatAngleunit, 0,
                NMEA_BOAT_ANGLE_SIN); // type: +-180 angle with sin/cos 12

  static char AWSlabel[] = "AWS";
  initDataValue(NMEA_AWS, AWSlabel, WindSpeedfmt, Speedunit);

  static char TWAlabel[] = "TWA";
  initDataValue(NMEA_TWA, TWAlabel, Anglefmt, BoatAngleunit, 0,
                NMEA_BOAT_ANGLE_SIN); // type: +-180 angle with sin/cos 12

  static char TWDlabel[] = "TWD";
  initDataValue(NMEA_TWD, TWDlabel, Anglefmt, TrueAngleunit, 0,
                NMEA_COMPASS_ANGLE_SIN); // type: 0-360 angle with sin/cos 11

  static char TWSlabel[] = "TWS";
  initDataValue(NMEA_TWS, TWSlabel, WindSpeedfmt, Speedunit);

  static char VMGlabel[] = "VMG";
  initDataValue(NMEA_VMG, VMGlabel, BoatSpeedfmt, Speedunit);

  static char VMGWPlabel[] = "WP VMG";
  initDataValue(NMEA_VMGWP, VMGWPlabel, BoatSpeedfmt, Speedunit);

  static char HEELlabel[] = "Heel";
  static char HEELunit[] = "Deg Stbd";
  initDataValue(NMEA_HEEL, HEELlabel, Anglefmt, HEELunit, 0,
                NMEA_BOAT_ANGLE); // type: angle +/-180 2

  static char PITCHlabel[] = "Pitch";
  static char PITCHunit[] = "Deg Bow Up";
  initDataValue(NMEA_PITCH, PITCHlabel, Anglefmt, PITCHunit, 0,
                NMEA_BOAT_ANGLE); // type: angle +/-180 2
  static char HDGlabel[] = "HDG";
  initDataValue(NMEA_HDG, HDGlabel, Anglefmt, MagAngleunit, 0,
                NMEA_COMPASS_ANGLE_SIN); // type: 0-360 angle with sin/cos 11

  static char HDTlabel[] = "HDG";
  initDataValue(NMEA_HDT, HDTlabel, Anglefmt, TrueAngleunit, 0,
                NMEA_COMPASS_ANGLE_SIN); // type: 0-360 angle with sin/cos 11

  static char VTWlabel[] = "VTW";
  initDataValue(NMEA_VTW, VTWlabel, BoatSpeedfmt, Speedunit);

  static char LOGlabel[] = "Log";
  static char LOGfmt[] = "%6.0f";
  static char LOGunit[] = "NM";
  initDataValue(NMEA_LOG, LOGlabel, LOGfmt, LOGunit);

  static char LOGRlabel[] = "Trip";
  static char LOGRfmt[] = "%6.2f";
  initDataValue(NMEA_LOG, LOGRlabel, LOGRfmt, LOGunit);

  static char DEPTHlabel[] = "Depth";
  static char DEPTHfmt[] = "%6.1f";
  static char DEPTHunit[] = "m";
  initDataValue(NMEA_DEPTH, DEPTHlabel, DEPTHfmt, DEPTHunit);

  static char RPM_M1label[] = "Motor 1";
  static char RPM_M1fmt[] = "%6.0f";
  static char RPM_M1unit[] = "RPM";
  initDataValue(NMEA_RPM_M1, RPM_M1label, RPM_M1fmt, RPM_M1unit);

  static char TEMPERATURE_M1label[] = "Temp 1";
  static char TEMPERATURE_M1fmt[] = "%6.0f";
  static char TEMPERATURE_M1unit[] = "Deg C";
  initDataValue(NMEA_TEMPERATURE_M1, TEMPERATURE_M1label, TEMPERATURE_M1fmt,
                TEMPERATURE_M1unit);

  static char PRESSURE_M1label[] = "Oil 1";
  static char PRESSURE_M1fmt[] = "%6.0f";
  static char PRESSURE_M1unit[] = "kPa";
  initDataValue(NMEA_PRESSURE_M1, PRESSURE_M1label, PRESSURE_M1fmt,
                PRESSURE_M1unit);

  static char VOLTAGE_M1label[] = "Motor 1";
  static char VOLTAGE_M1fmt[] = "%6.2f";
  static char VOLTAGE_M1unit[] = "Volts";
  initDataValue(NMEA_VOLTAGE_M1, VOLTAGE_M1label, VOLTAGE_M1fmt,
                VOLTAGE_M1unit);

  static char CURRENT_M1label[] = "Motor 1";
  static char CURRENT_M1fmt[] = "%6.1f";
  static char CURRENT_M1unit[] = "Amps";
  initDataValue(NMEA_CURRENT_M1, CURRENT_M1label, CURRENT_M1fmt,
                CURRENT_M1unit);

  static char RPM_M2label[] = "Motor 2";
  initDataValue(NMEA_RPM_M2, RPM_M2label, RPM_M1fmt, RPM_M1unit);

  static char TEMPERATURE_M2label[] = "Temp 2";
  initDataValue(NMEA_TEMPERATURE_M2, TEMPERATURE_M2label, TEMPERATURE_M1fmt,
                TEMPERATURE_M1unit);

  static char PRESSURE_M2label[] = "Oil 2";
  initDataValue(NMEA_PRESSURE_M2, PRESSURE_M2label, PRESSURE_M1fmt,
                PRESSURE_M1unit);

  static char VOLTAGE_M2label[] = "Motor 2";
  initDataValue(NMEA_VOLTAGE_M2, VOLTAGE_M2label, VOLTAGE_M1fmt,
                VOLTAGE_M1unit);

  static char CURRENT_M2label[] = "Motor 2";
  initDataValue(NMEA_CURRENT_M2, CURRENT_M2label, CURRENT_M1fmt,
                CURRENT_M1unit);

  static char TEMPERATURE_AIRlabel[] = "Air";
  static char TEMPERATURE_AIRfmt[] = "%6.1f";
  static char TEMPERATURE_AIRunit[] = "Deg C";
  initDataValue(NMEA_TEMPERATURE_AIR, TEMPERATURE_AIRlabel, TEMPERATURE_AIRfmt,
                TEMPERATURE_AIRunit);

  static char TEMPERATURE_WATERlabel[] = "Water";
  static char TEMPERATURE_WATERfmt[] = "%6.1f";
  static char TEMPERATURE_WATERunit[] = "Deg C";
  initDataValue(NMEA_TEMPERATURE_WATER, TEMPERATURE_WATERlabel,
                TEMPERATURE_WATERfmt, TEMPERATURE_WATERunit);

  static char HUMIDITYlabel[] = "Humidity";
  static char HUMIDITYfmt[] = "%6.0f";
  static char HUMIDITYunit[] = "% RH";
  initDataValue(NMEA_HUMIDITY, HUMIDITYlabel, HUMIDITYfmt, HUMIDITYunit);

  static char BAROMETERlabel[] = "Barometer";
  static char BAROMETERfmt[] = "%6.0f";
  static char BAROMETERunit[] = "Pa";
  initDataValue(NMEA_BAROMETER, BAROMETERlabel, BAROMETERfmt, BAROMETERunit);
#endif // NMEA_EXTENSIONS
}

#ifdef NMEA_EXTENSIONS
/**************************************************************************/
/*!
    @brief Clearer approach to retrieving NMEA values by allowing calls that
    look like nmea.get(NMEA_TWA) instead of val[NMEA_TWA].latest.
    Use newDataValue() to set the values.
    @param idx the NMEA value's index
    @return the latest NMEA value
*/
/**************************************************************************/
nmea_float_t Adafruit_GPS::get(nmea_index_t idx) {
  if (idx >= NMEA_MAX_INDEX || idx < NMEA_HDOP)
    return 0.0;
  return val[idx].latest;
}

/**************************************************************************/
/*!
    @brief Clearer approach to retrieving NMEA values
    @param idx the NMEA value's index
    @return the latest NMEA value, smoothed
*/
/**************************************************************************/
nmea_float_t Adafruit_GPS::getSmoothed(nmea_index_t idx) {
  if (idx >= NMEA_MAX_INDEX || idx < NMEA_HDOP)
    return 0.0;
  return val[idx].smoothed;
}

/**************************************************************************/
/*!
    @brief Initialize the contents of a data value table entry
    @param idx The data index for the value to be initialized
    @param label Pointer to a label string that describes the value
    @param fmt Pointer to a sprintf format to use for the value, e.g. "%6.2f"
    @param unit Pointer to a string for the units, e.g. "Deg Mag"
    @param response Time constant for smoothing in ms. The longer the time
    constant, the more slowly the smoothed value will move towards a new value.
    @param type The type of data contained in the value. simple float 0,
    angle 0-360 1, angle +/-180 2, angle with history centered +/- around
    the latest angle 3, lat/lon DDMM.mm 10, time HHMMSS 20.
    @return none
*/
/**************************************************************************/
void Adafruit_GPS::initDataValue(nmea_index_t idx, char *label, char *fmt,
                                 char *unit, unsigned long response,
                                 nmea_value_type_t type) {
  if (idx < NMEA_MAX_INDEX) {
    if (label)
      val[idx].label = label;
    if (fmt)
      val[idx].fmt = fmt;
    if (unit)
      val[idx].unit = unit;
    if (response)
      val[idx].response = response;
    val[idx].type = type;
    if ((int)(val[idx].type / 10) ==
        1) { // angle with sin/cos component recording
      initDataValue(
          (nmea_index_t)(idx +
                         1)); // initialize the next two data values as well
      initDataValue((nmea_index_t)(idx + 2));
    }
  }
}

/**************************************************************************/
/*!
    @brief Attempt to add history to a data value table entry. If it fails
    to malloc the space, history will not be added. Test the pointer for a
    check if needed. Select scale and offset values carefully so that
    operations and results will fit inside 16 bit integer limits. For example
    a scale of 1.0 and an offset of 100000.0 would be a good choice for
    atmospheric pressure in Pa with values ranging ~ +/- 3500, while a scale
    of 10.0 would be pushing the integer limits.
    @param idx The data index for the value to have history recorded
    @param scale Value for scaling the integer history list
    @param offset Value for scaling the integer history list
    @param historyInterval Approximate Time in seconds between historical
   values.
    @param historyN Set size of data buffer.
    @return pointer to the history
*/
/**************************************************************************/
nmea_history_t *Adafruit_GPS::initHistory(nmea_index_t idx, nmea_float_t scale,
                                          nmea_float_t offset,
                                          unsigned historyInterval,
                                          unsigned historyN) {
  historyN = max((unsigned)10, historyN);
  if (idx < NMEA_MAX_INDEX) {
    // remove any existing history
    if (val[idx].hist != NULL)
      removeHistory(idx);
    // space for the struct
    val[idx].hist = (nmea_history_t *)malloc(sizeof(nmea_history_t));
    if (val[idx].hist != NULL) {
      // space for the data array of the appropriate size
      val[idx].hist->data = (int16_t *)malloc(sizeof(int16_t) * historyN);
      if (val[idx].hist->data != NULL) {
        // initialize the data array
        for (unsigned i = 0; i < historyN; i++)
          val[idx].hist->data[i] = 0;
      } else
        free(val[idx].hist);
    }
    if (val[idx].hist != NULL) {
      val[idx].hist->n = historyN;
      if (scale > 0.0f)
        val[idx].hist->scale = scale;
      val[idx].hist->offset = offset;
      if (historyInterval > 0)
        val[idx].hist->historyInterval = historyInterval;
    }
    return val[idx].hist;
  }
  return NULL;
}

/**************************************************************************/
/*!
    @brief Remove history from a data value table entry, if it has been added.
    @param idx The data index for the value to have history removed
    @return none
*/
/**************************************************************************/
void Adafruit_GPS::removeHistory(nmea_index_t idx) {
  if (idx < NMEA_MAX_INDEX) {
    if (val[idx].hist == NULL)
      return;
    free(val[idx].hist->data);
    free(val[idx].hist);
    val[idx].hist = NULL;
  }
}

/**************************************************************************/
/*!
    @brief Print out the current state of a data value. Primarily useful as
    a debugging aid.
    @param idx The index for the data value
    @param n The number of history values to include
    @return none
*/
/**************************************************************************/
void Adafruit_GPS::showDataValue(nmea_index_t idx, int n) {
  Serial.print("idx: ");
  if (idx < 10)
    Serial.print(" ");
  Serial.print(idx);
  Serial.print(", ");
  Serial.print(val[idx].label);
  Serial.print(", ");
  Serial.print(val[idx].latest, 4);
  Serial.print(", ");
  Serial.print(val[idx].smoothed, 4);
  Serial.print(", at ");
  Serial.print(val[idx].lastUpdate);
  Serial.print(" ms, tau = ");
  Serial.print(val[idx].response);
  Serial.print(" ms, type:");
  Serial.print(val[idx].type);
  Serial.print(",  ockam:");
  Serial.print(val[idx].ockam);
  if (val[idx].hist) {
    Serial.print("\n     History at ");
    Serial.print(val[idx].hist->historyInterval);
    Serial.print(" second intervals:  ");
    Serial.print(val[idx].hist->data[val[idx].hist->n - 1]);
    for (unsigned i = val[idx].hist->n - 2;
         i >= max(val[idx].hist->n - n, (unsigned)0);
         i--) { // most recent first
      Serial.print(", ");
      Serial.print(val[idx].hist->data[i]);
    }
  }
  Serial.print("\n");
  if (idx == NMEA_LAT) {
    Serial.print("     latitude (DDMM.mmmm): ");
    Serial.print(latitude, 4);
    Serial.print(", lat: ");
    Serial.print(lat);
    Serial.print(", latitudeDegrees: ");
    Serial.print(latitudeDegrees, 8);
    Serial.print(", latitude_fixed: ");
    Serial.println(latitude_fixed);
  }
  if (idx == NMEA_LON) {
    Serial.print("     longitude (DDMM.mmmm): ");
    Serial.print(longitude, 4);
    Serial.print(", lon: ");
    Serial.print(lon);
    Serial.print(", longitudeDegrees: ");
    Serial.print(longitudeDegrees, 8);
    Serial.print(", longitude_fixed: ");
    Serial.println(longitude_fixed);
  }
}

/**************************************************************************/
/*!
    @brief Check if it is a compound angle
    @param idx The index for the data value
    @return true if a compound angle requiring 3 contiguos data values.
*/
/**************************************************************************/
bool Adafruit_GPS::isCompoundAngle(nmea_index_t idx) {
  if ((int)(val[idx].type / 10) == 1) // angle with sin/cos component recording
    return true;
  return false;
}

/**************************************************************************/
/*!
    @brief Estimate a direction in -180 to 180 degree range from the values
    of the sine and cosine of the compound angle, which could be noisy.
    @param s The sin of the angle
    @param c The cosine of the angle
    @return The angle in -180 to 180 degree range.
*/
/**************************************************************************/
nmea_float_t Adafruit_GPS::boatAngle(nmea_float_t s, nmea_float_t c) {
  // put the sin angle in -90 to 90 range
  nmea_float_t sAng = asin(s) * (nmea_float_t)RAD_TO_DEG;
  while (sAng < -90)
    sAng += 180.0f;
  while (sAng > 90)
    sAng -= 180.0f;
  // put the cos angle in 0 to 180 range
  nmea_float_t cAng = acos(c) * (nmea_float_t)RAD_TO_DEG;
  while (cAng < 0)
    cAng += 180.0f;
  while (cAng > 180)
    cAng -= 180.0f;
  // Pick the most accurate representation and translate
  if (cAng < 45)
    return sAng; //            Close hauled
  else {
    if (cAng > 135) { //       Running
      if (sAng > 0)
        return 180 - sAng; //     on starboard tack
      else
        return -180 - sAng; //    on port tack
    } else {                // Reaching
      if (sAng < 0)
        return -cAng; //          on port tack
      else
        return cAng; //           on starboard tack
    }
  }
  return 9999; // you can't get here, but there must be an explicit return
}

/**************************************************************************/
/*!
    @brief Estimate a direction in 0 to 360 degree range from the values
    of the sine and cosine of the compound angle, which could be noisy.
    @param s The sin of the angle
    @param c The cosine of the angle
    @return The angle in 0 to 360 degree range.
*/
/**************************************************************************/
nmea_float_t Adafruit_GPS::compassAngle(nmea_float_t s, nmea_float_t c) {
  nmea_float_t ang = boatAngle(s, c);
  if (ang < 5000) { // if reasonable range
    while (ang < 0)
      ang += 360.0f; // round up
    while (ang > 360)
      ang -= 360.0f; // round down
  }
  return ang;
}
#endif // NMEA_EXTENSIONS
