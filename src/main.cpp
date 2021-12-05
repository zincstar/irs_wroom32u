#include <WiFi.h>
#define null -999
const char* ssid     = "wifi";
const char* password = "23332333qwq";

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
            this->quantity=analogRead(13);
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

void Wifi_Connect()
{
    Serial.printf("\n\nConnecting to %s",ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nWiFi connected\nIP address: ");
    Serial.println(WiFi.localIP());
}

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

const int led_num=3;
struct Led_color
{
    int r,g,b;
    Led_color()
    {
        this->r=0;
        this->g=0;
        this->b=0;
    }
};

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
        }
        void set_all(Led_color sta)
        {
            for(int i=0;i<led_num;++i) this->status[i]=sta;
        }
};

Waterdrop_sensor waterdrop_sensor;
Temperature_humidity_sensor temperatrue_humidity_sensor;
Pressure_sensor pressure_sensor;
Weather weather;
Motor motor;
Led led;

void setup()
{
    Serial.begin(9600);
    pinMode(13,INPUT);
    delay(10);
    //Wifi_Connect();
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
    waterdrop_sensor.get();
    printf("water: %d\n",waterdrop_sensor.quantity);
    delay(5000);
}