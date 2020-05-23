/*
  ' ################################################################################
  ' ###
  ' ###       *** Wolfgang Franke 2020 - ESP32 software template including:      ***
  ' ###           
  ' ###       (A) ESP32 data visualized as Highcharts graphs in Web Browser (the HTML file is attached below after the code)
  ' ###         *Source: https://www.highcharts.com/demo
  ' ###         *use case: short term - numeric data on ESP32 (<100 in time series) will be visualized in line-charts (or other Highchart types) in a Web Browser
  ' ###           - the Browser opens the page 'ESP-IP-address/open_charts' to receive the Highcharts-HTML file stored in ESP32 SPIFFS,
  ' ###           - the downloaded Highcharts-HTML page runs the Highcharts scripts, embedded in a GET-request loop to receive data from ESP32 every x sec:
  ' ###           - the Web Browser sends an HTTP GET request to ESP32 Web Server, and waits to receive a JSON object to be parsed and data added to highcharts,
  ' ###           - the ESP32 program puts variables into a JSON object, then sends it as GET-response back to the Web Browser,
  ' ###         
  ' ###       (B) ESP32 Serial logger into Web Browser (the HTML file is attached below after the code)
  ' ###         *use case 1: mid term logging - all ESP32 Log-lines (tested max was 12 hours with 5 log-lines per second) get printed in a Web Browser (via HTML-GET/JSON):
  ' ###           - the Browser opens the page 'ESP-IP-address/open_logger' to receive the WebLogger-HTML file stored in ESP32 SPIFFS
  ' ###           - the ESP32 program/scetch continuously prints Log entries into a log-line buffer
  ' ###           - the downladed Weblogger-HTML page runs a script sending HTTP GET requests (every 1s) to ESP32 Web Server (ESP-IP-address/data1 and data2),
  ' ###           - the ESP32 program puts all filled log-line entries into a JSON object, sends it as GET-response to the Web Browser, then deletes the log-line buffer
  ' ###           - the Web Browser script parses the incoming JSON object and adds all log lines to the browser screen,
  ' ###           - to limit the http-load in ESP32, the Browser script should not send GET-requests more than 1 per sec,
  ' ###           - to limit RAM usage in ESP32, define a maximum number of log-Lines (all log-lines will be stored in an Array of strings)
  ' ###           - a good example is: max 13 log-lines, with a GET-request interval of 1s, which adds some safety in case of one failed GET:
  ' ###             the ESP can log up to 6 lines each second, and in case the web browser skips a second it will receive 12 log lines in the next second,
  ' ###             the 13th line within the log line buffer always keeps the very last log entry, and will indicate a log age of >=2 seconds 
  ' ###         *use case 2: long term logging - all ESP32 Log-lines continuously to be stored in a file (tested 25MB/day with 5 log-lines per second) on a PC (via CURL-Script/CSV):
  ' ###           - the ESP32 program/scetch continuously prints Log entries into a log-line buffer
  ' ###           - a CURL-script sends HTTP GET requests to ESP32 Web Server (ESP-IP-address/data), e.g. every 1 sec,
  ' ###           - the ESP32 program generates an CVS object from the available log-lines (also adds CR/LF at end of file), then deletes the log-line buffer
  ' ###             and sends the CSV as GET-response back to the CURL script, to be saved/appended to a file on PC hard disk,
  ' ###           - a good example is like in use case 1: max 13 log-lines, with a GET-request interval of 1s
  ' ###           - the belonging bash script can be downloaded from ESP32 SPIFFS (download by file to keep the code formatting)
  ' ###  
  ' ###       (C) ESP32 and SPIFFS file system limitations 
  ' ###           *Source: https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html)
  ' ###           - SPIFFS does not support directories, it just stores a “flat” list of files, but the slash character '/' is allowed in filenames to emulate directories
  ' ###           - Limit of 32 chars in total for filenames! The last char is reserved for '\0' C string termination, so that leaves us with 31 usable characters.
  ' ###           - Warning: That limit is easily reached and if ignored, problems might go unnoticed because no error message will appear at compilation nor runtime.
  ' ###   
  ' ###  
  ' ###       (X)  ESP32 configuration and software interfaces:
  ' ###         - Time-0 generates 1s interval (for use in main loop)
  ' ###         - ESP32 WebServer used from standard library, handling Get requests
  ' ###         - NTP time 
  ' ###         - SPIFFS upload with "Arduino ESP32 filesystem uploader" (Arduino IDE Extension "ESP32FS-1.0.zip" - download in https://github.com/me-no-dev/arduino-esp32fs-plugin)
  ' ###
  ' ################################################################################
*/

//#########################################################################
//  ******  MCU setting, Hardware declaration, compiler definitions  ******
//#########################################################################

#include <WiFi.h>               // Standard ESP32 WiFi library
// source: Arduino Standard Library

#include <WebServer.h>          // Standard ESP32 WiFi library
// source: Arduino Standard Library

#include "time.h"               // Standard of C Time Library with functions to get NTP time from NTP servers and to manipulate date and time information
// source: Arduino Standard Library
// Arduino (ESP32) Time library sets ESP32 RTC and gets NTP time from one of up to 3 servers and translates to local time with DST,

#include <SPIFFS.h>             // Statdnard ESP32 SPIFFS Filesystem library
// source: Arduino Standard Library
// using the ESP32 Filesystem Uploader ESP32FS v1.0 Arduino Extension  from https://github.com/me-no-dev/arduino-esp32fs-plugin

//-------------------------------------------------------------------------
//  ******  End of MCU setting, Hardware declaration, compiler definitions  ******
//-------------------------------------------------------------------------



//#########################################################################
//   ****** global variables definitions   ******
//#########################################################################

// WiFi credentials
const char* ssid = " your WiFi SSID";
const char* password = " your WiFi password";
const char* host = "ESP32_Demo_Device";

// Device and File names, used in SPIFFS and HTML-Root window
String SPIFFS_file_highcharts = "weblog_highcharts.html";       // specific name for the "highcharts.html" file in SPIFFS (<32 char)
String SPIFFS_file_seriallogger = "weblog_seriallogger.html";   // specific name for the "seriallogger.html" file in SPIFFS (<32 char)
String SPIFFS_file_helpfile = "weblog_helpfile.html";           // specific name for the "helpfile.html" file in SPIFFS (<32 char)
String SPIFFS_file_bashscript = "curl_script.sh";               // name for the curl-script file in SPIFFS (<32 char), which delivers the bash-script to the browser

// NTP and RTC time variable
const char* RTC_local_TZ_DST = "CET-1CEST,M3.5.0/02,M10.5.0/03";            // local TZ and DST, see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
const char* const PROGMEM ntpServer[] = {"de.pool.ntp.org", "ptbtime1.ptb.de", "europe.pool.ntp.org", "pool.ntp.org"};    // the NTP library can call up to 3 servers
struct tm RTC_time_components;            // struct containing calendar date and time broken down into its components, see www.cplusplus.com/reference/ctime/tm/
char NTP_array22[22];                     // String buffer for NTP date/time manipulations
String NTP_String;                        // String buffer to concat NTP status
time_t NTP_time_UnixSec = time(&NTP_time_UnixSec);  // fill a time struct with the current local time as Unix-Seconds

// Web-Logger string-array and variables
const int LogLines_array_size = 14;     // max number of log-lines in log-line buffer
const int LogLines_array_maxLen = 50;   // max lenght of each Log-Line text
char LogLines_array[LogLines_array_size][LogLines_array_maxLen+1];  // array to buffer log-lines, each log-line of max "LogLines_array_maxLen" chars
int LogLine_counter = 0;                // pointer to the next available log line (increments from 0 (no entry) 10 (for 10 lines in arraylist[0-9])
String LogLine_String;                  // String buffer to concat a single log-line in main loop
String LogLine_JsonString;              // String buffer to concat log-lines into a single JSON object in Get-Request handler
char buffer1[33];                       // char-array buffer for converting INT/FLOAT to String
char buffer2[33];                       // char-array buffer for converting INT/FLOAT to String 

// system variables
unsigned long Power_On_Seconds = 0;     // counts seconds since power-on (u-long seconds = 136 years)

//-------------------------------------------------------------------------
//   ****** End of global variables definitions ******
//-------------------------------------------------------------------------



//#########################################################################
//   ****** hardware decalarations, ISR and software inits   ******
//#########################################################################

// *** init WebServer ***
WebServer   ESP32WebServer(80);                 // start WebServer on Port 80


// *** Timer-0 Handling ***
// Timer declaration
hw_timer_t * timer0 = NULL;                              // pointer to a variable of type hw_timer_t, used in the Arduino "timerBegin()" function
portMUX_TYPE timer0Mux = portMUX_INITIALIZER_UNLOCKED;   // variable of type portMUX_TYPE, to protect read/write of data values by a Critical Section in ISR and main loop,
volatile int Timer0_INT_counter;                        // flag/counter/semaphore to signal the 1s from ISR to main-loop (shared variable between ISR and main loop to be declared "volatile" to avoid removal due to compiler optimizations)

// Timer-0 ISR
void IRAM_ATTR Timer0_ISR()
{
  // use a critical section to protect shared variables
  portENTER_CRITICAL_ISR(&timer0Mux);
  // here safely read timer value or calculate counters, used by e.g. both ISR and main loop
  Timer0_INT_counter++;                                 // Set flag/counter/semaphore to signal to the main loop that Timer0 has fired
  portEXIT_CRITICAL_ISR(&timer0Mux);
} // end of Timer0_ISR

//-------------------------------------------------------------------------
//   ****** End of hardware decalarations, ISR and software inits ******
//-------------------------------------------------------------------------



//#########################################################################
//    ****** main program init ******
//#########################################################################

// main setup
void setup(void)
{
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // *** MCU config ***
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // start Timer0 to generate 1s intervals by Timer-INT
  Timer0_INT_counter = 0;                             // reset flag/counter/semaphore of signal to main loop when the timer has fired
  timer0 = timerBegin(0, 80, true);                   // config Timer0 , with prescaler=80 (down to 1MHz) ,  true=count-up
  timerAttachInterrupt(timer0, &Timer0_ISR, true);    // attach Timer0_ISR , interrupt generated is of edge type
  timerAlarmWrite(timer0, 1000000, true);             // set timer counter to 1s , enable auto-reload counter
  timerAlarmEnable(timer0);                           // start Timer0


  // init serial port
  Serial.begin(115200);
  while (!Serial) {
    // will pause until serial console opens
    vTaskDelay(1 / portTICK_PERIOD_MS); //delay in ms
  }
  Serial.println();
  Serial.print("\nBooting: ");
  Serial.println(host);


  // Initialize SPIFFS
  if(!SPIFFS.begin())
  {
    #if debug_level >= 1
    Serial.println("");
    Serial.println("An Error has occurred while mounting SPIFFS");
    Serial.println("");
    #endif    
    return;
  }
  else                        // SPIFFS file system: directories don’t actually exist in SPIFFS, only files 
  {                           // List all the files, starting by “/”, regardless of how many other “/” characters exist in their names (simulating nested folders)
    #if debug_level >= 2
    Serial.println("Found the following files on ESP32 SPIFFS:");
    File root = SPIFFS.open("/");                   // return a File object, it represents the root directory, there is no file with this name on the file system
    File file = root.openNextFile();                // obtain the first file in the SPIFFS file system on root directory File object, 
    while(file)                                     // returns another File object, now representing an actual file,
    {
      Serial.print("  ");
      Serial.println(file.name());
      file = root.openNextFile();                   // loop until no file found anymore in "/" (SPIFFS has no directories, just simulating nested folders)
    }
    Serial.println(F(" "));  
    #endif
  }

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // init WiFi and WebServer
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // connect WiFi to SSID with password
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("WiFi connected to: ");
  Serial.println(ssid);
  Serial.print("DHCP IP address:   ");
  Serial.println(WiFi.localIP());
  

  // Init WiFi and Web Server Handler

  ESP32WebServer.onNotFound(ESP32WebServer_handle_notfound);                     // handler to send Page-not-found

  ESP32WebServer.on("/", ESP32WebServer_handle_Root);                           // handler for web requests to "/" path: open  Main Page or Captive Portal of ESP32 with Menu
   
  ESP32WebServer.on("/open_charts", ESP32WebServer_handle_send_chartsHTML);     // handler for web requests to "/open_charts" path: send HTML with Highcharts from SPIFFS to Browser

  ESP32WebServer.on("/open_logger", ESP32WebServer_handle_send_loggerHTML);     // handler for web requests to "/open_logger" path: send HTML with Serial-Web-Logger from SPIFFS to Browser

  ESP32WebServer.on("/open_help",ESP32WebServer_handle_send_helpHTML);          // handler for web requests to "/open_help" path: send HTML with Help from SPIFFS to Browser
  
  ESP32WebServer.on("/curl_script.sh",ESP32WebServer_handle_send_curlscriptHTML);  // handler for web requests to "/curl_script.sh" path: send HTML with CURL-script from SPIFFS to Browser

  ESP32WebServer.on("/get_chart1_json",ESP32WebServer_handle_chart1_json);      // handler for web get-requests to "/get_chart1_json" path: send 2 JSON data to the web-script running in Browser

  ESP32WebServer.on("/get_chart2_json",ESP32WebServer_handle_chart2_json);      // handler for web get-requests to "/get_chart2_json" path: send 1 JSON data to the web-script running in Browser

  ESP32WebServer.on("/get_loglines_json",ESP32WebServer_handle_loglines_json);  // handler for web get-requests to "/get_loglines_json" path: send a JSON object to the web-script running in Browser

  ESP32WebServer.on("/curl_loglines_csv",ESP32WebServer_handle_loglines_csv);   // handler for web curl-requests to "/curl_loglines_csv" path: send some CSV-lines to a script "curling" raw text

  ESP32WebServer.begin();                   // start WebServer on ESP32
  Serial.println("HTTP server started");

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // init NTP and get time from NTP Server
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // setup NTP Servers
  configTime(0, 0, ntpServer[1], ntpServer[2], ntpServer[3]);     // NTP Server setup (max 3 Server)
  // configure automatic TZ and DST
  setenv("TZ", RTC_local_TZ_DST, 1);                              // Time zone config https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  // receive time from NTP server and convert it to local time
  if (!getLocalTime(&RTC_time_components))                        // getLocalTime(): save NTP-time in tm-struct, http://www.cplusplus.com/reference/ctime/localtime/
  {
    Serial.println("Failed to obtain time from NTP Server");
  }
  else                                                            // print new ESP32 time
  {
    strftime(NTP_array22, sizeof(NTP_array22), "%d.%b.%Y %T ", &RTC_time_components);    // http://www.cplusplus.com/reference/ctime/strftime/
    Serial.print("NTP Server init ok:  ");
    Serial.println(NTP_array22);
  }

    
}//end of Arduino "Setup"
//-------------------------------------------------------------------------
//   ****** End setup ******
//-------------------------------------------------------------------------



//#########################################################################
//    ****** Arduino main program loop ******
//#########################################################################

//Arduino main loop
void loop(void)
{
  ESP32WebServer.handleClient();            // handle incoming Web Requests
  
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // *** wait for next second indicated by Timer0-ISR ***
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // check if Timer0 has fired the 1s signal
  if (Timer0_INT_counter > 0)                 // >0 means Timer0 has fired x times, counter would allow to process all/missing INTs
  {
    // here safely read/copy the global variable set by the Timer0-ISR (protected by a critical section)
    portENTER_CRITICAL(&timer0Mux);
    Timer0_INT_counter = 0;                   // reset Timer0-ISR counter
    portEXIT_CRITICAL(&timer0Mux);;

    // increase power-on second counter
    Power_On_Seconds = Power_On_Seconds + 1;  // add 1 second to the power-on counter (to display as minutes running)

    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // *** do your actions here every second: ***
    //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
    // update time variable (tm-struct) with current time
    NTP_time_UnixSec = time(&NTP_time_UnixSec);                         // get NTP/RTC time
    localtime_r(&NTP_time_UnixSec, &RTC_time_components);               // Convert Unix-Seconds to RTC_time_components as local time, http://www.cplusplus.com/reference/ctime/localtime/


    // start logging of values, to be printed as string to the Web Browser  (fill up to half of the log lines for next 1s Browser polling interval using GET requests)
    // I suggest to use a format (set commas) which best supports importing as CSV
    //LogLinePrint("");               // log an empty line (will translated to CR/LF before sending to Web Browser)
    
    //log line 1
    // log current RTC-time and power-on minutes
    //strftime(NTP_array22, sizeof(NTP_array22), "%d.%b.%Y %T ", &RTC_time_components);    // %d.%b.%Y %T: 25.Apr.2020 12:09:52     // http://www.cplusplus.com/reference/ctime/strftime/
    strftime(NTP_array22, sizeof(NTP_array22), "%T ", &RTC_time_components);             // &T:  ISO 8601 time format (HH:MM:SS)  //http://www.cplusplus.com/reference/ctime/strftime/
    NTP_String = "";
    NTP_String.concat(NTP_array22);
    NTP_String.concat(" - Power-On: ");    
    NTP_String.concat(Power_On_Seconds / 60);         
    NTP_String.concat(" min,");
    LogLinePrint(NTP_String);

    //log line 2
    dtostrf(ESP.getFreeHeap() / 1024.0, 3, 3, buffer1);                     //prepare a FLOAT value 
    dtostrf(ESP.getMaxAllocHeap() / 1024.0, 3, 3, buffer2);                 //prepare a FLOAT value 
    LogLine_String = "";
    LogLine_String.concat("HEAP free/maxAlloc (KB): ,");
    LogLine_String.concat(buffer1);
    LogLine_String.concat(", ");
    LogLine_String.concat(buffer2);
    LogLine_String.concat(",");    
    LogLinePrint(LogLine_String);       // print this string to the Web Browser (append as last item to LogLine_array)

    //log line 3
    itoa (random(100),buffer1,10);                            //prepare a INT value 
    dtostrf(random(10), 2, 0, buffer2);                       //prepare a FLOAT value 
    LogLine_String = "";
    LogLine_String.concat("3: random Volt: ,");    
    LogLine_String.concat(buffer1);         
    LogLine_String.concat(", random Amps: ,");
    LogLine_String.concat(buffer2);
    LogLine_String.concat(",");
    LogLinePrint(LogLine_String);       // print this string to the Web Browser (append as last item to LogLine_array)

    //log line 4
    itoa (random(100),buffer1,10);                            //prepare a INT value 
    dtostrf(random(10), 2, 0, buffer2);                       //prepare a FLOAT value 
    LogLine_String = "";
    LogLine_String.concat("4: random Temp: ,");    
    LogLine_String.concat(buffer1);         
    LogLine_String.concat(", random Humid: ,");
    LogLine_String.concat(buffer2);
    LogLine_String.concat(",");
    LogLinePrint(LogLine_String);       // print this string to the Web Browser (append as last item to LogLine_array)
    
    //log line 5
    itoa (random(100),buffer1,10);                            //prepare a INT value 
    dtostrf(random(10), 2, 0, buffer2);                       //prepare a FLOAT value 
    LogLine_String = "";
    LogLine_String.concat("5: random Pressure: ,");    
    LogLine_String.concat(buffer1);         
    LogLine_String.concat(", random Light: ,");
    LogLine_String.concat(buffer2);
    LogLine_String.concat(",");
    LogLinePrint(LogLine_String);       // print this string to the Web Browser (append as last item to LogLine_array)

    //log line 6
    itoa (random(100),buffer1,10);                            //prepare a INT value 
    dtostrf(random(10), 2, 0, buffer2);                       //prepare a FLOAT value 
    LogLine_String = "";
    LogLine_String.concat("6: random Status1: ,");    
    LogLine_String.concat(buffer1);         
    LogLine_String.concat(", random Status2: ,");
    LogLine_String.concat(buffer2);
    LogLine_String.concat(",");
    LogLinePrint(LogLine_String);       // print this string to the Web Browser (append as last item to LogLine_array)
    
  } // end of 1s interval generated by Timer0
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

}// end of Arduino "Main Loop"
//------------------------------------------------------------------------
//   ****** End main program loop ******
//-------------------------------------------------------------------------




//#########################################################################
//   ****** Subroutines ; ISR ; Handlers ******
//#########################################################################

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *** Subroutine: LogLinePrint() (into LogLines_array as log line buffer) ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LogLinePrint(String singleLogLine)   // append this string as last item to LogLine_array (and cast it as "string", to be sent as JSOB item the Web Browser later)
{ 
  // translate empty LogLine string to a short new line in Web Broswer (by just printing '.')
  if (singleLogLine == "")                                  // if LogLine is empty string then add a space to add an empty new line in Web Broswer
  {
    singleLogLine = ".";                                    // use "." for empty browser line, as the Browser script does not print empty lines (a string lenght of 0, or when only spaces)
  }

  // check if LogLine array is full
  if (LogLine_counter == LogLines_array_size)               // if LogLine array is full ("LogLines_array_size" log entries)
  {
    LogLine_counter = LogLines_array_size - 1;              // append current log entry as last line (overwrite 
  }

  // check maximium log-line lenght
  if ((singleLogLine.length() + 2) > LogLines_array_maxLen) // make sure "singleLogLine" and 2x -"- chars will fit in LogLine array, remove the first characters to shorten it
  {
    singleLogLine = singleLogLine.substring(singleLogLine.length()-LogLines_array_maxLen+2, singleLogLine.length());
  }

  // append this string as next line to LogLine_array (do not forget to copy the String-End-Zero too)
  singleLogLine.toCharArray(LogLines_array[LogLine_counter], singleLogLine.length()+1);
  LogLine_counter++;                   // increase the pointer to the next available log line (0 = no entry / LogLines_array_size = array list is full)  
}; //end of Sub LogLinePrint()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// // *** Handler: handle ESP32WebServer path notfound ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ESP32WebServer_handle_notfound()
{
  ESP32WebServer.send(404, "text/plain", "Error 404: Page not found! Did you upload the correct HTML-files to ESP32 SPIFFS ?");    // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}; //end of Sub ESP32WebServer_handle_notfound()
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// // *** Handler: handle web requests to "/" path ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ESP32WebServer_handle_Root()
{
  Serial.println(F(""));
  Serial.println(F("page / (root) requested and sent"));

  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>ESP32 - start WiFi-Manager or Data Visualization</title></head>";
  s += "<body><h1>Hello from your ESP32 device 'Web-Visualization Demo'!</h1><br/><font size=+1>";
  s += "Your options to see the Web-Visualization demos:<br/><br/>";
  s += "Go to the <a href='open_charts'>graphs page</a> (IP/open_charts) to start Highcharts visualization of ESP32 data.<br/><br/>";
  s += "Go to the <a href='open_logger'>text-log page</a> (IP/open_logger) to start ESP32 serial text logging in broswer.<br/><br/>";
  s += "Use the <a href='curl_loglines_csv'>CSV handler</a> (IP/curl_loglines_csv) to receive ESP32 serial text as CSV items every 1s, using a curl script.<br/>";
  s += "Download 'as file' the Bash script here: <a href='curl_script.sh'>CURL-script</a> (chmod +x it and replace the CURL IP-address with your ESP32's IP!)<br/><br/>";
  s += "Or open the <a href='open_help'>help page</a> (IP/open_help) to learn about device usage and error descriptions in the log output.<br/><br/><br/><br/>";
  s += "</font><hr /><p style=font-family:Courier New>";
  s += "<br/>ESP32 status:<font size=-1>";
  s += String("<br/>Hardware: ") + host;  
  s += String("<br/>Power-on: ") + (Power_On_Seconds / 60) + String(" minutes");  
  strftime(NTP_array22, sizeof(NTP_array22), "%d.%b.%Y %T ", &RTC_time_components);    // %d.%b.%Y %T: 25.Apr.2020 12:09:52      http://www.cplusplus.com/reference/ctime/strftime/
  s += String("<br/>NTP date and time is  ") + NTP_array22;
  // print the ESP32 Hardware Config
  s += String("<br/>- ESP32 CPU FREQ &nbsp;&nbsp;(MHz): ") + getCpuFrequencyMhz();
  s += String("<br/>- ESP32 FLASH FREQ (MHz): ") + (getApbFrequency() / 1000000);
  s += String("<br/>- ESP32 FLASH SIZE &nbsp;(KB): ") + (ESP.getFlashChipSize() / 1024);
  s += String("<br/>- ESP32 SRAM SIZE &nbsp;&nbsp;(KB): ") + (ESP.getHeapSize() / 1024);
  s += String("<br/>- ESP32 FREE SRAM &nbsp;&nbsp;(KB): ") + (ESP.getFreeHeap() / 1024);
  s += String("<br/>- ESP32 CHUNK SRAM &nbsp;(KB): ") + (ESP.getMaxAllocHeap() / 1024);
  s += String("<br/>- ESP32 AVAIL PSRAM (KB): ") + (ESP.getFreePsram() / 1024);
  s += String("<br/><br/>Found the following files on ESP32 SPIFFS:");    
  // print directory of all files in SPIFFS and add them to string
  File root = SPIFFS.open("/");                         // return a SPIFFS File object, it represents the root directory, there is no file with this name on the file system
  File file = root.openNextFile();                      // obtain the first file in the SPIFFS file system on root directory File object, 
  while(file)                                           // returns another File object, now representing an actual file,
  {
    s += String("<br/>&nbsp;") + file.name();           // add file name
    file = root.openNextFile();                         // loop until no file found anymore in "/" (SPIFFS has no directories, just simulating nested folders)
  }     
  s += "</font></p></body></html>\n";
  
  ESP32WebServer.send(200, "text/html", s);             // send the string containing an HTML file as GET-Response back to the browser

} // end of Sub ESP32WebServer_handle_Root
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *** Subroutine: ESP32WebServer_handle_send_chartsHTML file from SPIFFS ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // handler for web requests to "/open_charts" path: send HTML with Highcharts from SPIFFS to Browser
  // sending the HTML of the "Highcharts" Web Frontend (from SPIFFS), after receiving a Get-Request to /open_charts folder
  // the matching "highcharts.html" for this Multi-Line (data1) and Single-Line (data2) charts is attached below  https://arduinodiy.wordpress.com/2019/08/05/
  // "highcharts.html" file is to be loaded into ESP32 SPIFFS by ESP32 Filesystem Uploader ESP32FS Arduino Extension from https://github.com/me-no-dev/arduino-esp32fs-plugin
void ESP32WebServer_handle_send_chartsHTML()
{
  Serial.println(F(""));
  Serial.println(F("page /open_charts requested"));
   
  String pathWithExtension = String("/") + SPIFFS_file_highcharts;       // send Highcharts HTML from SPIFFS to the browser, then Highcharts will send Get-requests to receive the data
  if (SPIFFS.exists(pathWithExtension)) 
  {     
    File file = SPIFFS.open(pathWithExtension, "r");
    size_t sent = ESP32WebServer.streamFile(file, "text/html");         // send the SPIFFS-file containing an HTML file as GET-Response back to the browser
    file.close();

    Serial.print(SPIFFS_file_highcharts);
    Serial.println(F(" found on SPIFFS"));
  }
  else
  {
    String s = "ERROR: file '";
    s += String(pathWithExtension);
    s += String("' not found on ESP32 SPIFFS");
    
    ESP32WebServer.send(200, "text/plain", s);                          // send the file-not-found string containing an HTML file as GET-Response back to the browser
    
    Serial.println(s);
  }
} // end of Sub ESP32WebServer_handle_send_chartsHTML
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *** Subroutine: ESP32WebServer_handle_send_loggerHTML file from SPIFFS ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // handler for web requests to "/open_logger" path: send HTML with Serial-Web-Logger from SPIFFS to Browser
  // sending the HTML of the "Serial-Web-Logger" Web Frontend (from SPIFFS), after receiving a Get-Request to /open_logger folder
  // the matching "seriallogger.html" for the Web-Logger (data3) and JSON script is attached below 
  // "seriallogger.html" file is to be loaded into ESP32 SPIFFS by ESP32 Filesystem Uploader ESP32FS Arduino Extension from https://github.com/me-no-dev/arduino-esp32fs-plugin
void ESP32WebServer_handle_send_loggerHTML()
{
  Serial.println(F(""));
  Serial.println(F("page /open_logger requested"));
  
  String pathWithExtension = String("/") + SPIFFS_file_seriallogger;       // send SerialLogger HTML from SPIFFS to the browser, then SerialLogger will send Get-requests to receive the data
  if (SPIFFS.exists(pathWithExtension)) 
  {     
    File file = SPIFFS.open(pathWithExtension, "r");
    size_t sent = ESP32WebServer.streamFile(file, "text/html");           // send the SPIFFS-file containing an HTML file as GET-Response back to the browser
    file.close();

    Serial.print(SPIFFS_file_seriallogger);
    Serial.println(F(" found on SPIFFS"));
  }
  else
  {
    String s = "ERROR: file '";
    s += String(pathWithExtension);
    s += String("' not found on ESP32 SPIFFS");
    
    ESP32WebServer.send(200, "text/plain", s);                          // send the file-not-found string containing an HTML file as GET-Response back to the browser
    
    Serial.println(s);
  }
} // end of Sub ESP32WebServer_handle_send_loggerHTML
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *** Subroutine: ESP32WebServer_handle_send_helpHTML file from SPIFFS ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // handler for web requests to "/open_help" path: send HTML with Help from SPIFFS to Browser
  // the matching "helpfile.html" for the Web-Logger (data3) and JSON script is attached below 
  // "helpfile.html" file is to be loaded into ESP32 SPIFFS by ESP32 Filesystem Uploader ESP32FS Arduino Extension from https://github.com/me-no-dev/arduino-esp32fs-plugin
void ESP32WebServer_handle_send_helpHTML()
{
  Serial.println(F(""));
  Serial.println(F("page /open_help requested"));
  
  String pathWithExtension = String("/") + SPIFFS_file_helpfile;          // send SerialLogger HTML from SPIFFS to the browser, then SerialLogger will send Get-requests to receive the data
  if (SPIFFS.exists(pathWithExtension)) 
  {     
    File file = SPIFFS.open(pathWithExtension, "r");
    size_t sent = ESP32WebServer.streamFile(file, "text/html");           // send the SPIFFS-file containing an HTML file as GET-Response back to the browser
    file.close();

    Serial.print(SPIFFS_file_helpfile);
    Serial.println(F(" found on SPIFFS"));
  }
  else
  {
    String s = "ERROR: file '";
    s += String(pathWithExtension);
    s += String("' not found on ESP32 SPIFFS");
    
    ESP32WebServer.send(200, "text/plain", s);                           // send the file-not-found string containing an HTML file as GET-Response back to the browser
    
    Serial.println(s);
  }
} // end of Sub ESP32WebServer_handle_send_helpHTML
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *** Subroutine: ESP32WebServer_handle_send_curlscriptHTML file from SPIFFS ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // handler for web requests to "/curl_script.sh" path: allows to download the CURL-script as a file from SPIFFS to Browser
  // sending the HTML with the CURL script (from SPIFFS), after receiving a Get-Request to /script folder
  // the matching "curl_script.sh" to be saved and run in a terminal window is attached below 
  // "curl_script.sh" file is to be loaded into ESP32 SPIFFS by ESP32 Filesystem Uploader ESP32FS Arduino Extension from https://github.com/me-no-dev/arduino-esp32fs-plugin
void ESP32WebServer_handle_send_curlscriptHTML()
{
  Serial.println(F(""));
  Serial.println(F("page /curl_script requested"));

  String pathWithExtension = String("/") + SPIFFS_file_bashscript;        // send CURL-scirpt from SPIFFS to the browser, to be downloaded and run locally
  if (SPIFFS.exists(pathWithExtension)) 
  {     
    File file = SPIFFS.open(pathWithExtension, "r");
    size_t sent = ESP32WebServer.streamFile(file, "text/html");           // send the SPIFFS-file containing an HTML file as GET-Response back to the browser
    file.close();

    Serial.print(SPIFFS_file_bashscript);
    Serial.println(F(" found on SPIFFS"));
  }
  else
  {
    String s = "ERROR: file '";
    s += String(pathWithExtension);
    s += String("' not found on ESP32 SPIFFS");
    
    ESP32WebServer.send(200, "text/plain", s);                            // send the file-not-found string containing an HTML file as GET-Response back to the browser
    
    Serial.println(s);
  }
} // end of Sub ESP32WebServer_handle_send_curlscriptHTML
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *** Subroutine: ESP32WebServer_handle_chart1_json (to Highcharts-HTML) ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // handler for web requests to "/get_chart1_json" path: send back 2 JSON FLOAT values to the JSONparse in the Highcharts script in Web Browser
  // "Highcharts" Web Frontend is sending Get-Requests in intervals for each graph/chart to the ESP32 WebServer to retrieve from /get_chart1_json
  // the data structure to send by ESP32 need to match the receiving JSON strcuture in the "highcharts.html" executed in the Web browser
  // /get_chart1_json sends a JSON object to a Multi-Line chart and needs 2 float as JSON string
void ESP32WebServer_handle_chart1_json()
{
  itoa (random(100),buffer1,10);                            //example to send INT value to Web Browser
  itoa (random(50),buffer2,10);                             //example to send INT value to Web Browser
  //dtostrf(ChargeCurrent, 2, 0, buffer1);                  //example to send FLOAT vaue to Web Browser
  // compile the JSON string for Browser
  // String Buffer to concat Highcharts JSON data upload
  String payload = "{\"JSONdata\":[" + String(buffer1) + "," + String(buffer2) + "]}";
  ESP32WebServer.send(200, "text/plain", payload.c_str());  //send the JSON object as GET-Response back to the browser  
}; // end of Sub ESP32WebServer_handle_chart1_json
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *** Subroutine: ESP32WebServer_handle_chart2_json (to Highcharts-HTML) ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // handler for web requests to "/get_chart2_json" path: send back 1 data to the Highcharts script in Web Browser
  // "Highcharts" Web Frontend is sending Get-Requests in intervals for each graph/chart to the ESP32 WebServer to retrieve from /get_chart2_json
  // the data structure to send by ESP32 need to match the receiving JSON strcuture in the "highcharts.html" executed in the Web browser
  // /get_chart2_json sends JSON data to a Single-Line chart and needs a single float as string
void ESP32WebServer_handle_chart2_json()
{
  itoa (random(100),buffer1,10);                            //example to send INT value to Web Browser
  //dtostrf(ChargeCurrent, 2, 1, buffer1);                  //example to send FLOAT vaue to Web Browser
  // String Buffer to concat Highcharts JSON data upload
  String payload = String(buffer1);                         //prepare single value as string
  ESP32WebServer.send(200, "text/plain", payload.c_str());  //send object as Get-Response back to the web browser
}; // end of Sub ESP32WebServer_handle_chart2_json
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *** Subroutine: ESP32WebServer_handle_loglines_json (to SerialLog-HTML) ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // handler for web requests to "/get_loglines_json" path: send back some INT/FLOAT/Strings to a JSONparse script in Web Browser
  // the "WebLogger" Web Frontend is sending Get-Requests in intervals to the ESP32 WebServer to retrieve from /get_loglines_json
  // /get_loglines_json responds with a string list as JSON object, an empty string will not be written in Web Browser (it's not a line break <br/>)
void ESP32WebServer_handle_loglines_json()
{ 
  // compile the full JSON object with all LogLine strings for the Browser, first value is an INT indicating "number of strings following", 
  // in Browser the JSONparse(JSONdata) accepts : INT, FLOAT, "String"
  LogLine_JsonString = "";                              // start JSON object
  LogLine_JsonString.concat("{\"JSONdata\":[");         // JSON value string
  LogLine_JsonString.concat(LogLine_counter);           // 1st argument: INT as number of following data items in JSON list
  LogLine_JsonString.concat(",");
  for (int ii = 0; ii < LogLine_counter; ii++)          // add all available log line strings (LogLine_counter: 0 = no entry, max 10 lines in arraylist[0-9])
  {
    // cast every Log-Line as "string", to be sent as JSON item which can be parsed into single lines in the Web Browser
    String LogLine_TmpString = "";
    LogLine_TmpString.concat("\"");
    LogLine_TmpString.concat(LogLines_array[ii]);
    LogLine_TmpString.concat("\"");

    LogLine_JsonString.concat(LogLine_TmpString);       // add this log line to the JSON string       
    if ((LogLine_counter - ii) > 1)                     // add comma to JSON string, when a next data item will be appended
    {
      LogLine_JsonString.concat(",");
    }
  }
  LogLine_JsonString.concat("]}");                      // close JSON object
  LogLine_counter = 0;                                  // reset pointer to the next available log line (0 = no entry)
  
  ESP32WebServer.send(200, "text/plain", LogLine_JsonString.c_str()); // send JSON object as Get-Response back to the web browser

}; //end of Sub ESP32WebServer_handle_loglines_json
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *** Subroutine: ESP32WebServer_handle_loglines_csv (to SerialLog-HTML) ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // handler for web curl-requests to "/curl_loglines_csv" path: send some CSV-lines to a script "curling" raw text
  // a CURL script is sending Get-Requests in intervals to the ESP32 WebServer to retrieve from /curl_loglines_csv
  // /curl_loglines_csv responds with a text list as CSV, e.g. to a CURL scirpt or Web Browser
void ESP32WebServer_handle_loglines_csv()
{ 
  // compile the full CSV object with all LogLine strings for the curl/browser, first value is an INT indicating "number of strings following", 
  LogLine_JsonString = "";                              // start CSV line (re-use the JSON string to save memory)
  LogLine_JsonString.concat(LogLine_counter);           // 1st argument: INT as number of following data items
  LogLine_JsonString.concat(",");         
  for (int ii = 0; ii < LogLine_counter; ii++)          // add all available log line strings (e.g. LogLine_counter: 0 = no entry, max 10 lines in arraylist[0-9])
  {
    LogLine_JsonString.concat(LogLines_array[ii]);      // add this log line to the CSV    
    if ((LogLine_counter - ii) > 1)                     // add commas for CSV after each item
    {
      LogLine_JsonString.concat(",");
    }  
  }
  LogLine_JsonString.concat("\r\n");                    // close this CSV line with CR/LF
  LogLine_counter = 0;                                  // reset pointer to the next available log line (0 = no entry)
  
  ESP32WebServer.send(200, "text/plain", LogLine_JsonString.c_str()); // send CSV object as Get-Response back to the curl script or web browser

}; //end of Sub ESP32WebServer_handle_loglines_json
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


//------------------------------------------------------------------------
//   ****** End of subroutines ******
//-------------------------------------------------------------------------




//#########################################################################
//   ****** HTML files, to be stored in SPIFFS by ESP32 Filesystem Uploader (ESP32FS Arduino Extension) ******
//#########################################################################


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *** Bash Script "curl_script.sh" - script to CURL a CSV log-line, write to file, each 1 sec ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
----------------------------------------------------------------------------------------------------------------------

#!/bin/sh
#----------------------------------------------------------
# W.Franke 2020 - CURL script to remotely log ESP32 Serial Web-Log-Lines
# run the script:  ./curl_script.sh  (tested on Raspi and in Mac OS 10.15)
# make it executable after saved first time:  chmod +x curl_script.sh
#----------------------------------------------------------
# make sure to use the IP-Address of your Web-Server (e.g. in a ESP32)
# you may rename the script with the name of the IP/device the script is CURLing
#----------------------------------------------------------
# What is the script doing for you:
# every second, CURL the Solar-Measure values from ESP32 web interface 
# the Script writes every received string into a log file on local hard disk
# at every new hour, a new log file is started
# used file name format in this script:  Solar-20200515_112200.log
# means: hour is on position 16+17 to cut out from file name
# FYI: each second logging ~300 byte => ~1MB/h ; ~25MB/day ; ~1GB/month
#----------------------------------------------------------

# ! ! ! here make sure to use the IP-Address of your Web-Server ! ! !
myIPaddress=192.168.16.xxx
ServerPath="/curl_loglines_csv"

# change the log-file name to your convenience:
file_name_pre="WebLog"


#print start messsage in Terminal
echo "starting to curl $myIPaddress$ServerPath >> $file_name_pre-date_time.log (every second)"

#force main loop to compile an initial file name when starting the script
lastHour=25

#loop forever, until you hit CTRL-C , or close the terminal window
while :
do 
  #if next hour started then start a new log file
  if [ "$(date "+%H")" != "$lastHour" ]
  then
    #compile the new file name
    current_time=$(date "+%Y%m%d_%H%M%S")
    fileName=$file_name_pre-$current_time.log
    # save the current hour as last hour
    lastHour=$(echo $fileName | cut -c 16-17)
    #open a log new file with this hours file name
    echo Start logging INA219 measures: $current_time > "$fileName"
  fi
  #write the received the data from web server int the log file
  #clear
  # CURL options: -s = silent mode / -S = still show error messages
  curl -s -S $myIPaddress$ServerPath >> "$fileName"
  sleep 1
done



----------------------------------------------------------------------------------------------------------------------
*/


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *** template_seriallogger.html - implements for Serial-Web Logger ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
----------------------------------------------------------------------------------------------------------------------

<!DOCTYPE HTML><html>
<!-- WFranke 2020 - Serial Web Logger
-->

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      min-width: 310px;
      max-width: 800px;
      height: 400px;
      margin: 0 auto;
      font-family:monospace,sans-serif;
      font-size:14px;
      font-style:normal;
      line-height: 0.4;
    }
    h2 {
      font-family: Arial;
      font-size: 2.0rem;
      text-align: left;
    }
  </style>
</head>
<body>
  <h2>ESP32 Serial Data Logger in Web Browser</h2>
  <button onClick="StopScroll()">Stop Scrolling</button>
  <button onClick="StartScroll()">Start Scrolling</button>
  <hr />
  <div id="textblock-data3"> <p>Serial data from ESP32 - received by GET request every 1 second.</p></div>
  <hr />
  <button onClick="StopScroll()">Stop Scrolling</button>
  <button onClick="StartScroll()">Start Scrolling</button>
  <div <p></p> </div>
</body>

<script>

var ScrollFlag = 1;

////data3 dynamic text box
setInterval(function ( ) 
{
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() 
  {
    if (this.readyState == 4 && this.status == 200) 
    {
      // parse incoming JSON object into String-Array
      var myObj=JSON.parse(this.responseText);
      // print every single strings by appending it to Browser's text object
      for (var i = 1; i <= myObj.JSONdata[0]; i++) 
      {
        // append one new text line in web browser 
        // source: https://wiki.selfhtml.org/wiki/JavaScript/Tutorials/DOM/DOM-Manipulation
        var container = document.getElementById('textblock-data3');
        var newElm = document.createElement('p');
        newElm.innerText = myObj.JSONdata[i];
        container.appendChild(newElm);

        // scroll to bottom in web browser
        // source: https://jsfiddle.net/L56wxhqm/678/
        if (ScrollFlag == 1) 
        {
          scrollingElement = (document.scrollingElement || document.body)
          scrollingElement.scrollTop = scrollingElement.scrollHeight;
        }
      }
    }
  };
  xhttp.open("GET", "/get_loglines_json", true);
  xhttp.send();
}, 1000 ) ;   // get values from Server every x ms


function StopScroll (id) 
{
  ScrollFlag = 0;
}
function StartScroll (id) 
{
  ScrollFlag = 1;
}

</script>
</html>

----------------------------------------------------------------------------------------------------------------------
*/


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *** template_highcharts.html - implements Multi-Line and Single-Line charts ***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
----------------------------------------------------------------------------------------------------------------------

<!DOCTYPE HTML><html>
<!-- Rui Santos - Complete project details at https://randomnerdtutorials.com/esp32-esp8266-plot-chart-web-server/

W. Franke 2020 - added Multi Line charts
sources:
* https://arduinodiy.wordpress.com/2019/08/05/
* https://gitlab.com/diy_bloke/highcharts/blob/master/index.html
-->

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://code.highcharts.com/highcharts.js"></script>
  <style>
    body {
      min-width: 310px;
      max-width: 800px;
      height: 400px;
      margin: 0 auto;
    }
    h2 {
      font-family: Arial;
      font-size: 2.5rem;
      text-align: center;
    }
  </style>
</head>
<body>
  <h2>ESP32 Random Data for Highcharts</h2>
  <div id="chart-data1" class="container"></div>
  <div <p></p> </div>
  <div <p></p> </div>
  <div <p></p> </div>
  <div <p></p> </div>
  <div <p></p> </div>
  <div id="chart-data2" class="container"></div>
</body>

<script>

//data1 chart
var chartT = new Highcharts.Chart({
  chart:{ renderTo : 'chart-data1' },
  title: { text: 'Random Generator rnd(100)' },
  subtitle: {text: 'subtitle data1'},

  legend: {
        layout: 'vertical',
        align: 'right',
        verticalAlign: 'middle'
    },

  series: [{
    name: 'Line Name 1',
    data: [],
    color: '#059e8a',
    lineWidth: 4,
    showInLegend: true
  }, {
    name: 'Line Name 2',
    data: [],
    color: '#18009c',
    lineWidth: 2,
    showInLegend: true
  }],

  plotOptions: {
    line: { animation: true,
      dataLabels: { enabled: true }
    },
    series: { color: '#059e8a' }
  },

  xAxis: { type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },

  yAxis: {
    title: { text: 'data1 (Celsius)' }
  },

  credits: { enabled: true }
  });

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
     //get single value: y = parseFloat(this.responseText);
     //get multiple data via JSON object
     var x = (new Date()).getTime();
        var myObj=JSON.parse(this.responseText);
        var y=myObj.JSONdata[0]; 
        var z=myObj.JSONdata[1];

      if(chartT.series[0].data.length > 40) {
        chartT.series[0].addPoint([x, y], true, true, true);
        chartT.series[1].addPoint([x, z],true,true,true);
      } else {
        chartT.series[0].addPoint([x, y], true, false, true);
        chartT.series[1].addPoint([x, z],true,false,true);
      }
    }
  };
  xhttp.open("GET", "/get_chart1_json", true);
  xhttp.send();
}, 10000 ) ;   // get values from Server every x ms


//data2 chart
var chartH = new Highcharts.Chart({
  chart:{ renderTo:'chart-data2' },
  title: { text: 'title data2' },
  subtitle: {text: 'subtitle data2'},

  legend: {
        layout: 'vertical',
        align: 'right',
        verticalAlign: 'middle'
    },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    }
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'data2 (%)' }
  },
  credits: { enabled: false }
});
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      //console.log(this.responseText);
      if(chartH.series[0].data.length > 40) {
        chartH.series[0].addPoint([x, y], true, true, true);
      } else {
        chartH.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/get_chart2_json", true);
  xhttp.send();
}, 10000 ) ;   // get values from Server every x ms

</script>

</html>

----------------------------------------------------------------------------------------------------------------------
*/


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// *** template_helpfile.html - show a help file for the IoT device usage and errors***
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
----------------------------------------------------------------------------------------------------------------------

<!DOCTYPE HTML><html>
<!-- WFranke 2020 - Help File
-->

<head>
  <meta charset="UTF-8" name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>
  <title>ESP32 Help & Information</title>
</head>

<body>
<h2>ESP32 Template - Help & Information</h2>
<hr />
<br/>

<font>
Info line 1.<br/><br/>
Info line 2.<br/><br/>
Info line 3.<br/><br/>
Info line 4.<br/><br/>
Info line 5.<br/><br/>
</font>

<p style=font-family:Courier New>
<br/>Error Codes:
<font size=-1>
<br/>Hardware: ESP32 Wemos Lolin OLED Clone
<br/>Software: Web-Logger Template - W.Franke 2020
</font>
</p>

</body>
</html>

----------------------------------------------------------------------------------------------------------------------
*/
