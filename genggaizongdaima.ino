//按键传感器，WIFI
#define INTERVAL_SENSOR   400             //定义传感器采样时间间隔  597000
#define INTERVAL_NET       4000           //定义发送时间
#include <Wire.h>                                  //调用库  
#include "./ESP8266.h"                    
#include "I2Cdev.h"       

//WIFI连接
#define SSID           "Honor 5C"                   // WIFI名
#define PASSWORD       "123698745"                  //密码

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.
 

//onenet数据传输部分
#define HOST_NAME "api.heclouds.com"
//#define HOST_NAME "183.230.40.33"

//OneNet上的设备ID
#define DEVICEID "23777280"
//OneNet上的产品ID
#define PROJECTID "114071"
#define HOST_PORT 80
String apiKey="=ivc9Y7uQMGVzAUv=dHHDkbTsmg=";//与你的设备绑定的APIKey

#define INTERVAL_sensor 500
unsigned long sensorlastTime = millis();

String mCottenData;     //用于存储传感器数据
String jsonToSend;     //用于存储发送的json格式参数

//WIFI
#include <SoftwareSerial.h>
SoftwareSerial mySerial(2, 3);  // 对于Core必须使用软串口进行WIFI模块通信
#define esp8266Serial mySerial   // 定义WIFI模块通信串口

char serialbuffer[1000];  //url储存
String dataToSend;  //AT指令储存
String startcommand;
String sendcommand;
String dataToRead=""; //指令读取

//传感器值的设置 
int sensor_key;      
//SoftwareSerial mySerial(2, 3); /* RX:D3, TX:D2 */
ESP8266 wifi(mySerial);
//ESP8266 wifi(Serial1);                                      //定义一个ESP8266（wifi）的对象
unsigned long net_time1 = millis();                          //数据上传服务器时间
unsigned long sensor_time = millis();                        //传感器采样时间计时器

String postString;                                //用于存储发送数据的字符串


//电机
#include <Microduino_Motor.h>
Motor MotorLeft(MOTOR0_PINA, MOTOR0_PINB);
Motor MotorRight(MOTOR1_PINA, MOTOR1_PINB);//D6，D8控制1A，1B的电机

//按键接在8引脚
#define PIN_KEY D0


void setup() {
  /*
Serial.begin(9600);//串口初始化
 pinMode(PIN_KEY, INPUT);//设置按键输入状态
   esp8266Serial.begin(9600);//connection to ESP8266
   esp8266Init();
   */

//motor
   MotorLeft.begin();
   MotorRight.begin();

/*
//ESP8266初始化
  Serial.print("setup begin\r\n");

  Serial.print("FW Version:");
  Serial.println(wifi.getVersion().c_str());

  if (wifi.setOprToStationSoftAP()) {
    Serial.print("to station + softap ok\r\n");
  } else {
    Serial.print("to station + softap err\r\n");
  }

  if (wifi.joinAP(SSID, PASSWORD)) {      //加入无线网
    Serial.print("Join AP success\r\n");  
    Serial.print("IP: ");
    Serial.println(wifi.getLocalIP().c_str());
  } else {
    Serial.print("Join AP failure\r\n");
  }

  if (wifi.disableMUX()) {
    Serial.print("single ok\r\n");
  } else {
    Serial.print("single err\r\n");
  }
  
  Serial.print("setup end\r\n");
  */
    //初始化串口波特率  
    Wire.begin();
    Serial.begin(115200);   
    while(!Serial);
    //pinMode(sensorPin_1, INPUT);

   //ESP8266初始化
    Serial.print("setup begin\r\n");   

  Serial.print("FW Version:");
  Serial.println(wifi.getVersion().c_str());

  if (wifi.setOprToStationSoftAP()) {
    Serial.print("to station + softap ok\r\n");
  } else {
    Serial.print("to station + softap err\r\n");
  }

  if (wifi.joinAP(SSID, PASSWORD)) {      //加入无线网
    Serial.print("Join AP success\r\n");  
    Serial.print("IP: ");
    Serial.println(wifi.getLocalIP().c_str());
  } else {
    Serial.print("Join AP failure\r\n");
  }

  if (wifi.disableMUX()) {
    Serial.print("single ok\r\n");
  } else {
    Serial.print("single err\r\n");
  }

  Serial.print("setup end\r\n");
    
}

void loop() 
{
  //电机
  Serial.println("Forward!");
  MotorLeft.setSpeed(-300);
  MotorRight.setSpeed(-300);
  
  //灰度传感器
 Serial.println(analogRead(A0));
 Serial.println(analogRead(A2));
  delay(100);
  if(analogRead(A0)>300&&analogRead(A2)<100)
  {
      Serial.println("turn right");
      MotorLeft.setSpeed(-500);
      MotorRight.setSpeed(-100);
  }
  if(analogRead(A2)>300&&analogRead(A0)<100)
  {
     Serial.println("turn left");
      MotorLeft.setSpeed(-100);
      MotorRight.setSpeed(-500);
  }


//上传数据onenet 
  if (sensor_time > millis()) 
  sensor_time = millis();  
    
  if(millis() - sensor_time > INTERVAL_SENSOR)              //传感器采样时间间隔  
  {  
    getSensorData();                                        //读串口中的传感器数据
    sensor_time = millis();
  }  

    
  if (net_time1 > millis())  net_time1 = millis();
  
  if (millis() - net_time1 > INTERVAL_NET)                  //发送数据时间间隔
  {                
    updateSensorData();                                     //将数据上传到服务器的函数
    net_time1 = millis();
  }
  
}
void getSensorData(){    
  //按键判断物体是否被拿走
  if (digitalRead(PIN_KEY))//检测按键状态
  {
    Serial.println("KEY RELEASED");//串口打印按键松开
       sensor_key=0;
 }
 else
 {
 Serial.println("KEY PRESSED");//串口打印按键按下
       sensor_key=1;
 }
    
}

void updateSensorData() {
  if (wifi.createTCP(HOST_NAME, HOST_PORT)) { //建立TCP连接，如果失败，不能发送该数据
    Serial.print("create tcp ok\r\n");
/*
jsonToSend="{\"Temperature\":";
    dtostrf(sensor_tem,1,2,buf);
    jsonToSend+="\""+String(buf)+"\"";
    jsonToSend+=",\"Humidity\":";
    dtostrf(sensor_hum,1,2,buf);
    jsonToSend+="\""+String(buf)+"\"";
    jsonToSend+=",\"Light\":";
    dtostrf(sensor_lux,1,2,buf);
    jsonToSend+="\""+String(buf)+"\"";
    jsonToSend+="}";



    postString="POST /devices/";
    postString+=DEVICEID;
    postString+="/datapoints?type=3 HTTP/1.1";
    postString+="\r\n";
    postString+="api-key:";
    postString+=apiKey;
    postString+="\r\n";
    postString+="Host:api.heclouds.com\r\n";
    postString+="Connection:close\r\n";
    postString+="Content-Length:";
    postString+=jsonToSend.length();
    postString+="\r\n";
    postString+="\r\n";
    postString+=jsonToSend;
    postString+="\r\n";
    postString+="\r\n";
    postString+="\r\n";
*/

char buf[64];
jsonToSend="{\"sensor_keynow\":";
    dtostrf(sensor_key,1,2,buf);
    jsonToSend+="\""+String(buf)+"\"";
    jsonToSend+="}";
    //((void(*)())0)();
    
    postString="POST /devices/";
    postString+=23777280;
    postString+="/datapoints?type=3 HTTP/1.1";
    postString+="\r\n";
    postString+="api-key:";
    postString+=apiKey;
    postString+="\r\n";
    postString+="Host:api.heclouds.com\r\n";
    postString+="Connection:close\r\n";
    postString+="Content-Length:";
    postString+=jsonToSend.length();
    postString+="\r\n";
    postString+="\r\n";
    postString+=jsonToSend;
    postString+="\r\n";
    postString+="\r\n";
    postString+="\r\n"; 
  const char *postArray = postString.c_str();                 //将str转化为char数组
  Serial.println(postArray);
  wifi.send((const uint8_t*)postArray, strlen(postArray));    //send发送命令，参数必须是这两种格式，尤其是(const uint8_t*)
  Serial.println("send success");   
     if (wifi.releaseTCP()) {                                 //释放TCP连接
        Serial.print("release tcp ok\r\n");
        } 
     else {
        Serial.print("release tcp err\r\n");
        }
      postArray = NULL;                                       //清空数组，等待下次传输数据
  
  } else {
    Serial.print("create tcp err\r\n");
  }
  
}

void esp8266Init() {
  esp8266Serial.println("AT");
  delay(500);
  esp8266Serial.println("AT+RST");
  delay(500);
  Serial.println("SETUP FINISHED!WELCOME! ");
}
