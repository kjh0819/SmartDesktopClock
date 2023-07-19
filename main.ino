#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TM1637Display.h>
#include <string>
#include "DHTesp.h"

const char* ssid = "*****";                                       //WiFi SSID
const char* password = "********";                                //WiFi PW
const char* mqtt_server = "*******************";                  //Mosquitto Brocker
bool DisplayMode = 0;                                             //0:시간 1:날짜
bool mode24 = 0;                                                  //true:24시간 false:am,pm
int alarmyear, alarmmonth, alarmday, alarmhour, alarmminute = 0;  //특정일 날짜 알람
int alarmWeekDay = 7;                                             //7은 존재하지 않는 요일, 설정안됨 상태
int alarmWeekHour, alarmWeekMinute = 0;                           //특정 요일 알람
bool date_alarm_status = 0;                                       //알람 켜짐 꺼짐
bool Week_alarm_status = 0;                                       //알람 켜짐 꺼짐
bool Stop_Alarm_status = 0;                                       //알람 중지 여부, false:중지 true:켜짐
int TimeZone = 9;                                                 //시간대 기본 9
int TimeOffset = 0;                                               //시간 오프셋, 분단위
bool autoBrightness = 0;                                          //자동 밝기
int brightness = 7;                                               //밝기, 기본 7
int Year, Month, day = 0;                                         //날짜저장
uint8_t DisplayData[] = { 0xff, 0xff, 0xff, 0xff };               //세그먼트에 전송할 데이터
int lastDate = 0;                                                 //마지막 출력 날짜,바뀔때만 동작하기 위함
int lasthour = 25;                                                // 시간대 변화 대비, 마지막 시간 기억 00시에 장치를 켤때 대비 25 기본값
int lastmin = 25;                                                 // 분에 변화 있을때만 동작하기 위해 마지막의 시간 기억
unsigned long long lastbuttonTime = 0;                            //마지막에 버튼을 누른 시간
unsigned long long lastTime1sec, lastTime2sec = 0;                //각 초마다 작동할 코드들의 마지막 동작시간 기억\

DHTesp dht;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "kr.pool.ntp.org", TimeZone * 3600, 3600000 / 60);

#define displayCLK D3
#define displayDIO D4
TM1637Display display(displayCLK, displayDIO);

void sendSetting()  //현재 설정값 전송
{
  char tmp[30];
  sprintf(tmp, "%d", (int)DisplayMode);
  client.publish("SmartClock/Status/DisplayMode", tmp);

  sprintf(tmp, "%d", (int)mode24);
  client.publish("SmartClock/Status/mode24", tmp);

  sprintf(tmp, "%d/%d/%d/%d/%d", alarmyear, alarmmonth, alarmday, alarmhour, alarmminute);
  client.publish("SmartClock/Status/alarmdate", tmp);

  sprintf(tmp, "%d/%d/%d", alarmWeekDay, alarmWeekHour, alarmWeekMinute);
  client.publish("SmartClock/Status/alarmWeek", tmp);

  sprintf(tmp, "%d", (int)date_alarm_status);
  client.publish("SmartClock/Status/alarmdate/Status", tmp);

  sprintf(tmp, "%d", (int)Week_alarm_status);
  client.publish("SmartClock/Status/alarmWeek/Status", tmp);

  sprintf(tmp, "%d", TimeZone);
  client.publish("SmartClock/Status/TimeZone", tmp);

  sprintf(tmp, "%d", TimeOffset);
  client.publish("SmartClock/Status/TimeOffset", tmp);

  sprintf(tmp, "%d", (int)autoBrightness);
  client.publish("SmartClock/Status/autoBrightness", tmp);

  sprintf(tmp, "%d", brightness);
  client.publish("SmartClock/Status/brightness", tmp);

  String tmp_date;
  tmp_date += Year;
  tmp_date += "년 ";
  tmp_date += Month;
  tmp_date += "월 ";
  tmp_date += day;
  tmp_date += "일 ";
  switch (timeClient.getDay()) {
    case 0: tmp_date += "일요일"; break;
    case 1: tmp_date += "월요일"; break;
    case 2: tmp_date += "화요일"; break;
    case 3: tmp_date += "수요일"; break;
    case 4: tmp_date += "목요일"; break;
    case 5: tmp_date += "금요일"; break;
    case 6: tmp_date += "토요일"; break;
  }
  client.publish("SmartClock/Status/Date", tmp_date.c_str());
  client.publish("SmartClock/Status/Time", (timeClient.getFormattedTime()).c_str());
}
void gotlight()  //밝기 측정
{
  if (autoBrightness) {
    brightness = analogRead(A0) / 146;
    Serial.print("AutoBrightness : ");
    Serial.println(brightness);
    display.setBrightness(brightness);
    display.setSegments(DisplayData);  // 세그먼트 재전송
  }
}
void SetDate()  //현재 날짜 설정
{
  time_t nowTime;
  struct tm* ts;
  char buf[80];
  timeClient.update();
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    nowTime = timeClient.getEpochTime();
    ts = localtime(&nowTime);
    Year = (ts->tm_year + 1900);
    Month = (ts->tm_mon + 1);
    day = (ts->tm_mday);
  }
}

void alarm()  //알람 출력
{
  if (date_alarm_status && alarmyear == Year && alarmmonth == Month && alarmday == day && alarmhour == timeClient.getHours() && alarmminute == timeClient.getMinutes()) {
    if (Stop_Alarm_status)  //while 사용시 장치 상태 전송이 불가능하여 NodeRed에서 꺼짐으로 나옴
    {
      digitalWrite(D6, 1);
      delay(100);
      digitalWrite(D6, 0);
      Serial.println("ringing");
      delay(100);
    } else return;
  } else if (Week_alarm_status && alarmWeekDay == timeClient.getDay() && alarmWeekHour == timeClient.getHours() && alarmWeekMinute == timeClient.getMinutes()) {
    if (Stop_Alarm_status) {  //알람 정지 여부
      digitalWrite(D6, 1);
      delay(100);
      digitalWrite(D6, 0);
      Serial.println("ringing");
      delay(100);
    } else return;
  } else Stop_Alarm_status = 1;  //알람이 울리는 시간이 아니고 알람이 버튼으로 중지되었다면 알림을 재 활성화
}
void SetAlarmWeek(String Data)  //주 반복 알람 설정
{
  int firstSlash = Data.indexOf('/');
  int secondSlash = Data.indexOf('/', firstSlash + 1);

  alarmWeekDay = Data.substring(0, firstSlash).toInt();
  alarmWeekHour = Data.substring(firstSlash + 1, secondSlash).toInt();
  alarmWeekMinute = Data.substring(secondSlash + 1).toInt();

  Serial.println(alarmWeekDay);
  Serial.println(alarmWeekHour);
  Serial.println(alarmWeekMinute);
}
void SetAlarmDate(String Data)  //특정일 알람 설정
{
  int firstSlash = Data.indexOf('/');
  int secondSlash = Data.indexOf('/', firstSlash + 1);
  int thirdSlash = Data.indexOf('/', secondSlash + 1);
  int fourthSlash = Data.indexOf('/', thirdSlash + 1);

  alarmyear = Data.substring(0, firstSlash).toInt();
  alarmmonth = Data.substring(firstSlash + 1, secondSlash).toInt();
  alarmday = Data.substring(secondSlash + 1, thirdSlash).toInt();
  alarmhour = Data.substring(thirdSlash + 1, fourthSlash).toInt();
  alarmminute = Data.substring(fourthSlash + 1).toInt();
  Serial.println(alarmyear);
  Serial.println(alarmmonth);
  Serial.println(alarmday);
  Serial.println(alarmhour);
  Serial.println(alarmminute);
}

void DisplayDate()  //날짜 출력
{
  if (day != lastDate) {
    if (Month < 10) {
      DisplayData[0] = display.encodeDigit(0);
      DisplayData[1] = display.encodeDigit(Month);
    } else {
      DisplayData[0] = display.encodeDigit(1);
      DisplayData[1] = display.encodeDigit(Month - 10);
    }
    if (day < 10) {
      DisplayData[2] = display.encodeDigit(0);
      DisplayData[3] = display.encodeDigit(day);
    } else if (day < 20) {
      DisplayData[2] = display.encodeDigit(1);
      DisplayData[3] = display.encodeDigit(day - 10);
    } else if (day < 30) {
      DisplayData[2] = display.encodeDigit(2);
      DisplayData[3] = display.encodeDigit(day - 20);
    } else {
      DisplayData[2] = display.encodeDigit(3);
      DisplayData[3] = display.encodeDigit(day - 30);
    }
    display.setSegments(DisplayData);
    Serial.println("Date changed");
    lastDate = day;
  }
}

void DisplayTime()  //시간 출력
{
  int hour = timeClient.getHours();
  int min = timeClient.getMinutes();
  if (lasthour != hour || lastmin != min)  //이전과 시간이 다르면 시간 처리 후 출력
  {
    if (mode24)  //24시 모드 처리
    {
      if (hour < 10) {
        DisplayData[0] = display.encodeDigit(0);
        DisplayData[1] = display.encodeDigit(hour);
      } else if (hour < 20) {
        DisplayData[0] = display.encodeDigit(1);
        DisplayData[1] = display.encodeDigit(hour - 10);
      } else {
        DisplayData[0] = display.encodeDigit(2);
        DisplayData[1] = display.encodeDigit(hour - 20);
      }
    } else  //12시 모드 처리
    {
      if (hour == 0)  //AM 00를 12시로 처리,실제로 올바른 시간은 아니지만 관습적으로 밤 12시라 부름을 고려
      {
        DisplayData[0] = display.encodeDigit(1);
        DisplayData[1] = display.encodeDigit(2);
      } else if (hour < 10) {
        DisplayData[0] = display.encodeDigit(0);
        DisplayData[1] = display.encodeDigit(hour);
      } else if (hour < 13)  //pm 00시 또한 같은이유로 12시로 표기
      {
        DisplayData[0] = display.encodeDigit(1);
        DisplayData[1] = display.encodeDigit(hour - 10);
      } else if (hour < 22) {
        DisplayData[0] = display.encodeDigit(0);
        DisplayData[1] = display.encodeDigit(hour - 12);
      } else if (hour < 22) {
        DisplayData[0] = display.encodeDigit(1);
        DisplayData[1] = display.encodeDigit(hour - 12);
      } else {
        DisplayData[0] = display.encodeDigit(1);
        DisplayData[1] = display.encodeDigit(hour - 22);
      }
    }
    switch (min / 10)  //분 단위 처리
    {
      case 0:
        DisplayData[2] = display.encodeDigit(0);
        DisplayData[3] = display.encodeDigit(min);
        break;
      case 1:
        DisplayData[2] = display.encodeDigit(1);
        DisplayData[3] = display.encodeDigit(min - 10);
        break;
      case 2:
        DisplayData[2] = display.encodeDigit(2);
        DisplayData[3] = display.encodeDigit(min - 20);
        break;
      case 3:
        DisplayData[2] = display.encodeDigit(3);
        DisplayData[3] = display.encodeDigit(min - 30);
        break;
      case 4:
        DisplayData[2] = display.encodeDigit(4);
        DisplayData[3] = display.encodeDigit(min - 40);
        break;
      case 5:
        DisplayData[2] = display.encodeDigit(5);
        DisplayData[3] = display.encodeDigit(min - 50);
        break;
    }
    DisplayData[1] |= 0x80;  // 콜론(:) 추가
    display.setSegments(DisplayData);
    Serial.println("time changed");
    lastmin = min;
    lasthour = hour;
  }
}

ICACHE_RAM_ATTR void button()  //버튼 동작, 인터럽트를 위해 램에 올려둠
{
  if (millis() > lastbuttonTime + 1000)
    if (Stop_Alarm_status &&                                                                                                                                                 //알람이 일시정지 상태가 아닐때
        ((date_alarm_status && alarmyear == Year && alarmmonth == Month && alarmday == day && alarmhour == timeClient.getHours() && alarmminute == timeClient.getMinutes())  //특정일 알람이 동작중
         || (Week_alarm_status && alarmWeekDay == timeClient.getDay() && alarmWeekHour == timeClient.getHours() && alarmWeekMinute == timeClient.getMinutes())))             //혹은 특정 요일 알람 동작중
    {
      Stop_Alarm_status = 0;  //알람 일시정지
      lastbuttonTime = millis();
    } else {
      DisplayMode = !DisplayMode;
      lastbuttonTime = millis();
      Serial.println("ModeChanged " + (int)DisplayMode);
      lastDate = 0;
      lasthour = 25;
    }
}
void setup_display()  //디스플레이 설정
{
  display.setBrightness(brightness);  //7이 최대값 0~7
  display.setSegments(DisplayData);
}

void setup_wifi()  //와이파이 설정
{
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  IPAddress address(192,168,0, 129);
  IPAddress gateway(192,168,0,1);
  //IPAddress mask(255, 255, 0, 0);
  IPAddress mask(255, 255, 255, 0);
  IPAddress DNS1(8, 8, 8, 8);
  IPAddress DNS2(8, 8, 4, 4);
  if (!WiFi.config(address, gateway, mask, DNS1, DNS2))
    Serial.println("STA FAIL");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length)  //MQTT데이터 수신,처리
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String Data;
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    Data += (char)payload[i];
  }
  Serial.println();

  //토픽에 따른처리, 반드시 깊은 토픽을 후순위로 둘것, 깊은게 먼저 작성되면 중복동작함
  if (!strcmp(topic, "SmartClock/mode24")) {
    mode24 = !mode24;
    lasthour = 25;  //세그먼트 업데이트 목적
  } else if (!strcmp(topic, "SmartClock/Status")) {
    sendSetting();
    return;
  } else if (!strcmp(topic, "SmartClock/DisplayMode")) {
    DisplayMode = std::stoi(Data.c_str());
    lasthour = 25;  //세그먼트 업데이트 목적
    lastDate = 0;
  } else if (!strcmp(topic, "SmartClock/Brightness")) {
    brightness = std::stoi(Data.c_str());
    display.setBrightness(brightness);
    display.setSegments(DisplayData);
    //Serial.println("밝기" + Data);
  } else if (!strcmp(topic, "SmartClock/Brightness/Auto")) {
    autoBrightness = std::stoi(Data.c_str());
  } else if (!strcmp(topic, "SmartClock/TimeZone")) {
    TimeZone = (std::stoi(Data.c_str()));  //timezone, gmt Data
    timeClient.setTimeOffset(3600 * TimeZone + TimeOffset);
  } else if (!strcmp(topic, "SmartClock/TimeOffset")) {
    TimeOffset = (std::stoi(Data.c_str()) * 60);  // offset 분단위 조정
    timeClient.setTimeOffset(3600 * TimeZone + TimeOffset);
  } else if (!strcmp(topic, "SmartClock/Alarm/Date")) {
    date_alarm_status = std::stoi(Data.c_str());
  } else if (!strcmp(topic, "SmartClock/Alarm/Date/time")) {
    SetAlarmDate(Data);
  } else if (!strcmp(topic, "SmartClock/Alarm/Week")) {
    Week_alarm_status = std::stoi(Data.c_str());
  } else if ("SmartClock/Alarm/Week/time") {
    SetAlarmWeek(Data);
  }
  sendSetting();
}

void reconnect()  //MQTT설정 및 연결 끊길시 재연결
{
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("SmartClock/mode24");
      client.subscribe("SmartClock/DisplayMode");
      client.subscribe("SmartClock/Brightness");
      client.subscribe("SmartClock/Brightness/Auto");
      client.subscribe("SmartClock/TimeZone");
      client.subscribe("SmartClock/TimeOffset");
      client.subscribe("SmartClock/Alarm/Date");
      client.subscribe("SmartClock/Alarm/Date/time");
      client.subscribe("SmartClock/Alarm/Week");
      client.subscribe("SmartClock/Alarm/Week/time");
      client.subscribe("SmartClock/Status");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void HumTemCheck()  //온습도 측정
{
  if (dht.getStatusString() == "OK") {
    char temp[30];

    float humidity = dht.getHumidity();
    sprintf(temp, "%.1f", humidity);
    client.publish("SmartClock/Hum", temp);

    float temperature = dht.getTemperature();
    sprintf(temp, "%.1f", temperature);
    client.publish("SmartClock/Temp", temp);

    float heatIndex = dht.computeHeatIndex(temperature, humidity, false);
    sprintf(temp, "%.1f", heatIndex);
    client.publish("SmartClock/HeatIndex", temp);
  }
}

void setup() {
  Serial.begin(115200);
  setup_display();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  timeClient.begin();

  DisplayTime();

  dht.setup(D5, DHTesp::DHT11);

  pinMode(5, INPUT_PULLUP);  //버튼 인터럽트, D1
  attachInterrupt(digitalPinToInterrupt(5), button, FALLING);

  pinMode(D6, OUTPUT);  //beep
  digitalWrite(D6, 1);
  delay(100);
  digitalWrite(D6, 0);
}

void loop() {
  SetDate();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if (!DisplayMode)
    DisplayTime();
  else
    DisplayDate();
  alarm();

  if (millis() > lastTime1sec + 1000) {
    gotlight();
    lastTime1sec = millis();
    sendSetting();
  }
  if (millis() > lastTime2sec + 2000) {
    HumTemCheck();
    lastTime2sec = millis();
  }
}
