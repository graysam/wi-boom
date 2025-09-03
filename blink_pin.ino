#include <WiFi.h>
#include <WebServer.h>

/*PIN ASSIGNMENTS
======================================================================================================================================
RIBBON_ORDER	    BOARD-PIN#	GPIO#       	LED_COLOUR		PURPOSE
======================================================================================================================================
1	                22          5Vin	       		                Vcc
2	                21          GND		                          Ground
3	                18        	12	  			                    PULSE Signal
4	                17	        11		  		                    [Not Assigned] - Reserved for ADC-Input
______________________________________________________________________________________________________________________________________
5	                9	          16        	  AMBER_BICOLOUR		Wait; Wifi not connected
6	                10	        17        	  GREEN_BICOLOUR		Ready; Wi-Fi Connected
--------------------------------------------------------------------------------------------------------------------------------------
^^ AMBER_BICOLOUR & GREEN_BICOLOUR are one physical LED; Only one may be active at a given time.^^
--------------------------------------------------------------------------------------------------------------------------------------
7	                12	        8          	  BLUE			        Pulse signal active
8	                15	        9	            RED			          ARMED & Ready to fire
N/A     	        N/A	        n/a	          GREEN			        5V regulator active (indicates Vcc net is active @5v; power to MCU)
======================================================================================================================================


*/


const int pulseOut        = 12;
const int statusLedAmber  = 16;
const int statusLedGreen  = 17;
const int pulseLed        = 8;
const int armedLed        = 9;
const int ADCReserved     = 11;

int LEDstat = 0;

// WIFI CONFIGURATION:
// assign SSID & Password
const char* ssid = "Trigger-Remote";         // Enter SSID here
const char* password = "lollipop";  // Enter Password here
int counter;
// IPv4 Settings:
IPAddress local_ip(10,11,12,1);
IPAddress gateway(10,11,12,1);
IPAddress subnet(255,255,255,0);
WebServer server(80);

void setup() {
  //Initialise Serial:
  Serial.begin(115200);
  //Initialise outputs, pull pulse pin low
  pinMode(pulseOut, OUTPUT);
  digitalWrite(pulseOut,LOW);

  // Remaining outputs:
  pinMode(statusLedAmber, OUTPUT);
  pinMode(statusLedGreen, OUTPUT);
  pinMode(pulseLed, OUTPUT);
  pinMode(armedLed, OUTPUT);

  // This pin is wired but currently unused. Pull down internally:
  pinMode(ADCReserved, INPUT_PULLDOWN);

  //Send a Greeting on serial console to check and report pins initialised:
  Serial.println("Welcome to TRIGGER(wifi)");
  Serial.println("Serial communication initialised at 115200 baud.");
  Serial.println("In + Out pins initialised.");

  
  // Initialise Wifi library as SoftAP
  Serial.println("Initialising Wifi");

  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);

// Handle connections
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");

}


// THE one and only.... LOOP __--__
void loop() {
  server.handleClient();
  statusLED(2, false);
  delay(500);
  statusLED(1, false);
  delay(500);

}

void LEDControl(int led, int action) {

}

void statusLED(int colour, bool flash) {
  // LED status: 0=off, 1=green, 2=amber
    switch(colour){
      case 0:
      digitalWrite(statusLedAmber,LOW);
      digitalWrite(statusLedGreen,LOW);
      break;
      case 1:
      digitalWrite(statusLedAmber,LOW);
      digitalWrite(statusLedGreen,HIGH);
      break;
      case 2:
      digitalWrite(statusLedAmber,HIGH);
      digitalWrite(statusLedGreen,LOW);
      break;
    }
  }


void handle_OnConnect() {
  counter++;
  server.send(200, "text/html", createHTML());
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

String createHTML() {
  String str = "<!DOCTYPE html> <html>";
  str += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">";
  str += "<style>";
  str += "body {font-family: Arial, sans-serif; color: #444; text-align: center;}";
  str += ".title {font-size: 30px; font-weight: bold; letter-spacing: 2px; margin: 80px 0 55px;}";
  str += ".subtitle {font-size: 20px; font-weight: 200; letter-spacing: 2px; margin: 40px 0 25px;}";
  str += ".counter {font-size: 80px; font-weight: 300; line-height: 1; margin: 0px; color: #4285f4;}";
  str += "</style>";
  str += "</head>";
  str += "<body>";
  str += "<h1 class=\"title\">HV Explosive Trigger</h1>";
  str += "<h2 class=\"subtitle\">Page Load Count:</h2>";
  str += "<div class=\"counter\">";
  str += counter;
  str += "</div>";
  str += "</body>";
  str += "</html>";
  return str;
}
/*
void logger(String(message)) {
  String(logMessage) = message;
  Serial.println(logMessage);
}
*/