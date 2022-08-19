// 2021-03-07    Larry Luo	Initial version of Arduino ESP-12F CO2 Webserver

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>

 // Set web server port number to 80
  WiFiServer server(80);
  SoftwareSerial co2Serial(13,2);
  // Variable to store the HTTP request
  String header;
  int SpecailCmd = 0;
  
  // Auxiliar variables to store the current output state
  const int BUTTON_PIN = 4;    // Define pin the button is connected to
  String output5State = "off";
  String output4State = "off";
  
  // Assign output variables to GPIO pins
  const int output5 = 15;
  const int output4 = 12;
  // MH-Z19B Sensor
  int co2ppm, raw = 0;                 // CO2 ppm
  
void readCO2(int cal)                          // Communication MH-Z19 CO2 Sensor
{
  char retByteArray[9];
  char cmd[9] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 }; //Default read CO2
//  Serial.println("cmd=0x"+String(cal, HEX));

  if(cal == 49){
      //request CO2 and temp values                                        //request CO2 and temp values
  } //r[2..3] - "final" CO2 level. r[4]=(T in C) + 40;  r[6],r[7] - if ABC on - counter in "ticks" within a calibration cycle and the number of performed calibration cycles.
  else if(cal == 1){
    cmd[2] = 0x78; cmd[8] = 0x87;                                           //Cmd Changes operation mode and performs MCU reset
    Serial.println("performs MCU reset");
  }
  else if(cal == 2){
    cmd[2] = 0x79; cmd[8] = 0x86;                                           //Cmd ABC Disable
    Serial.println("Cmd ABC Disable");
  }
  else if(cal == 3){
    cmd[2] = 0x79; cmd[3] = 0xa0; cmd[8] = 0xe6;                            //Cmd ABC Enable
    Serial.println("Cmd ABC Enable");
  }
  else if(cal == 4){
    cmd[2] = 0x7d; cmd[8] = 0x82;                                           //Returns ABC logic status (1 - enabled, 0 - disabled)
    Serial.println("r[7] Returns ABC logic status (81 - enabled, 82 - disabled)");
  }
  else if(cal == 5){
      cmd[2] = 0x84; cmd[8] = 0x7b;                                         //r[2.3] light sensor value, r[4.5] is constant 32000.
      Serial.println("r[2.3] light sensor value, r[4.5] is constant 32000.");
  }
  else if(cal == 6){
      cmd[2] = 0x85; cmd[8] = 0x7a;   //b[2..3] is smoothed temperature ADC value. b[4..5] is CO2 level before clamping.
      Serial.println("b[2..3] is smoothed temperature ADC value. b[4..5] is CO2 level before clamping. b[6..7] is minimum light ADC value");
  }
  else if(cal == 7){
      cmd[2] = 0x87; cmd[8] = 0x78;                                         //Zero Calibration
      Serial.println("Zero Calibration start...");
  }
  else if(cal == 8){
      cmd[2] = 0x88; cmd[3] = 0x20; cmd[4] = 0x00; cmd[8] = 0x57;           //Cmd CAL_SPAN_PIONT 5000
      Serial.println("CAL_SPAN_PIONT 8000");
  }
  else if(cal == 9){
      cmd[2] = 0x99; cmd[3] = 0x13; cmd[4] = 0x88; cmd[8] = 0xcb;           //Cmd SET_RANGE 5000
      Serial.println("Cmd SET_RANGE 5000");
  }
  else if(cal == 10){
      cmd[2] = 0x9b; cmd[8] = 0x64;                                         //Cmd GET_RANGE
      Serial.println("Cmd GET_RANGE");
  }
  else if(cal == 20){
      cmd[2] = 0xa0; cmd[8] = 0x5f;                                         //Cmd GET Firmware version, "0430" and "0443" observed
      Serial.println("GET Firmware version");
  }
  else{
  //read CO2
  }
  
  co2Serial.write(cmd, 9);
  delay(100);
    
// The serial stream can get out of sync. The response starts with 0xff, try to resync.
//  while (co2Serial.available() > 0 && (unsigned char)co2Serial.peek() != 0xFF)
//  {
//    co2Serial.read();
//    Serial.print(String(co2Serial.peek(), HEX));
//    Serial.print(",");
//  }

  // Clear the Buffer
  memset(retByteArray, 0, 9);

  if (co2Serial.available() > 0)
  {
    co2Serial.readBytes(retByteArray, 9);
    Serial.println("+++ Response Start 0xff [0] +++ " + String(retByteArray[0], HEX) + " +++");
    Serial.println("+++ Response cmd        [1] +++ " + String(retByteArray[1], HEX) + " +++");
    Serial.println("+++ Response CO2 High   [2] +++ " + String(retByteArray[2], HEX) + " +++");
    Serial.println("+++ Response CO2 Low    [3] +++ " + String(retByteArray[3], HEX) + " +++");
    Serial.println("+++ Response Tempera    [3] +++ " + String(retByteArray[4], HEX) + " +++");
    Serial.println("+++ Response Status     [4] +++ " + String(retByteArray[5], HEX) + " +++");
    Serial.println("+++ Response Unknown    [5] +++ " + String(retByteArray[6], HEX) + " +++");
    Serial.println("+++ Response Unknown    [6] +++ " + String(retByteArray[7], HEX) + " +++");
    Serial.println("+++ Response Unknown    [7] +++ " + String(retByteArray[8], HEX) + " +++");
    Serial.println("+++ Response Checksum   [8] +++ " + String(retByteArray[9], HEX) + " +++");

    if ((retByteArray[0] == 0xFF) && (retByteArray[1] != cmd[1])) {
       byte crc = 0;
       for (int i = 1; i < 8; i++)
       {
         crc += retByteArray[i];
       }
       crc = 255 - crc + 1;

       if (retByteArray[8] == crc)
       {
         if(retByteArray[1] == 0x86){
           int retByteArrayHigh = (int) retByteArray[2]; // CO2 High Byte
           int retByteArrayLow = (int) retByteArray[3];  // CO2 Low Byte
           co2ppm = (256 * retByteArrayHigh) + retByteArrayLow;
           Serial.println("ppm="+String(co2ppm));
         }
         else if(retByteArray[1] == 0x84){
           int retByteArrayHigh = (int) retByteArray[2]; // CO2 High Byte
           int retByteArrayLow = (int) retByteArray[3];  // CO2 Low Byte
           raw = (256 * retByteArrayHigh) + retByteArrayLow;
           Serial.println("raw="+String(raw));
         }       }
       else
       {
         Serial.println("CRC error!");
       }
     }
     else
     {
       Serial.println("First 2 return bytes not correct!");
       while (co2Serial.available() > 0)
       {
         co2Serial.read();
         Serial.print(String(co2Serial.read(), HEX));
         Serial.print(",");
       }
    }
  }
  else
  {
    Serial.println("No response!");
  }
}

// Current time
  unsigned long TcalCurrent,TcalPrevious = millis();
  unsigned long Tcurrent,Tprevious = millis();
  unsigned long currentTime,previousTime; 
  
// Set your Static IP address
  IPAddress local_IP(192, 168, 43, 180);
  IPAddress gateway(192, 168, 43, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(8, 8, 8, 8);   //optional
  IPAddress secondaryDNS(8, 8, 4, 4); //optional

void setup() {
  Serial.begin(115200);
  co2Serial.begin(9600);
// Initialize the output variables as outputs
  pinMode(output5, OUTPUT);
  pinMode(output4, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output5, LOW);
  digitalWrite(output4, LOW);
  
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  } Serial.println();

  WiFi.begin("Linksys", "password123");
  WiFi.mode(WIFI_STA);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
  server.begin();
}

void loop(){
  int btn_Status = HIGH;
  WiFiClient client = server.available(); // Listen for incoming clients
  btn_Status = digitalRead (BUTTON_PIN);  // Check status of button
  if (btn_Status == LOW) {                // Button pushed, so do something
    SpecailCmd = 7;                       // Zero Calibration
    readCO2(SpecailCmd);
    TcalPrevious = millis();
  }

  if(Serial.available()>0)    //Checks is there any data in buffer
  {
    char a = Serial.read();
    Serial.print(a);
    if(a != 10)   //LF
    {
      SpecailCmd = a - '0';
      readCO2(SpecailCmd);
      TcalPrevious = millis();
    }
  }
  //SpecailCmd
  TcalCurrent = millis();
  if(((TcalCurrent - TcalPrevious)> 1200000) && (SpecailCmd > 0)){
    SpecailCmd = 0;
    Serial.println("SpecailCmd stoped.");
  }

  //readCO2
  Tcurrent = millis();
  if(((Tcurrent - Tprevious)> 30000) && (SpecailCmd == 0)){
    readCO2(0);
    Serial.println(String(Tcurrent) + ": CO2 PPM: " + String(co2ppm));
    readCO2(5);
    Serial.println(String(Tcurrent) + ": CO2 PPM: " + String(co2ppm));
    Tprevious = Tcurrent;

  }

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= 2000) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            
            // Web Page Heading
            client.println("<body><h1>ESP8266 Web Server</h1>");
            client.println("CO2=");
            client.println(String(co2ppm)+"<br>");
            client.println("Raw=");
            client.println(String(raw)+"<br>Cmd="+ String(SpecailCmd )+" (0=Read)");
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

/*!
 * \brief Read CO2 from sensor
 * \retval <0
 *      MH-Z19B response error codes.
 * \retval 0..399 ppm
 *      Incorrect values. Minimum value starts at 400ppm outdoor fresh air.
 * \retval 400..1000 ppm
 *      Concentrations typical of occupied indoor spaces with good air exchange.
 * \retval 1000..2000 ppm
 *      Complaints of drowsiness and poor air quality. Ventilation is required.
 * \retval 2000..5000 ppm
 *      Headaches, sleepiness and stagnant, stale, stuffy air. Poor concentration, loss of
 *      attention, increased heart rate and slight nausea may also be present.\n
 *      Higher values are extremely dangerous and cannot be measured.
 */
