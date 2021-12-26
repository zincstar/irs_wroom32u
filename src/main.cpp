#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <Stepper.h>
#include <WiFi.h>
// #include <WiFiClient.h>
// #include <HTTPClient.h>
#include <ArduinoJson.h>
#include "WeatherNow.h"
#include <WebServer.h>
#include <ESP_Mail_Client.h>
#include "time.h"

#define null -999
#define SMTP_HOST "smtp.163.com"
#define SMTP_PORT 465
/* The sign in credentials */
#define AUTHOR_EMAIL "esp32_station@163.com"
#define AUTHOR_PASSWORD "CJIVAETSOCRBUQCM"

/* Recipient's email*/
#define RECIPIENT_EMAIL "pidtlsj@qq.com"
const char *ssid = "wifi";                      // Wifi name
const char *password = "23332333qwq";           // Wifi password
const char* reqUserKey = "SLrCCb-7P3THqEagb";   // 心知天气api私钥
const char* reqLocation = "39.98:116.35";            // 城市，可使用"ip"自动识别请求 IP 地址
const char* reqUnit = "c";                      // 摄氏(c)/华氏(f)
int LEDState1=1;                               // LED灯的开关状态

// IPAddress local_IP(192, 168, 200, 35);// Set your Static IP address
// IPAddress gateway(192, 168, 200, 103);// Set your Gateway IP address
// IPAddress subnet(255, 255, 255, 0);
// IPAddress primaryDNS(192, 168, 1, 1);   //optional

SMTPSession smtp;
// WiFiServer server(80);
WebServer server(80);
void smtpCallback(SMTP_Status status);
void ESP_Send_Email(int id);
#define PWM_FREQ 32000
#define PWM_RESOLUTION 8
int _adc = 0;
int ADC_MAP[48] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1};
void setupPWM(int pin)
{
    pinMode(pin, OUTPUT);
#ifdef USE_SIGMADELTA
    sigmaDeltaSetup(_adc, PWM_FREQ);
    sigmaDeltaAttachPin(pin, _adc);
#else
    ledcSetup(_adc, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(pin, _adc);
#endif
    ADC_MAP[pin] = _adc;

    _adc = _adc + 1;
}

void analogWrite(int pin, uint8_t val)
{
    if (ADC_MAP[pin] == -1)
        setupPWM(pin);
#ifndef USE_SIGMADELTA
    ledcWrite(ADC_MAP[pin], val);
#else
    sigmaDeltaWrite(ADC_MAP[pin], val);
#endif
}

class Waterdrop_sensor
{
    public:
        int status,quantity;
        Waterdrop_sensor()
        {
            pinMode(33,INPUT);//rain
            this->status=null;
            this->quantity=null;
        }
        void get()
        {
            this->quantity=(4095-analogRead(33))/10;
        }
};

DHT_Unified dht(32, DHT11);
class Temperature_humidity_sensor
{
public:
    float t, h;
    Temperature_humidity_sensor()
    {
        dht.begin();
        sensor_t sensor;
        dht.temperature().getSensor(&sensor);
        dht.humidity().getSensor(&sensor);
        this->t = -999;
        this->h = -999;
    }
    void get()
    {
        // Get temperature event and print its value.
        sensors_event_t event;
        dht.temperature().getEvent(&event);
        if (isnan(event.temperature))
        {
            Serial.println("Error reading temperature!");
        }
        else
        {
            this->t = event.temperature; //C
        }
        // Get humidity event and print its value.
        dht.humidity().getEvent(&event);
        if (isnan(event.relative_humidity))
        {
            Serial.println("Error reading humidity!");
        }
        else
        {
            this->h = event.relative_humidity; //%
        }
    }
};

Adafruit_BMP280 bmp; // use I2C interface
Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();
// 0x76 = CSB to VCC; 0x77 = CSB to GND
class Pressure_sensor
{
public:
    float pressure;
    Pressure_sensor()
    {
        this->pressure = null;
    }
    void get()
    {
        bmp.begin();
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
        sensors_event_t pressure_event;
        bmp_pressure->getEvent(&pressure_event);
        this->pressure=pressure_event.pressure;//hPa
    }
};

const int led_num = 1;
struct Led_color
{
    int r, g, b;
    Led_color(int rr = 0, int gg = 0, int bb = 0)
    {
        this->r = rr;
        this->g = gg;
        this->b = bb;
    }
};

const Led_color WiFi_disconnect_col = Led_color(255, 0, 0);
const Led_color WiFi_connect_col = Led_color(0, 255, 0);
const Led_color Motor_Running = Led_color(0, 0, 255);
const Led_color LED_Off = Led_color(255,255,255);

class Led
{
    //0 for off !0 for colors
public:
    Led_color status[led_num];
    Led()
    {
        pinMode(25,OUTPUT);
        pinMode(26,OUTPUT);
        pinMode(27,OUTPUT);
    }
    void set()
    {
        //code: interact with led pins
        // printf("set_led_col:%d %d %d\n",status[0].b,status[0].r,status[0].g);
        if(LEDState1==0)return;
        analogWrite(25, this->status[0].b); //blue
        analogWrite(26, this->status[0].r); //red
        analogWrite(27, this->status[0].g); //green
    }
    void set_all(Led_color sta)
    {
        for (int i = 0; i < led_num; ++i)
            this->status[i] = sta;
        this->set();
    }
};
Led led;

const int stepsPerRevolution = 2048;
Stepper myStepper(stepsPerRevolution, 19, 5, 18, 17);
class Motor
{
public:
    int status; //1->stretch -1->shrink
    Motor()
    {
        this->status = -1;
        //shrink the shed before it run!
        myStepper.setSpeed(5);
    }
    void set(int sta)
    {
        if (this->status == sta)
            return;
        Led_color las=led.status[0];
        led.set_all(Motor_Running);
        if (sta == 1)
        {
            myStepper.step(stepsPerRevolution/4);
            delay(500);
            myStepper.step(stepsPerRevolution/4);
            delay(500);
            this->status = 1;
        }
        if (sta == -1)
        {
            myStepper.step(-stepsPerRevolution/4);
            delay(500);
            myStepper.step(-stepsPerRevolution/4);
            delay(500);
            this->status = -1;
        }
        led.set_all(las);
        return;
    }
};

Waterdrop_sensor waterdrop_sensor;
Temperature_humidity_sensor temperature_humidity_sensor;
Pressure_sensor pressure_sensor;
WeatherNow weatherNow;
Motor motor;

class Weather
{
public:
    int typ, flex;
    /*
        typ:1/-1
        level:1->normal 2->sensitive
        Reference:https://seniverse.yuque.com/books/share/e52aa43f-8fe9-4ffa-860d-96c0f3cf1c49/yev2c3?inner=d3LpV
        ● 第一优先级：冰雹、雷暴、冰粒、冰针、龙卷风、热带风暴
        ● 第二优先级：雪
        ● 第三优先级：雨
        ● 第四优先级：风和沙尘类（浮尘、扬沙、沙尘暴、风、大风、飓风）
        ● 第五优先级：雾霾
        ● 第六优先级：其他天气现象
    */
    Weather()
    {
        this->typ = -999;
        this->flex = 1;
    }
    void analysis_the_weather(int temp, int humi, int pres,int waterd) //
    {
        if(waterd >= 10)
        {
            this->typ = 1;
        }
        else if (humi >= 80)
        {
            this->typ = 1;
        }
        else if (pres <= 980)
        {
            this->typ = 1;
        }
        else if (this->flex == 2 && (weatherNow.getWeatherCode() <= 29 && weatherNow.getWeatherCode() >= 10))
        {
            this->typ = 1;
        }
        else
        {
            this->typ = -1;
        }
        // this->typ = 2;
        // this->level = 3;
        //the real algorithm will be updated soon
    }
    void check()
    {
        analysis_the_weather(temperature_humidity_sensor.t, temperature_humidity_sensor.h, pressure_sensor.pressure, waterdrop_sensor.quantity);
        motor.set(typ);
        //return a number(-1/1) to control the motor
    }
};
Weather weather;

String myhtmlPage =
    String("") +
    "<!DOCTYPE html><html>" +
    "<head>" +
    "    <meta charset=\"utf-8\">" +
    "    <title>ESP32 WebServer</title>" +
    "    <script>" +
    "       setInterval(function() {" +
    "       getData();" +
    "       }, 10000);" +
    "        function getData() {" +
    "            var xmlhttp = new XMLHttpRequest();" +
    "            xmlhttp.onreadystatechange = function () {" +
    "                if (this.readyState == 4 && this.status == 200) {" +
    "                    try {" +
    "                        var msg = JSON.parse(this.responseText);" +
    "                        var var1 = msg.var1;" +
    "                        var var2 = msg.var2;" +
    "                        var var3 = msg.var3;" +
    "                        var var4 = msg.var4;" +
    "                        var var5 = msg.var5;" +
    "                        document.getElementById(\"SensorData1\").innerHTML = var1;" +
    "                        document.getElementById(\"SensorData2\").innerHTML = var2;" +
    "                        document.getElementById(\"SensorData3\").innerHTML = var3;" +
    "                        document.getElementById(\"SensorData4\").innerHTML = var4;" +
    "                        document.getElementById(\"SensorData5\").innerHTML = var5;" +
    "                    }catch (e) {}" +
    "                }" +
    "            }," +
    "            xmlhttp.open(\"GET\", \"getData\", true); " +
    "            xmlhttp.send();" +
    "        }" +
    "        function SwitchMotor() {" +
    "            var xmlhttp = new XMLHttpRequest();" +
    "            xmlhttp.onreadystatechange = function () {" +
    "                if (this.readyState == 4 && this.status == 200) {" +
    "                    try {" +
    "                        var msg = JSON.parse(this.responseText);" +
    "                        var var1 = msg.var1;" +
    "                        document.getElementById(\"MotorSta\").innerHTML = var1;" +
    "                    }catch (e) {}" +
    "                }" +
    "            }," +
    "            xmlhttp.open(\"GET\", \"SwitchMotor\", true); " +
    "            xmlhttp.send();" +
    "        }" +
    "        function SwitchLED() {" +
    "            var xmlhttp = new XMLHttpRequest();" +
    "            xmlhttp.onreadystatechange = function () {" +
    "                if (this.readyState == 4 && this.status == 200) {" +
    "                    try {" +
    "                        var msg = JSON.parse(this.responseText);" +
    "                        var var1 = msg.var1;" +
    "                        document.getElementById(\"LEDSta\").innerHTML = var1;" +
    "                    }catch (e) {}" +
    "                }" +
    "            }," +
    "            xmlhttp.open(\"GET\", \"SwitchLED\", true); " +
    "            xmlhttp.send();" +
    "        }" +
    "        function EmailReport() {" +
    "            var xmlhttp = new XMLHttpRequest();" +
    "            xmlhttp.open(\"GET\", \"EmailReport\", true); " +
    "            xmlhttp.send();" +
    "        }" +
    "        function SetClothesTime() {" +
    "            var xmlhttp = new XMLHttpRequest();" +
    "            xmlhttp.open(\"GET\", \"SetClothesTime\", true); " +
    "            xmlhttp.send();" +
    "        }" +
    "    </script>" +
    "    <link rel=\"icon\" href=\"https://www.buaa.edu.cn/favicon.ico\">" +
    "    <style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}" +
    "        .button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;" +
    "        text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}" +
    "        .button2 {background-color: #555555;}" +
    "    </style>" +
    "    <style type=\"text/css\">" +
    "        th.titfont{font-size: 22px;font-weight: bold;color: #255e95;background-color:#e9faff;}" +
    "        td.cellfont1{font-size: 18px;background:#f2fbfe;}" +
    "        td.cellfont2{font-size: 18px;background:#fffaaa;}" +
    "    </style>" +
    "</head>" +
    "<body>" +
    "    <h1>ESP32 Web Server</h1>" +
    "    <p>LED_1 - State: </p><div id=\"LEDSta\">On</div>" +
    "    <input type=\"button\" class=\"button button1\" value=\"Switch\" onclick=\"SwitchLED()\"></div>" +
    "    <p>Motor_1 - State: </p><div id=\"MotorSta\">Shrunk</div>" +
    "    <input type=\"button\" class=\"button button1\" value=\"Switch\" onclick=\"SwitchMotor()\"></div>" +
    "    <h2>Email Report</h2>" +
    "    <input type=\"button\" class=\"button button1\" value=\"Send\" onclick=\"EmailReport()\"></div>" +
    "    <h2>Clothes Timer Set</h2>" +
    "    <input type=\"button\" class=\"button button1\" value=\"Set\" onclick=\"SetClothesTime()\"></div>" +
    "    <table width=\"100%\" border=\"0\" cellspacing=\"1\" cellpadding=\"4\" bgcolor=\"#cccccc\" align=\"center\">" +
    "        <caption><font size=\"7\">Monitor Status</font></caption>" +
    "        <tr><th class=\"titfont\">Monitor Name</th><th class=\"titfont\">Value</th></tr>" +
    "        <tr><td class=\"cellfont1\">Temperature</td><td class=\"cellfont2\"><div id=\"SensorData1\">" + (String)temperature_humidity_sensor.t + "</div></td></tr>" +
    "        <tr><td class=\"cellfont1\">Humidity</td><td class=\"cellfont2\"><div id=\"SensorData2\">" + (String)temperature_humidity_sensor.h + "</div></td></tr>" +
    "        <tr><td class=\"cellfont1\">Pressure</td><td class=\"cellfont2\"><div id=\"SensorData3\">" + (String)pressure_sensor.pressure + "</div></td></tr>" +
    "        <tr><td class=\"cellfont1\">Weather</td><td class=\"cellfont2\"><div id=\"SensorData4\">" + weatherNow.getWeatherText() + "</div></td></tr>" +
    "        <tr><td class=\"cellfont1\">Water</td><td class=\"cellfont2\"><div id=\"SensorData5\">" + (String)waterdrop_sensor.quantity + "</div></td></tr>" +
    "    </table>" +
    "</body>" +
    "</html>";

const char* appassword = "123456789";
struct tm Clothes_end_time, Clothes_start_time;
bool Have_clothes = 0;
void Wifi_Connect()
{
    Serial.printf("\n\nConnecting to %s", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    led.set_all(WiFi_connect_col);
    Serial.printf("\nWiFi connected\nIP address: ");
    Serial.println(WiFi.localIP());
    String tmp = WiFi.localIP().toString();
    for (int i = 0; i < tmp.length(); i++)
    {
        if(tmp[i] == '.')
        {
            tmp[i] = '_';
        }
    }
    const char* apssid =  tmp.c_str();
    WiFi.softAP(apssid, appassword);
}

const int Connect_Try_Times = 3, Waiting_Times = 20;
void Wifi_Check()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        led.set_all(WiFi_disconnect_col);
        Serial.printf("WiFi Connection Lost!\n");
        for (int i = 0; i < Connect_Try_Times; i++)
        {
            WiFi.disconnect();
            WiFi.begin(ssid, password);
            for (int j = 0; j < Waiting_Times; j++)
            {
                if (WiFi.status() != WL_CONNECTED)
                {
                    delay(500);
                    Serial.print(".");
                }
                else
                    break;
            }
            if (WiFi.status() == WL_CONNECTED)
            {
                Serial.printf("Reconnect Successfully!\n");
                led.set_all(WiFi_connect_col);
                return;
            }
            Serial.printf("Reconnect Tring Times: %d\n", i + 1);
        }
    }
}

const int api_loop_times=20;
int api_request_counter=0;
void get_weather_api()
{
    api_request_counter--;
    if(WiFi.status() == WL_CONNECTED && api_request_counter <= 0)
    {
        //request from the api
        api_request_counter=api_loop_times;
        if(weatherNow.update())
        {
            Serial.println(weatherNow.getWeatherText());  // 获取当前天气（字符串格式）
            Serial.println(weatherNow.getWeatherCode());// 获取当前天气（整数格式）
            Serial.println(weatherNow.getDegree());     // 获取当前温度数值
        }
        else
        {
            Serial.printf("Request from api failed...\nServer Response: ");
            Serial.println(weatherNow.getServerCode());
        }
    }
}

String header;
unsigned long current_Time=millis();
unsigned long previous_Time=0;
const long timeout_Time=30000;
String Motor_Status_String[2]={"Shrunk","Stretched"};
String Button_Status[2]={"Off","On"};
void Web_Server_Monitor()
{
    /*
    Bug:We cannot turn on/off(the same action) the LED in a short time(5 seconds or so)
    Reason:when you creat a new client, it count the times of commands in header.However, 
    the currentLine was not cleared when an old client still listening.
    Result:solved by clear header once check out an available get line.
    */
    /*WiFiClient client = server.available();
    if(!client.connected())
    {
        header="";
        client.stop();
        previous_Time=current_Time=millis();
        Serial.println("New Client.");
        String currentLine = "";
    }
    if(client.connected())
    {
        Serial.println("Client Connected!!!");
        current_Time=previous_Time=millis();
        while (client.connected() && current_Time - previous_Time <= timeout_Time)
        {
            String currentLine = "";
            current_Time = millis();
            if (client.available()) 
            {
                char c = client.read();
                Serial.write(c);
                header += c;
                if (c == '\n') 
                {
                    if (currentLine.length() == 0) 
                    {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();

                        if (header.indexOf("GET /1/on") >= 0) {
                            Serial.println("LED_1 on");
                            LEDState1 = "on";
                            if(WiFi.status() != WL_CONNECTED) led.set_all(WiFi_disconnect_col);
                            else led.set_all(WiFi_connect_col);
                            header="";
                        } 
                        else if (header.indexOf("GET /1/off") >= 0) {
                            Serial.println("LED_1 off");
                            led.set_all(LED_Off);
                            LEDState1 = "off";
                            header="";
                        }

                        if (header.indexOf("GET /2/on") >= 0) {
                            Serial.println("Motor Stretching...");
                            motor.set(1);
                            header="";
                        } 
                        else if (header.indexOf("GET /2/off") >= 0) {
                            Serial.println("Motor Shrinking...");
                            motor.set(-1);
                            header="";
                        }

                        if (header.indexOf("GET /3/2") >= 0) {
                            ESP_Send_Email(2);
                            header="";
                        } 

                        // Display the HTML web page
                        client.println("<!DOCTYPE html><html>");
                        client.println("<head><meta charset=\"utf-8\" name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("<title>Web Server</title>");
                        client.println("<link rel=\"icon\" href=\"https://www.buaa.edu.cn/favicon.ico\">");
                        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                        client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
                        client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
                        client.println(".button2 {background-color: #555555;}</style>");
                        client.println("<style type=\"text/css\">");
                        client.println("th.titfont{font-size: 22px;font-weight: bold;color: #255e95;background-color:#e9faff;}");
                        client.println("td.cellfont1{font-size: 18px;background:#f2fbfe;}");
                        client.println("td.cellfont2{font-size: 18px;background:#fffaaa;}</style></head>");
                        
                        client.println("<body><h1>ESP32 Web Server</h1>");
                        
                        // Display current state, and ON/OFF buttons for GPIO 26  
                        client.println("<p>LED_1 - State: " + LEDState1 + "</p>");
                        // If the LEDState1 is off, it displays the ON button       
                        if(LEDState1=="off")client.println("<p><a href=\"/1/on\"><button class=\"button\">ON</button></a></p>");
                        else client.println("<p><a href=\"/1/off\"><button class=\"button button2\">OFF</button></a></p>");

                        client.println("<p>Motor_1 - State: " + Motor_Status_String[motor.status==1] + "</p>");  
                        if(motor.status==-1)client.println("<p><a href=\"/2/on\"><button class=\"button\">ON</button></a></p>");
                        else client.println("<p><a href=\"/2/off\"><button class=\"button button2\">OFF</button></a></p>");

                        client.println("<h2>Email Report</h2>"); 
                        client.println("<p><a href=\"/3/2\"><button class=\"button button1\">Send</button></a></p>");

                        client.println("<table width=\"100%\" border=\"0\" cellspacing=\"1\" cellpadding=\"4\" bgcolor=\"#cccccc\" align=\"center\">");
                        client.println("<caption><font size=\"7\">Monitor Status</font></caption>");
                        client.println("<caption>Last update time:" + (String)((current_Time - previous_Time)/1000) + " second(s) ago</caption>");
                        client.println("<tr><th class=\"titfont\">Monitor Name</th><th class=\"titfont\">Value</th></tr>");
                        client.println("<tr><td class=\"cellfont1\">Temperature</td><td class=\"cellfont2\">" + (String)(temperature_humidity_sensor.t) + "</td></tr>");
                        client.println("<tr><td class=\"cellfont1\">Humidity</td><td class=\"cellfont2\">" + (String)(temperature_humidity_sensor.h) + "</td></tr>");
                        client.println("<tr><td class=\"cellfont1\">Pressure</td><td class=\"cellfont2\">" + (String)(pressure_sensor.pressure) + "</td></tr>");
                        client.println("<tr><td class=\"cellfont1\">Weather</td><td class=\"cellfont2\">" + weatherNow.getWeatherText() + "</td></tr>");
                        client.println("<tr><td class=\"cellfont1\">Water</td><td class=\"cellfont2\">" + (String)(waterdrop_sensor.quantity) + "</td></tr>");
                        client.println("</table>");

                        client.println("</body></html>");
                        
                        // The HTTP response ends with another blank line
                        client.println();
                        // Break out of the while loop
                        break;
                    }
                    else currentLine = "";
                } 
                else if (c != '\r') currentLine += c;
            }
        }
    }*/
}

void ESP_Send_Email(int id) // 1->Warning 2->Report
{
    /* Set the callback function to get the sending results */
    smtp.callback(smtpCallback);

    /* Declare the session config data */
    ESP_Mail_Session session;

    /* Set the session config */
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;
    session.login.user_domain = "";

    /* Declare the message class */
    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = "ESP_Canopy_noreply";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "ESP Monitors Report";
    message.addRecipient("Phinney", RECIPIENT_EMAIL);

    /*Send HTML message*/
    String htmlMsg="";
    if(id==1)
    {
        message.subject = "Notice - It's raining now";
        htmlMsg += "<div style=\"color:#2f4468;\"><h1>ESP32 platform has monitored that it is raining now, and your canopy has automatically stretched.</h1>";
    }
    if(id == 1 || id == 2)
    {
        htmlMsg += "<div style=\"color:#2f4468;\"><h2>Monitors Report</h2><table>";
        htmlMsg += "<tr><td style=\"color:#255e95;\"><b>Temperature</b></td><td>" + (String)(temperature_humidity_sensor.t) + "</td></tr>";
        htmlMsg += "<tr><td style=\"color:#255e95;\"><b>Humidity</b></td><td>" + (String)(temperature_humidity_sensor.h) + "</td></tr>";
        htmlMsg += "<tr><td style=\"color:#255e95;\"><b>Pressure</b></td><td>" + (String)(pressure_sensor.pressure) + "</td></tr>";
        htmlMsg += "<tr><td style=\"color:#255e95;\"><b>Weather</b></td><td>" + weatherNow.getWeatherText() + "</td></tr>";
        htmlMsg += "<tr><td style=\"color:#255e95;\"><b>Water</b></td><td>" + (String)(waterdrop_sensor.quantity) + "</td></tr>";
    }
    if(id == 3)
    {
        message.subject = "Notice - You can pick your clothes now.";
        tm time_now;
        getLocalTime(&time_now);
        htmlMsg += "<div style=\"color:#2f4468;\"><h1>Your clothes have been exposed for " + (String)((double)(difftime(mktime(&time_now),mktime(&Clothes_start_time)))/3600) + " h, and you can pick them now.</h1>";
    }

    htmlMsg += "</table><p>- Sent from ESP32 board</p></div>";
    message.html.content = htmlMsg.c_str();
    message.text.charSet = "us-ascii";
    message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    /* Connect to server with the session config */
    if (!smtp.connect(&session))
        return;

    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &message))
        Serial.println("Error sending Email, " + smtp.errorReason());
}

void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
}

void setTimezone(String timezone){
  Serial.printf("  Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

void initTime(String timezone){
  struct tm timeinfo;

  Serial.println("Setting up time");
  configTime(0, 0, "pool.ntp.org");    // First connect to NTP server, with 0 TZ offset
  if(!getLocalTime(&timeinfo)){
    Serial.println("  Failed to obtain time");
    return;
  }
  Serial.println("  Got the time from NTP");
  // Now we can set the real timezone
  setTimezone(timezone);
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time 1");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}

void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
  struct tm tm;

  tm.tm_year = yr - 1900;   // Set date
  tm.tm_mon = month-1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;      // Set time
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;  // 1 or 0
  time_t t = mktime(&tm);
  Serial.printf("Setting time: %s", asctime(&tm));
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}

void check_clothes()
{
    if(!Have_clothes) return;
    tm time_now;
    getLocalTime(&time_now);
    // if(difftime(mktime(&Clothes_end_time),mktime(&Clothes_start_time)) >= 3600 * 24 * 2)
    if(difftime(mktime(&time_now),mktime(&Clothes_start_time)) >= 60)
    {
        ESP_Send_Email(3);
        Have_clothes = 0;
    }
}

void handleRoot() //回调函数
{
    server.send(200, "text/html", myhtmlPage); //！！！注意返回网页需要用"text/html" ！！！
}

void handleData() //回调函数
{
    String var1 = (String)(temperature_humidity_sensor.t) + " °C";
    String var2 = (String)(temperature_humidity_sensor.h) + " %RH";
    String var3 = (String)(pressure_sensor.pressure) + " hPa";
    String var4 = weatherNow.getWeatherText();
    String var5 = (String)(waterdrop_sensor.quantity);
    String jresp = "{\"var1\":\"" + var1 + "\",\"var2\":\"" + var2 + "\",\"var3\":\"" + var3 + "\",\"var4\":\"" + var4 + "\",\"var5\":\"" + var5 + "\"}";
    server.send(200, "application/json", jresp);
}

void handleLED() //回调函数
{
    if(LEDState1==1)led.set_all(LED_Off),LEDState1=0;
    else LEDState1=1,led.set_all((WiFi.status() != WL_CONNECTED)?WiFi_disconnect_col:WiFi_connect_col);
    String var1 = Button_Status[LEDState1];
    String jresp = "{\"var1\":\"" + var1 + "\"}";
    server.send(200, "application/json", jresp);
}

void handleMotor() //回调函数
{
    motor.set((motor.status==1)?(-1):1);
    String var1 = Motor_Status_String[motor.status==1];
    String jresp = "{\"var1\":\"" + var1 + "\"}";
    server.send(200, "application/json", jresp);
    delay(2500);
}

void handleEmail() //回调函数
{
    ESP_Send_Email(2);
    String jresp = "{}";
    server.send(200, "application/json", jresp);
}

void handleClothesTime()
{
    Have_clothes = 1;
    getLocalTime(&Clothes_start_time);
    // Clothes_end_time = Clothes_start_time + ;
    String jresp = "{}";
    server.send(200, "application/json", jresp);
}

void Task1code(void *pvParameters)
{
    for (;;)
    {
        temperature_humidity_sensor.get();
        printf("t:  %f\nh:  %f\n", temperature_humidity_sensor.t, temperature_humidity_sensor.h);
        pressure_sensor.get();
        printf("pr: %f\n", pressure_sensor.pressure);
        led.set();
        waterdrop_sensor.get();
        printf("water: %d\n", (waterdrop_sensor.quantity));
        weather.check();
        delay(2500);
    }
}

void Task2code(void *pvParameters)
{
    led.set_all(WiFi_disconnect_col);
    // if (!WiFi.config(local_IP, gateway, subnet, primaryDNS))
    // {
    //     Serial.println("STA Failed to configure");
    // }
    Wifi_Connect();
    server.on("/", handleRoot);                        //注册链接和回调函数
    server.on("/getData", HTTP_GET, handleData); //注册网页中ajax发送的get方法的请求和回调函数
    server.on("/SwitchLED", HTTP_GET, handleLED); //注册网页中ajax发送的get方法的请求和回调函数
    server.on("/SwitchMotor", HTTP_GET, handleMotor); //注册网页中ajax发送的get方法的请求和回调函数
    server.on("/EmailReport", HTTP_GET, handleEmail); //注册网页中ajax发送的get方法的请求和回调函数
    server.on("/SetClothesTime", HTTP_GET, handleClothesTime); //注册网页中ajax发送的get方法的请求和回调函数
    server.begin();
    weatherNow.config(reqUserKey, reqLocation, reqUnit);
    initTime("<-08>8");
    // printLocalTime();
    for (;;)
    {
        // printLocalTime();
        Wifi_Check();
        if (WiFi.status() == WL_CONNECTED)
        {
            get_weather_api();
            if(Have_clothes)
            {
                check_clothes();
            }
            server.handleClient();
            // Web_Server_Monitor();
        }
        delay(2500);
    }
}

void setup()
{
    Serial.begin(9600);
    delay(10);
    xTaskCreatePinnedToCore(Task1code, "Task1", 50000, NULL, 1, NULL,  0); //mind the stackdepth!!!
    xTaskCreatePinnedToCore(Task2code, "Task2", 10000, NULL, 1, NULL,  1);
    
}

void loop()
{
    delay(5000);    
}