#include<HardwareSerial.h>
#include <WiFi.h>
#define null -999
const char* ssid     = "wifi";
const char* password = "23332333qwq";

class Led;

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
            this->status=0;
            this->quantity=0;
        }
        void get()
        {
            this->quantity=(4095-analogRead(33))/10;
        }
};

class Temperature_humidity_sensor
{
	public:
		int t,h;
		Temperature_humidity_sensor()
		{
			this->t=-999;
			this->h=-999;
		}
		void get()
		{
			this->t=10;
			this->h=20;
			//turn to the real values after we get the hardware
            static void InputInitial(void) //设置端口为输入
            {
                gpio_pad_select_gpio(DHT11_PIN);
                gpio_set_direction(DHT11_PIN, GPIO_MODE_INPUT);
            }

            static void OutputHigh(void) //输出 1
            {
                gpio_pad_select_gpio(DHT11_PIN);
                gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
                gpio_set_level(DHT11_PIN, 1);
            }

            static void OutputLow(void) //输出 0
            {
                gpio_pad_select_gpio(DHT11_PIN);
                gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
                gpio_set_level(DHT11_PIN, 0);
            }
            //位数据 0 和位数据 1 的区别是：数据 1 的高电平时间比数据 0 的时间长，根据这个特 点，读取一个字节的代码如下：

                //读取一个字节数据
                static void COM(void) // 温湿写入
            {
                int i;
                for (i = 0; i < 8; i++)
                {
                    int FLAG = 2;

                    //等待 IO 口变低，变低后，通过延时去判断是 0 还是 1 
                    while((getData()==0)&&FLAG++) ets_delay_us(10); 
                    ets_delay_us(35);//延时 35us
                    int temp = 0;

                    //如果这个位是 1，35us 后，还是 1，否则为 0 
                    if(getData()==1)  temp=1;
                    FLAG=2;

                    //等待 IO 口变高，变高后，表示可以读取下一位 
                    while((getData()==1)&&FLAG++) ets_delay_us(10); 
                    if(FLAG==1) break;
                    comdata <<= 1;
                    comdata |= temp;
                }
            }
        }
};

class Pressure_sensor
{
    public:
        int pressure;
        Pressure_sensor()
        {
            this->pressure=null;
        }
        void get()
        {
            this->pressure=1001;
        }

};

class Weather
{
	public:
		int typ,level;
		//typ:-999->null 1->sunny 2->rainy 3->cloudy 4->misty 5->smoggy 6->snowy 7->sand-dust
		//level:-999->null 1(weak)--->10(strong)
		Weather()
		{
			this->typ=-999;
			this->level=-999;
		}
        void get()
        {
            //weather data vars are according to the api
        }
		void analysis_the_weather(int temp,int humi)//
		{
			this->typ=2;
			this->level=3;
			//the real algorithm will be updated soon
		}
		int check()
		{
			int res=-999;
			//return a number(-1/1) to control the motor
			return res;
		}
};

class Motor
{
    public:
        int status;//1->stretch -1->shrink
        Motor()
        {
            this->status=-1;
            //shrink the shed before it run!
        }
        void set(int sta)
        {
            if (this->status == sta) return;
            if (sta == 1){
                //code: interact with the motor
                this->status=1;
                return;
            }
            if (sta == -1){
                //code: interact with the motor
                this->status=-1;
                return;
            }
        }
};

const int led_num=1;
struct Led_color
{
    int r,g,b;
    Led_color(int rr=0,int gg=0,int bb=0)
    {
        this->r=rr;
        this->g=gg;
        this->b=bb;
    }
};

const Led_color WiFi_disconnect_col=Led_color(255,0,0),WiFi_connect_col=Led_color(0,255,0);

class Led
{
    //0 for off !0 for colors
    public:
        Led_color status[led_num];
        Led()
        {
            
        } 
        void set()
        {
            //code: interact with led pins
            analogWrite(25,this->status[0].b);//blue
            analogWrite(26,this->status[0].r);//red
            analogWrite(27,this->status[0].g);//green
        }
        void set_all(Led_color sta)
        {
            for(int i=0;i<led_num;++i) this->status[i]=sta;
            this->set();
        }
};

Waterdrop_sensor waterdrop_sensor;
Temperature_humidity_sensor temperatrue_humidity_sensor;
Pressure_sensor pressure_sensor;
Weather weather;
Motor motor;
Led led;

void Wifi_Connect()
{
    Serial.printf("\n\nConnecting to %s",ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    led.set_all(WiFi_connect_col);
    Serial.printf("\nWiFi connected\nIP address: ");
    Serial.println(WiFi.localIP());
}

const int Connect_Try_Times=3,Waiting_Times=20;
void Wifi_Check()
{
    if (WiFi.status() != WL_CONNECTED) {
        led.set_all(WiFi_disconnect_col);
        Serial.printf("WiFi Connection Lost!\n");
        for (int i = 0;i < Connect_Try_Times;i++){
            WiFi.disconnect();
            WiFi.begin(ssid, password);
            for (int j = 0;j < Waiting_Times;j++) {
                if (WiFi.status() != WL_CONNECTED) {
                    delay(500);
                    Serial.print(".");
                }
                else break;
            }
            if(WiFi.status() == WL_CONNECTED){
                Serial.printf("Reconnect Successfully!\n");
                led.set_all(WiFi_connect_col);
                return;
            }
            Serial.printf("Reconnect Tring Times: %d\n",i+1);
        }
    }
}

void setup()
{
    Serial.begin(9600);
    pinMode(33,INPUT);
    pinMode(25,OUTPUT);//led
    pinMode(26,OUTPUT);//led
    pinMode(27,OUTPUT);//led
    dht.begin();
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    dht.humidity().getSensor(&sensor);
    delayMS = sensor.min_delay / 1000;
    delay(10);
    led.set_all(WiFi_disconnect_col);
    Wifi_Connect();
}

void loop()
{
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
    Wifi_Check();
    led.set();
    waterdrop_sensor.get();
    printf("water: %d\n",(waterdrop_sensor.quantity));
    delay(1000);
}