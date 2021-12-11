#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <MS5xxx.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "WeatherNow.h"
#include <WebServer.h>
#define null -999
const char *ssid = "wifi";                      // Wifi name
const char *password = "23332333qwq";           // Wifi password
const char* reqUserKey = "SuzImoB5Dv06BZmNU";   // 心知天气api私钥
const char* reqLocation = "beijing";            // 城市，可使用"ip"自动识别请求 IP 地址
const char* reqUnit = "c";                      // 摄氏(c)/华氏(f)
String LEDState1="on";                               // LED灯的开关状态

WiFiServer server(80);

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

MS5xxx ms5611(&Wire);   // 0x76 = CSB to VCC; 0x77 = CSB to GND
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
        ms5611.ReadProm();
        ms5611.Readout();
        this->pressure = ms5611.GetPres();
    }
};

class Weather
{
public:
    int typ, level;
    //typ:-999->null 1->sunny 2->rainy 3->cloudy 4->misty 5->smoggy 6->snowy 7->sand-dust
    //level:-999->null 1(weak)--->10(strong)
    Weather()
    {
        this->typ = -999;
        this->level = -999;
    }
    void get()
    {
        //weather data vars are according to the api
    }
    void analysis_the_weather(int temp, int humi) //
    {
        this->typ = 2;
        this->level = 3;
        //the real algorithm will be updated soon
    }
    int check()
    {
        int res = -999;
        //return a number(-1/1) to control the motor
        return res;
    }
};

class Motor
{
public:
    int status; //1->stretch -1->shrink
    Motor()
    {
        this->status = -1;
        //shrink the shed before it run!
    }
    void set(int sta)
    {
        if (this->status == sta)
            return;
        if (sta == 1)
        {
            //code: interact with the motor
            this->status = 1;
            return;
        }
        if (sta == -1)
        {
            //code: interact with the motor
            this->status = -1;
            return;
        }
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
        printf("set_led_col:%d %d %d\n",status[0].b,status[0].r,status[0].g);
        if(LEDState1=="off")return;
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

Waterdrop_sensor waterdrop_sensor;
Temperature_humidity_sensor temperatrue_humidity_sensor;
Pressure_sensor pressure_sensor;
WeatherNow weatherNow;
Weather weather;
Motor motor;
Led led;

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
            Serial.print(weatherNow.getWeatherText());  // 获取当前天气（字符串格式）
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
const long timeout_Time=2000;
void Web_Server_Monitor()
{
    /*
    Bug:We cannot turn on/off(the same action) the LED in a short time(5 seconds or so)
    Reason:when you creat a new client, it count the times of commands in header.However, 
    the currentLine was not cleared when an old client still listening.
    Result:solved by clear header once check out an available get line.
    */
    WiFiClient client = server.available();
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

                        // Display the HTML web page
                        client.println("<!DOCTYPE html><html>");
                        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("<link rel=\"icon\" href=\"data:,\">");
                        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                        client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
                        client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
                        client.println(".button2 {background-color: #555555;}</style></head>");
                        
                        client.println("<body><h1>ESP32 Web Server</h1>");
                        
                        // Display current state, and ON/OFF buttons for GPIO 26  
                        client.println("<p>LED_1 - State " + LEDState1 + "</p>");
                        // If the LEDState1 is off, it displays the ON button       
                        if(LEDState1=="off")client.println("<p><a href=\"/1/on\"><button class=\"button\">ON</button></a></p>");
                        else client.println("<p><a href=\"/1/off\"><button class=\"button button2\">OFF</button></a></p>");
                        
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
    }
}

void setup()
{
    Serial.begin(9600);
    delay(10);
    led.set_all(WiFi_disconnect_col);
    Wifi_Connect();
    server.begin();
    weatherNow.config(reqUserKey, reqLocation, reqUnit);
}

void loop()
{
    temperatrue_humidity_sensor.get();
    printf("t: %f\n h: %f\n", temperatrue_humidity_sensor.t, temperatrue_humidity_sensor.h);
    pressure_sensor.get();
    printf("pr: %f\n",pressure_sensor.pressure);
    Wifi_Check();
    if(WiFi.status() == WL_CONNECTED)
    {
        get_weather_api();
        Web_Server_Monitor();
    }
    led.set();
    waterdrop_sensor.get();
    printf("water: %d\n", (waterdrop_sensor.quantity));
    delay(1000);

    //test if i2c connected
    /*
        byte error, address;
    int nDevices;
    Serial.println("Scanning...");
    nDevices = 0;
    for(address = 1; address < 127; address++ ) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
        Serial.print("I2C device found at address 0x");
        if (address<16) {
            Serial.print("0");
        }
        Serial.println(address,HEX);
        nDevices++;
        }
        else if (error==4) {
        Serial.print("Unknow error at address 0x");
        if (address<16) {
            Serial.print("0");
        }
        Serial.println(address,HEX);
        }    
    }
    if (nDevices == 0) {
        Serial.println("No I2C devices found\n");
    }
    else {
        Serial.println("done\n");
    }
    */
    delay(5000);    
}

//tcp example:
    /*
    const char* host = "www.baidu.com";
    Serial.print("connecting to ");
    Serial.println(host);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
    }

    // We now create a URI for the request
    String url = "/";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

    // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }
    */
   /*
void InputInitial(void) //设置端口为输入
        {
            gpio_pad_select_gpio(DHT11_PIN);
            gpio_set_direction(DHT11_PIN, GPIO_MODE_INPUT);
        }
        void OutputHigh(void) //输出 1
        {
            gpio_pad_select_gpio(DHT11_PIN);
            gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
            gpio_set_level(DHT11_PIN, 1);
        }
        void OutputLow(void) //输出 0
        {
            gpio_pad_select_gpio(DHT11_PIN);
            gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
            gpio_set_level(DHT11_PIN, 0);
        }
        //位数据 0 和位数据 1 的区别是：数据 1 的高电平时间比数据 0 的时间长，根据这个特 点，读取一个字节的代码如下：

        //读取一个字节数据
        void get_one_byte(void) // 温湿写入
        {
            int comdata=0;
            for (int i = 0; i < 8; i++)
            {
                int FLAG = 2;

                //等待 IO 口变低，变低后，通过延时去判断是 0 还是 1
                while ((getData() == 0) && FLAG++)
                    ets_delay_us(10);
                ets_delay_us(35); //延时 35us
                int temp = 0;

                //如果这个位是 1，35us 后，还是 1，否则为 0
                if (getData() == 1)
                    temp = 1;
                FLAG = 2;

                //等待 IO 口变高，变高后，表示可以读取下一位
                while ((getData() == 1) && FLAG++)
                    ets_delay_us(10);
                if (FLAG == 1)
                    break;
                comdata <<= 1;
                comdata |= temp;
            }
        }
        */