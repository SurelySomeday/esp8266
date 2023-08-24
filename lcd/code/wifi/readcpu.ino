/* 作者：flyAkari 会飞的阿卡林 bilibili UID:751219
 * 本代码适用于ESP8266 NodeMCU + 12864显示屏
 * 7pin SPI引脚，正面看，从左到右依次为GND、VCC、D0、D1、RES、DC、CS
 *    ESP8266 ---  OLED
 *      3V    ---  VCC
 *      G     ---  GND
 *      D7    ---  D1
 *      D5    ---  D0
 *      D2orD8---  CS
 *      D1    ---  DC
 *      RST   ---  RES
 * 4pin IIC引脚，正面看，从左到右依次为GND、VCC、SCL、SDA
 *      ESP8266  ---  OLED
 *      3.3V     ---  VCC
 *      G (GND)  ---  GND
 *      D1(GPIO5)---  SCL
 *      D2(GPIO4)---  SDA
 */
#include <Effortless_SPIFFS.h>
#include <ESP8266WiFi.h>
#include <DYWiFiConfig.h>
#include <U8g2lib.h>
//U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/4, /* dc=*/5, /* reset=*/3);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
DYWiFiConfig wificonfig;
ESP8266WebServer webserver(80);
#define DEF_WIFI_SSID     "Tenda_B5ABA0"
#define DEF_WIWI_PASSWORD "sethrollins321"
#define AP_NAME "flyAkari" //dev
String tcpu, scpuuti, scpuclk,tgpu,sgpuuti,sgpuclk, vcpu;//用来存着4个数值
boolean ready=false;
int flag=0;
int len=4;
int failCount=0;
long record=millis();
String url="/sse";
String host = "192.168.0.109";//电脑ip
const int httpPort = 80;
WiFiClient client;
void initdisplay();
void getInfo();
void searchHost();
eSPIFFS fileSystem;

const unsigned char xing[] U8X8_PROGMEM = {
  0x00, 0x00, 0xF8, 0x0F, 0x08, 0x08, 0xF8, 0x0F, 0x08, 0x08, 0xF8, 0x0F, 0x80, 0x00, 0x88, 0x00,
  0xF8, 0x1F, 0x84, 0x00, 0x82, 0x00, 0xF8, 0x0F, 0x80, 0x00, 0x80, 0x00, 0xFE, 0x3F, 0x00, 0x00
};  /*星*/
const unsigned char liu[] U8X8_PROGMEM = { 
  0x40, 0x00, 0x80, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00,
  0x20, 0x02, 0x20, 0x04, 0x10, 0x08, 0x10, 0x10, 0x08, 0x10, 0x04, 0x20, 0x02, 0x20, 0x00, 0x00
};  /*六*/


void setup()
{
  Serial.print("begin...");
  Serial.begin(115200);
  while (!Serial)
    continue;
  initdisplay();
  configWifi();
  searchHost();
}
void configWifi(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.setCursor(0, 14);
  u8g2.print("Waiting for WiFi");
  u8g2.setCursor(0, 30);
  u8g2.print("connection...");
  u8g2.setCursor(0, 47);
  u8g2.print("flyAkari");
  u8g2.setCursor(0, 64);
  u8g2.print("192.168.4.1");
  u8g2.sendBuffer();
  Serial.println("OLED Ready");

  Serial.print("Connecting WiFi...");
  wificonfig.begin(&webserver, "/");
  DYWIFICONFIG_STRUCT defaultConfig = wificonfig.createConfig();
  strcpy(defaultConfig.SSID, DEF_WIFI_SSID);
  strcpy(defaultConfig.SSID_PASSWORD, DEF_WIWI_PASSWORD);
  strcpy(defaultConfig.HOSTNAME, AP_NAME);
  strcpy(defaultConfig.APNAME, AP_NAME);
  wificonfig.setDefaultConfig(defaultConfig);
  wificonfig.enableAP();
  while (WiFi.status() != WL_CONNECTED)
  {
    wificonfig.handle();
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  u8g2.clearBuffer();
  u8g2.print("WiFi connected");
  u8g2.setFont(u8g2_font_profont17_tf);
  u8g2.clearBuffer();
  u8g2.drawStr(0, 25, "please watting...");
  u8g2.sendBuffer();
}
bool readStoreData(){//读取flash
  Serial.println("read host");
  if(fileSystem.openFromFile("/Host.txt", host)&&!host.equals("")) return true;
  else return false;
}
void resetStoreData(){//重置flash
    fileSystem.saveToFile("/Host.txt", "");
    host="";
    delay(1000);
}
void writeStoreData(String str){//写入flash
  bool re=fileSystem.saveToFile("/Host.txt", host);
  Serial.println(re);
}
void searchHost(){//搜索主机ip
  if(readStoreData()) {//先从flash中读取,若读取到则不需要搜索
    Serial.println("read info");
    Serial.println(host);
    ready=true;
    return;
  }
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.drawStr(0, 30, "searching Host...");//显示等待搜索
  u8g2.sendBuffer(); 
  IPAddress ip=WiFi.localIP();
  String res="";//保存ip前3段
  for(int i=0;i<3;i++){
    res=res+ip[i]+".";
  }
  Serial.println("前面部分："+res);
  String temp;
  for(int i=2;i<255;i++){//枚举所有可能的ip，其中1和255排除
      temp=res+String(i);
      Serial.println(temp);
      client.connect(temp, httpPort);
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Accept: text/event-stream\r\n"+
               "Accept-Encoding: gzip, deflate\r\n"+
               "Accept-Language: zh-Hans-CN, zh-Hans; q=0.5\r\n"+
               "Host: " + host + "\r\n"+
               "Connection: close\r\n" +
                     "\r\n");//拼接请求
      delay(50);
      if(!client.connected()) continue;//无法连接的ip,跳过
      else {
        host=temp;//查找成功！存下ip
        writeStoreData(host);//写入flash
        ready=true;//可以开始显示
        break;
      }
  }
  Serial.println("找到主机："+temp);
  if(!ready){//如果无法找到，检查aida64设置
    Serial.println("未到主机！");
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_unifont_t_chinese2);
    u8g2.drawStr(0, 30, "cannot find Host");
    u8g2.sendBuffer(); 
  }
  
}
void refresh()//显示信息
{
  switch(flag){//切换不同的显示
    case 0:
      showMessage("CPU  Temp:",tcpu,"°C");
      break;
    case 1:
      showMessage("CPU  Freq:",scpuclk,"MHZ");
      break;
    case 2:
      showMessage("GPU  Temp:",tgpu,"°C");
      break;
    case 3:
      showMessage("GPU  Freq:",sgpuclk,"MHZ");
      
  }
}
void switchMessage(){//计算当前页面已经显示了多长时间,到时间就切换
    long now=millis();
    if(now-record>4000){
      flag=(flag+1)%len;
      record=now;
    }
}
void showMessage(String name,String info,String unit){//显示信息
    u8g2.clearBuffer();                         // 清空显存
    u8g2.setFont(u8g2_font_profont22_tf); // 选一个合适的字体
    u8g2.setCursor(0,15);
    u8g2.print(name);
    u8g2.setFont(u8g2_font_logisoso30_tf);
    u8g2.setCursor(0,60);
    u8g2.print(info);
    u8g2.setCursor(75,60);
    u8g2.setFont(u8g2_font_logisoso24_tf);
    u8g2.print(unit);
    u8g2.sendBuffer(); // 打到屏幕上
}
void loop()
{
  getInfo();//获取并显示
  switchMessage();//切换显示
}

void initdisplay()
{
  u8g2.begin();
  u8g2.enableUTF8Print();
}
void putValue(String key,String value){//将读到的数据存下
   if (key.equals("TCPU"))
          tcpu=value;
   if (key.equals("SCPUCLK"))
          scpuclk=value;
   if (key.equals("TGPU"))
          tgpu=value;
   if (key.equals("SGPUCLK"))
          sgpuclk=value;
}
void getInfo()//读取信息并显示
{
  if(!ready) return;
  client.connect(host, httpPort);
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Accept: text/event-stream\r\n"+
               "Accept-Encoding: gzip, deflate\r\n"+
               "Accept-Language: zh-Hans-CN, zh-Hans; q=0.5\r\n"+
               "Host: " + host + "\r\n"+
               "Connection: close\r\n" +
                     "\r\n");
  delay(500);
  if(!client.connected()) failCount++;
  if(failCount>=20){//失败 重新查找
    ready=false;
    failCount=0;
    searchHost();
  }
  // Read all the lines of the reply from server and print them to Serial
  Serial.println("Respond:");
  while(client.available()){
    String line = client.readStringUntil('\n');
    if(line.indexOf("{|}")==-1)continue;
    line=line.substring(15);
    Serial.println(line);
    while(true){
      int start=line.indexOf("|");
      int end=line.indexOf("{|}");
      String temp=line.substring(start+1,end);
      int index=temp.indexOf(" ");
      String key=temp.substring(0,index);
      String value=temp.substring(index+1);
      putValue(key,value);
      Serial.println(temp);
      if(end+3<line.length()){
        line=line.substring(end+3);
      }
      else{
        break;
      }
    }
  }
  refresh();
  Serial.println();
  Serial.println("closing connection");
}
