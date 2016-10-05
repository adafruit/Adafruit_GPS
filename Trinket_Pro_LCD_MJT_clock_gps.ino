// Clock example using an LCD display & GPS for time.
//
// Must have the Adafruit GPS library installed too!  See:
//   https://github.com/adafruit/Adafruit-GPS-Library
//
// Adafruit invests time and resources providing this open source code,
// please support Adafruit and open-source hardware by purchasing
// products from Adafruit!
//
// Written by Tony DiCola for Adafruit Industries.
// Released under a MIT license: https://opensource.org/licenses/MIT

// Modified by Miles Treacher 17-18 June 2016

#include <SoftwareSerial.h>
#include "Adafruit_GPS.h"
#include <LiquidCrystal.h>

// Set to false to display time in 12 hour format, or true to use 24 hour:
#define TIME_24_HOUR  false

// Offset the hours from UTC (universal time) to your local time by changing
// this value.  The GPS time will be in UTC so lookup the offset for your
// local time from a site like:
//   https://en.wikipedia.org/wiki/List_of_UTC_time_offsets
// This value, +1, will set the time to UTC+1 or British Summer Time.
#define HOUR_OFFSET       +1

// GPS breakout/shield will use a software serial connection with
// (TX = pin 12 and RX = pin 11.)
// Use pins TX to 12 and RX to 11
SoftwareSerial gpsSerial(12, 11);  // GPS breakout/shield will use a


// Create display and GPS objects.  These are global variables that
// can be accessed from both the setup and loop function below.
Adafruit_GPS gps(&gpsSerial);
LiquidCrystal lcd( 9,  3, 4, 5, 6, 8);

// Define state of push switch to false/open
boolean switchState = false;

// Define state of clock to be not set by GPS
boolean set = false;

//Define variable to display screen refreshes, see later
byte bar = 0;

// Setup function runs once at startup to initialize the display and GPS.
void setup() {
  //Set Pin 10 to digital input for switch
  pinMode(10, INPUT);

  // define size of LCD display
  lcd.begin(16, 2);

  // clean up the screen before printing
  lcd.clear();
  lcd.setCursor(2, 0);
  // print welcome screen
  lcd.print("GPS Clock by");
  delay(1500);
  lcd.clear();
  // set the cursor to column 0, line 1
  lcd.setCursor(1, 0);
  lcd.print("Tony DiCola &");
  lcd.setCursor(1, 1);
  lcd.print("Miles Treacher");
  delay(2500);
  lcd.clear();
  // turn on blinking cursor
  lcd.blink();
  // set the cursor to column 0, line 0
  lcd.setCursor(0, 0);
  // print welcome screen
  lcd.print("Clock");
  // set the cursor to column 0, line 1
  lcd.setCursor(0, 1);
  lcd.print("initialsing...");
  //wait 3 seconds
  delay(3000);
  //turn off blinking cursor
  lcd.noBlink();
  //clear LCD
  lcd.clear();
  //updateDisplay();


  // Setup Serial port to print debug output.
  //Serial.begin(115200);
  //Serial.println("Clock starting!");

  // Ask for firmware version
  //Serial.println(PMTK_Q_RELEASE);

  // Setup the GPS using a 9600 baud connection (the default for most
  // GPS modules).
  gps.begin(9600);

  // Configure GPS to only output minimum data (location, time, fix).
  // this line to turns on RMC (recommended minimum) and GGA (fix data)
  gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  //gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // Use a 1 hz, once a second, update rate.
  gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);

  // Enable the interrupt to parse GPS data.
  enableGPSInterrupt();

  // set pin 13 to output
  pinMode(13, OUTPUT);
  // turn off pin 13 LED
  digitalWrite(13, LOW);
}

// Loop function runs over and over again to implement the clock logic.
void loop() {

  // Define flag to indicate if we are in AM or PM
  boolean PM = false;

  // Check if GPS has new data and parse it.
  if (gps.newNMEAreceived()) {
    gps.parse(gps.lastNMEA());
  }

  // Grab the current hours, minutes, seconds from the GPS.
  // This will only be set once the GPS has a fix!  Make sure to add
  // a coin cell battery so the GPS will save the time between power-up/down.
  byte hours = gps.hour + HOUR_OFFSET;  // Add hour offset to convert from UTC
  // to local time.

  // check to see it time has been set
  if (set == false ) {
    //position cursor at top left
    lcd.setCursor(0, 0);
    //print waiting message
    lcd.print("Waiting for time");
    //if (gps.hour != 0 && gps.minute != 0 && gps.seconds != 0 ) {
    //there is a bug here if the time is acquired in the 59th minute of the hour
    //it will not be displayed till the minute and hour roll over to new hour
    if (gps.hour != 0 && gps.minute != 59 ) {
      //clear LCD
      lcd.clear();
      //set time is set flag to true
      set = true;
    }
  }

  // Handle when UTC + offset wraps around to a negative or > 23 value.
  if (hours < 0) {
    hours = 24 + hours;
  }
  if (hours > 23) {
    hours = 24 - hours;
  }

  //grab minutes and seconds data
  byte minutes = gps.minute;
  byte seconds = gps.seconds;

  // Do 24 hour to 12 hour format conversion when required.
  if (TIME_24_HOUR == false) {
    // Set PM value to true if not in 24hr mode and hour is 12 or greater
    if (hours > 11) {
      PM = true;
    }
    // Handle when hours are past 12 by subtracting 12 hours.
    if (hours > 12) { // Needs to be > 12
      hours -= 12;
    }
    // Handle hour 0 (midnight) being shown as 12.
    else if (hours == 0) {
      hours += 12;
    }
  }



  //if time has been set then display it
  if (set) {
    //set cursor to 3rd position
    lcd.setCursor(3, 0);

    // Add zero padding when in 24 hour mode and it's midnight.
    //if 24 hour mode and hour is 9 or less print a zero before it
    if (TIME_24_HOUR && hours <= 9) {
      lcd.print("0");
    }
    // print hours
    lcd.print(hours); lcd.print(":");
    //if minutes is 9 or less print a zero before it
    if (minutes <= 9) {
      lcd.print("0");
    }
    //print minutes
    lcd.print(minutes); lcd.print(":");
    //if seconds is 9 or less print a zero before it
    if (seconds <= 9) {
      lcd.print("0");
    }
    //print seconds
    lcd.print(seconds);
    //print 'pm' or 'am' as required
    if (!TIME_24_HOUR && PM == true) {
      lcd.print("pm ");
    }
    if (!TIME_24_HOUR && PM == false) {
      lcd.print("am ");
    }

    byte day = gps.day;
    //Do some conditional positioning of cursor??
    //   if (day <= 9) {
    // put cursor at position 4 on 2nd line
    //     lcd.setCursor(2, 1);
    //   } else {
    // put cursor at position 3 on 2nd line
    lcd.setCursor(2, 1);
    //   }
    //detect if year has been set from initial value and if so print date
    if (gps.year != 80) {
      lcd.print(gps.day); lcd.print(" ");

      // if you prefer numeric month uncomment next line and comment out whole switch statement
      //      lcd.print('/');lcd.print(gps.month); lcd.print("/");

      byte month = gps.month;
      switch (month) {
        case 1: lcd.print("Jan");
          break;
        case 2: lcd.print("Feb");
          break;
        case 3: lcd.print("Mar");
          break;
        case 4: lcd.print("Apr");
          break;
        case 5: lcd.print("May");
          break;
        case 6: lcd.print("Jun");
          break;
        case 7: lcd.print("Jul");
          break;
        case 8: lcd.print("Aug");
          break;
        case 9: lcd.print("Sep");
          break;
        case 10: lcd.print("Oct");
          break;
        case 11: lcd.print("Nov");
          break;
        case 12: lcd.print("Dec");
      }
      lcd.print(" ");
      lcd.print("20"); lcd.print(gps.year); lcd.print(" ");

      //update and print a crude frame rate indicator
      //(comment out the next three lines once you have the clock working)
      if (bar > 99) bar = 1;
      lcd.print(bar);//lcd.print(" ");
      bar ++;

    }
  }

  // /*
  // detect if GPS has fix
  if (gps.fix) {
    // Read digital 10 to see if switch is pressed
    switchState = digitalRead(10);
    if (switchState == HIGH) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sats: "); lcd.print(gps.satellites); lcd.print("  ");
      delay(1500);
      lcd.setCursor(0, 0);
      lcd.print("Lat:"); lcd.print(gps.latitudeDegrees, 4); lcd.print(" "); lcd.print(gps.lat);
      lcd.setCursor(0, 1);
      lcd.print("Lon:"); lcd.print(gps.longitudeDegrees, 4); lcd.print(" "); lcd.print(gps.lon);

      // delay by 2.5sec to enable it to read by eye
      delay(2500);

      // clear LCD display
      lcd.clear();
    }

  }

  // Loop code is finished, it will jump back to the start of the loop
  // function again! Don't add any delays because the parsing needs to
  // happen all the time!
}

SIGNAL(TIMER0_COMPA_vect) {
  // Use a timer interrupt once a millisecond to check for new GPS data.
  // This piggybacks on Arduino's internal clock timer for the millis()
  // function.
  gps.read();
}

void enableGPSInterrupt() {
  // Function to enable the timer interrupt that will parse GPS data.
  // Timer0 is already used for millis() - we'll just interrupt somewhere
  // in the middle and call the "Compare A" function above
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);
}
