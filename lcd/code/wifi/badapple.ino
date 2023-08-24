#include <ESP8266WiFi.h>
#include <DYWiFiConfig.h>
#include <U8g2lib.h>
#include <Effortless_SPIFFS.h>
#include <U8x8lib.h>

WiFiClient client;
DYWiFiConfig wificonfig;
ESP8266WebServer webserver(80);
#define DEF_WIFI_SSID     "Tenda_B5ABA0"
#define DEF_WIWI_PASSWORD "sethrollins321"
#define AP_NAME "configWifi" //dev
#define rec_size 704  //每次接收大小
#define pic_num 1098  //字模个数


const char* ssid     = "Tenda_B5ABA0";
const char* password = "sethrollins321";

uint8_t testb[rec_size]={};//存储字模
uint8_t temp[rec_size*2]={};//临时存储未转化的字模
int nowIndex=1;//当前渲染页码
const int httpPort = 8080;//端口
String host="192.168.0.109";//主机ip
String prefix=""; //地址前缀
String type="badapple"; //类型目录
String suffix=".txt"; //文件后缀

int failCount=0;
int maxFailCount=20;
bool ready=false;

eSPIFFS fileSystem;

//U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0,D0,D1,D2,D3);
//U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0,D2,D3); //左到右 5 7 2(CS) 3(不用接)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//U8G2_SSD1306_128X64_ALT0_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);   // same as the NONAME variant, but may solve the "every 2nd line skipped" problem
//U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* reset=*/ 8);

int htoi(uint8_t s[]);
int getNum(char chr);
int convert(uint8_t res[],uint8_t des[]);
void getInfo();

void setup() {

  WiFi.begin(ssid, password);
  Serial.print("begin...");
  Serial.begin(115200);
  while (!Serial)
    continue;
  Serial.print("begin...");
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.setCursor(0, 14);
  u8g2.print("Waiting for WiFi");
  u8g2.sendBuffer();
  configWifi();
}

void loop() {
  getInfo();
} 
void configWifi(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.setCursor(0, 14);
  u8g2.print("Waiting for WiFi");
  u8g2.setCursor(0, 30);
  u8g2.print("connection...");
  u8g2.setCursor(0, 47);
  u8g2.print("wifi:flyAkari");
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
  u8g2.drawStr(0, 28, "please watting...");
  u8g2.sendBuffer();
}
void getInfo(){
  if(WiFi.status()==WL_CONNECTED)
  {
    client.connect(host, httpPort);
    client.print(String("GET ") + prefix+"/"+type+"/"+String(nowIndex)+suffix + " HTTP/1.1\r\n"+
               "Host: " + host + "\r\n"+
               "Connection: colse\r\n\r\n");
  }
  Serial.println(prefix+"/"+type+"/"+String(nowIndex)+suffix);
  nowIndex=nowIndex%pic_num+1;
  delay(150);
  Serial.println(client.available());
  String line="";
  while(client.available()){
     line=client.readStringUntil('\n');
     line.trim();
     Serial.println(line);
     if(line.equals("")) break;
  }
  client.flush();
  Serial.println(client.available());
  while(client.available()){
      client.read(temp, rec_size*2);
      convert(temp,testb);
      u8g2.firstPage();
      do {
         /* all graphics commands have to appear within the loop body. */    
         u8g2.drawXBM(30,0, 86, 64, testb);
      } while (u8g2.nextPage());
  }
  client.stop();
}
int convert(uint8_t res[],uint8_t des[]){
    for(int i=0;i<rec_size;i++){
        int num=getNum(res[2*i])*16+getNum(res[2*i+1]);
        des[i]=num;
    }
}
int htoi(uint8_t s[])
{
    int a=getNum(s[0]);
    int b=getNum(s[1]);
    return a*16+b;
}
int getNum(char chr){
  if(chr>='0'&&chr<='0'){
    return chr-'0';
  }
  else if(chr>='A'&&chr<='Z'){
    return chr-'A'+10;
  }
  else if(chr>='a'&&chr<='z'){
    return chr-'a'+10;
  }
  return 0;
}
