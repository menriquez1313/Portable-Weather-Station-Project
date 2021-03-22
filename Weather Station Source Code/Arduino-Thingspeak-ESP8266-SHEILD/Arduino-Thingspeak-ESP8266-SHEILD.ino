#include <Wire.h>
#include <SoftwareSerial.h>
#include <SparkFunTSL2561.h>
#include <SFE_BMP180.h>
//#include "DHT.h"

//-- IoT Information
#define SSID "xxx"
#define PASS "xxxxyyyy"
#define IP "184.106.153.149" // ThingSpeak IP Address: 184.106.153.149

//Create  DHT22 sensor object.
//DHT dht;

// You will need to create an SFE_BMP180 object, here called "pressure": 
SFE_BMP180 pressure;

// Create an SFE_TSL2561 object, here called "lightSensor": 
SFE_TSL2561 lightSensor;

//-- Software Serial
SoftwareSerial esp(8,9); // RX, TX

// Global variables:
boolean gain;     // Gain setting, 0 = X1, 1 = X16;
unsigned int ms;  // Integration ("shutter") time in milliseconds
// GET /update?key=[THINGSPEAK_KEY]&field1=[data 1]&field2=[data 2]...;
//String GET = "GET /update?key=WAP5BYR50BVXPMAB"; //Random Numbers
String GET = "GET /update?key=XOU7CXJ1C03XA6PH"; //CSUB Fablab Weather Station
//int errorTS = 0;

// function prototypes
void updateTS( String H, String T1, String P, String T2);
void sendCommand(String cmd);
boolean connectWiFi();
//float DHT22Read(int s);
double BMP180Read(int s);
double TSL2561Read();
void printError(byte error);

void setup() 
{
  restart:  //label 
  
  Serial.begin(9600);
  esp.begin(115200);

  sendCommand("AT");
  delay(5000);
  if(esp.find("OK"))
  {
    reconnect:  //label 
    sendCommand("AT+RST");
    Serial.println("RECEIVED: OK\nData ready to sent!");
    if(!connectWiFi())
    {
      Serial.println("ESP8266 Wifi Connect Error!");
      goto reconnect; //go to label "reconnect"
    }
  }
  else
  {
    Serial.println("ESP8266 Serial Connection Error!");
    Serial.println("Soft restting!");
    goto restart; //go to label "restart"
  }

  // Initialize DHT22 sensor. dht.setup(2);

  // Initialize BMP180 sensor.
  if (pressure.begin())
  {
    Serial.println("BMP180 init success");
  }
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    Serial.println("BMP180 init fail\n\n");
    goto restart; //go to label "restart"
  }

    pinMode(A0,INPUT);
    
  // Initialize TSL2561 sensor.
  lightSensor.begin();
  // The light sensor has a default integration time of 402ms,
  // and a default gain of low (1X).
  // If you would like to change either of these, you can
  // do so using the setTiming() command.
  // If gain = false (0), device is set to low gain (1X)
  // If gain = high (1), device is set to high gain (16X)
  gain = 0;
  // If time = 0, integration will be 13.7ms
  // If time = 1, integration will be 101ms
  // If time = 2, integration will be 402ms
  // If time = 3, use manual start / stop to perform your own integration
  unsigned char time = 1;
  // setTiming() will set the third parameter (ms) to the
  // requested integration time in ms (this will be useful later):
  Serial.println("Set timing...");
  lightSensor.setTiming(gain,time,ms);
  // To start taking measurements, power up the sensor:
  Serial.println("Powerup...");
  lightSensor.setPowerUp();
  // The sensor will now gather light during the integration time.
  // After the specified time, you can retrieve the result from the sensor.
  // Once a measurement occurs, another integration period will start.

}

void loop() 
{
  Serial.println(" ");
  String temp2 = String(BMP180Read(0));// turn integer to string
  String pressure = String(BMP180Read(1));// turn integer to string 
  String light = String(TSL2561Read());// turn integer to string
  String soil = String(analogRead(A0));// turn integer to string
  Serial.print("Soil Moisture: "); Serial.println(soil);
  String humidity = soil; //String(DHT22Read(0));// turn integer to string
  String temp1 = temp2; //String(DHT22Read(2));// turn integer to string
  updateTS(humidity,temp1,pressure,temp2,light,soil);
  delay(20000);
}
//----- update the  Thingspeak string with 3 values
void updateTS( String H, String T1, String P, String T2, String L, String S)
{
  // ESP8266 Client
  String cmd = "AT+CIPSTART=\"TCP\",\"";// Setup TCP connection
  cmd += IP;
  cmd += "\",80";
  sendCommand(cmd);
  delay(2000);
  if( esp.find( "Error" ) )
  {
    Serial.print( "RECEIVED: Error\nExit1" );
    return;
  }

  cmd = GET + "&field1=" + H +"&field2="+ T1 + "&field3=" + P + "&field4=" + T2 + "&field5=" + L + "&field6=" + S +"\r\n";
  esp.print( "AT+CIPSEND=" );
  esp.println( cmd.length() );
  if(esp.find( ">" ) )
  {
    Serial.print(">");
    Serial.print(cmd);
    esp.print(cmd);
  }
  else
  {
    sendCommand( "AT+CIPCLOSE" );//close TCP connection
  }
  if( esp.find("OK") )
  {
    Serial.println( "RECEIVED: OK" );
  }
  else
  {
    Serial.println( "RECEIVED: Error\nExit2" );
  }
}

void sendCommand(String cmd)
{
  Serial.print("SEND: ");
  Serial.println(cmd);
  esp.println(cmd);
}

boolean connectWiFi()
{
  esp.println("AT+CWMODE=1");//WiFi STA mode - if '3' it is both client and AP
  delay(2000);
  //Connect to Router with AT+CWJAP="SSID","Password";
  // Check if connected with AT+CWJAP?
  String cmd="AT+CWJAP=\""; // Join accespoint
  cmd+=SSID;
  cmd+="\",\"";
  cmd+=PASS;
  cmd+="\"";
  sendCommand(cmd);
  delay(5000);
  if(esp.find("OK"))
  {
    Serial.println("RECEIVED: OK");
    return true;
  }
  else
  {
    Serial.println("RECEIVED: Error");
    return false;
  }

  cmd = "AT+CIPMUX=0";// Set Single connection
  sendCommand( cmd );
  if( esp.find( "Error") )
  {
    Serial.print( "RECEIVED: Error" );
    return false;
  }
}
/*
float DHT22Read(int s)
{
  delay(dht.getMinimumSamplingPeriod());  // Pause for ~2 seconds.
  float h,t,f;
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  switch (s) 
  {
      case 0:
        h = dht.getHumidity();
        // Check if any reads failed and exit early (to try again).
        if (isnan(h))
          {
            Serial.println("Failed to read from DHT sensor!");
            return 0.0;
          }
          else
          {
            Serial.print("Humidity: ");
            Serial.println(h);
            return h;
          }
        break;

      case 1:
        // Read temperature as Celsius (the default)
        t = dht.getTemperature();
        // Check if any reads failed and exit early (to try again).
        if (isnan(t))
          {
            Serial.println("Failed to read from DHT sensor!");
            return 0.0;
          }
          else
          {
            Serial.print("Temperature: ");
            Serial.print(t);
            Serial.println(" *C ");
            return t;
          }
        break;

      case 2:
        // Read temperature as Celsius (the default)
        t = dht.getTemperature();
        // Read temperature as Fahrenheit (isFahrenheit = true)
        f = dht.toFahrenheit(t);
        // Check if any reads failed and exit early (to try again).
        if (isnan(t) || isnan(f))
          {
            Serial.println("Failed to read from DHT sensor!");
            return 0.0;
          }
          else
          {
            Serial.print("Temperature: ");
            Serial.print(f);
            Serial.println(" *F ");
            return f;
          }
          break;

      default: Serial.println("Wrong Case Selected!\n"); break;
  }
}
*/
double BMP180Read(int s)
{
  char status;
  double T,P,p0,a;
  switch (s) 
  {
      case 0:
        // Start a temperature measurement:
        // If request is successful, the number of ms to wait is returned.
        // If request is unsuccessful, 0 is returned.
        status = pressure.startTemperature();
        if (status != 0)
          {
            // Wait for the measurement to complete:
            delay(status);

            // Retrieve the completed temperature measurement:
            // Note that the measurement is stored in the variable T.
            // Function returns 1 if successful, 0 if failure.
            status = pressure.getTemperature(T);
            if (status != 0)
              {
                // Print out the measurement:
                Serial.print("temperature: ");
                Serial.print(T,2);
                Serial.print(" deg C, ");
                Serial.print((9.0/5.0)*T+32.0,2);
                Serial.println(" deg F");
                return ((9.0/5.0)*T+32.0);
              }
              else Serial.println("error starting temperature measurement\n");
            }
            else Serial.println("error starting temperature measurement\n");

        break;

      case 1:
        // Start a temperature measurement:
        // If request is successful, the number of ms to wait is returned.
        // If request is unsuccessful, 0 is returned.
        status = pressure.startTemperature();
        if (status != 0)
          {
            // Wait for the measurement to complete:
            delay(status);

            // Retrieve the completed temperature measurement:
            // Note that the measurement is stored in the variable T.
            // Function returns 1 if successful, 0 if failure.
            status = pressure.getTemperature(T);
            if (status != 0)
              {
            // Start a pressure measurement:
            // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
            // If request is successful, the number of ms to wait is returned.
            // If request is unsuccessful, 0 is returned.
            status = pressure.startPressure(3);
            if (status != 0)
              {
                // Wait for the measurement to complete:
                delay(status);

                // Retrieve the completed pressure measurement:
                // Note that the measurement is stored in the variable P.
                // Note also that the function requires the previous temperature measurement (T).
                // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
                // Function returns 1 if successful, 0 if failure.

                status = pressure.getPressure(P,T);
                if (status != 0)
                  {
                    // Print out the measurement:
                    Serial.print("absolute pressure: ");
                    Serial.print(P,2);
                    Serial.print(" mb, ");
                    Serial.print(P*0.0295333727,2);
                    Serial.println(" inHg");
                    return P;
                  }
                  else Serial.println("error retrieving pressure measurement\n");
                }
                else Serial.println("error retrieving pressure measurement\n");
              }
              else Serial.println("error starting temperature measurement\n");
            }
            else Serial.println("error starting temperature measurement\n");
        break;

      default: Serial.println("Wrong Case Selected!\n"); break;
  }
}

double TSL2561Read()
{
  // Wait between measurements before retrieving the result
  // (You can also configure the sensor to issue an interrupt
  // when measurements are complete)
  
  // This sketch uses the TSL2561's built-in integration timer.
  // You can also perform your own manual integration timing
  // by setting "time" to 3 (manual) in setTiming(),
  // then performing a manualStart() and a manualStop() as in the below
  // commented statements:
  
  // ms = 1000;
  // light.manualStart();
  delay(ms);
  // light.manualStop();
  
  // Once integration is complete, we'll retrieve the data.
  
  // There are two light sensors on the device, one for visible light
  // and one for infrared. Both sensors are needed for lux calculations.
  
  // Retrieve the data from the device:

  unsigned int data0, data1;
  
  if (lightSensor.getData(data0,data1))
  {
    // getData() returned true, communication was successful
    /*
    Serial.println(" ");
    Serial.print("data0: ");
    Serial.print(data0);
    Serial.print(" data1: ");
    Serial.print(data1);
    */
  
    // To calculate lux, pass all your settings and readings
    // to the getLux() function.
    
    // The getLux() function will return 1 if the calculation
    // was successful, or 0 if one or both of the sensors was
    // saturated (too much light). If this happens, you can
    // reduce the integration time and/or gain.
    // For more information see the hookup guide at: https://learn.sparkfun.com/tutorials/getting-started-with-the-tsl2561-luminosity-sensor
  
    double lux;    // Resulting lux value
    boolean good;  // True if neither sensor is saturated
    
    // Perform lux calculation:

    good = lightSensor.getLux(gain,ms,data0,data1,lux);
    
    // Print out the results:
  
    Serial.print("lux: ");
    Serial.print(lux);
    if (good)
    {
      Serial.println(" (good)");
      return lux;
    } 
    else
    {
      Serial.println(" (BAD)");
    }
  }
  else
  {
    // getData() returned false because of an I2C error, inform the user.

    byte error = lightSensor.getError();
    printError(error);
    return 0;
  }
}

void printError(byte error)
  // If there's an I2C error, this function will
  // print out an explanation.
{
  Serial.print("I2C error: ");
  Serial.print(error,DEC);
  Serial.print(", ");
  
  switch(error)
  {
    case 0:
      Serial.println("success");
      break;
    case 1:
      Serial.println("data too long for transmit buffer");
      break;
    case 2:
      Serial.println("received NACK on address (disconnected?)");
      break;
    case 3:
      Serial.println("received NACK on data");
      break;
    case 4:
      Serial.println("other error");
      break;
    default:
      Serial.println("unknown error");
  }
}

