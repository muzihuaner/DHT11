#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <WiFiUdp.h>

const char* ssid = "HUANWIFI";            // 替换为你的 WiFi 名称
const char* password = "asdfghjkl...";    // 替换为你的 WiFi 密码

const char* apiKey = "Sd8_LszRLgx94kS2m"; // 心知天气 API Key
const char* city = "datong";              // 城市

const char* ntpServer = "ntp.aliyun.com";
const long  gmtOffset_sec = 8 * 3600;
const int   daylightOffset_sec = 0;

#define DHTPIN 4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);
WiFiUDP ntpUDP;

float temperature = 0;
float humidity = 0;
String ntpTime = "--";
String ntpDate = "--";
String ntpWeekday = "--";
bool ntpSynced = false;
unsigned long bootMillis = 0;
unsigned long lastNtpSync = 0;

String forecastDate[5];
String forecastDayText[5];
String forecastNightText[5];
String forecastCode[5];
int    forecastHigh[5];
int    forecastLow[5];
String forecastWindDir[5];
int    forecastWindScale[5];
int    forecastHumidity[5];
float  forecastRainfall[5];
int    forecastCount = 0;

unsigned long lastWeatherFetch = 0;
const unsigned long weatherInterval = 600000;

const char NTP_PACKET_BASE[48] = {
  0x1B, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#f4f2ee">
<title>室内环境监测</title>
<style>
:root{
  --bg:#f4f2ee;
  --surface:#fff;
  --line:#e3dfd8;
  --line-soft:#f0ede7;
  --ink:#1b1917;
  --ink-2:#6c665d;
  --ink-3:#a29a8f;
  --warm:#b0492a;
  --cool:#0e6b62;
}
*{margin:0;padding:0;box-sizing:border-box}
html{-webkit-text-size-adjust:100%}
body{
  background:var(--bg);color:var(--ink);
  font:400 15px/1.55 -apple-system,BlinkMacSystemFont,"Segoe UI","PingFang SC","Hiragino Sans GB","Microsoft YaHei",sans-serif;
  -webkit-font-smoothing:antialiased;
  padding:32px 20px 56px;
}
.wrap{max-width:760px;margin:0 auto}
.mono{font-family:ui-monospace,SFMono-Regular,"SF Mono",Menlo,Consolas,monospace;font-variant-numeric:tabular-nums}

/* ---------- 头部 ---------- */
.top{display:flex;justify-content:space-between;align-items:flex-end;gap:16px;flex-wrap:wrap;padding-bottom:16px;border-bottom:1px solid var(--line)}
.top h1{font-size:15px;font-weight:600}
.top p{font-size:12px;color:var(--ink-3);margin-top:3px}
.status{display:flex;gap:16px;font-size:12px;color:var(--ink-2);flex-wrap:wrap}
.status span{white-space:nowrap}
.status i{display:inline-block;width:6px;height:6px;border-radius:50%;background:var(--ink-3);margin-right:6px;vertical-align:1px}
.status i.ok{background:#3f8f6d}
.status i.bad{background:#c1553a}

/* ---------- 时钟 ---------- */
.clock{padding:30px 0 26px}
.clock .t{font-size:clamp(40px,10vw,60px);font-weight:500;letter-spacing:-.02em;line-height:1}
.clock .d{font-size:13px;color:var(--ink-2);margin-top:10px}

/* ---------- 数据卡片 ---------- */
.stats{display:grid;grid-template-columns:repeat(3,1fr);background:var(--surface);border:1px solid var(--line);border-radius:12px;overflow:hidden}
.stat{padding:18px 20px;border-right:1px solid var(--line-soft)}
.stat:last-child{border-right:none}
.stat .k{font-size:11px;letter-spacing:.08em;color:var(--ink-3)}
.stat .val{margin-top:9px;font-size:26px;font-weight:500;line-height:1}
.stat .val small{font-size:13px;font-weight:400;color:var(--ink-3);margin-left:2px}
.stat .val.warm{color:var(--warm)}
.stat .val.cool{color:var(--cool)}

/* ---------- 天气 ---------- */
.sec{margin-top:36px}
.sec h2{font-size:12px;font-weight:600;letter-spacing:.1em;color:var(--ink-3);margin-bottom:12px}
.list{background:var(--surface);border:1px solid var(--line);border-radius:12px;overflow:hidden}
.row{display:grid;grid-template-columns:64px 1fr 1fr 92px;align-items:center;gap:14px;padding:14px 20px;border-bottom:1px solid var(--line-soft);font-size:14px}
.row:last-child{border-bottom:none}
.row .dt{font-size:13px;color:var(--ink-2)}
.row .cd{display:flex;align-items:center;gap:9px}
.row .cd .ico{font-size:17px;line-height:1}
.row .mt{font-size:12px;color:var(--ink-3)}
.row .tp{text-align:right}
.row .tp .hi{font-weight:600}
.row .tp .lo{color:var(--ink-3);margin-left:6px}
.empty{padding:22px;text-align:center;color:var(--ink-3);font-size:13px}

/* ---------- 页脚 ---------- */
.foot{margin-top:20px;font-size:12px;color:var(--ink-3)}

@media(max-width:600px){
  body{padding:20px 16px 40px}
  .clock{padding:22px 0 20px}
  .stat{padding:14px}
  .stat .val{font-size:21px}
  .row{grid-template-columns:56px 1fr 84px;gap:10px;padding:12px 14px;font-size:13.5px}
  .row .mt{display:none}
}
</style>
</head>
<body>
<div class="wrap">

  <header class="top">
    <div>
      <h1>室内环境监测</h1>
      <p>ESP32 · DHT11</p>
    </div>
    <div class="status">
      <span><i id="wifiDot"></i><span id="wifiText">连接中</span></span>
      <span><i id="ntpDot"></i><span id="ntpText">对时中</span></span>
    </div>
  </header>

  <section class="clock">
    <div class="t mono" id="time">--:--:--</div>
    <div class="d" id="date">--</div>
  </section>

  <section class="stats">
    <div class="stat">
      <div class="k">温度</div>
      <div class="val warm mono"><span id="temp">--</span><small>°C</small></div>
    </div>
    <div class="stat">
      <div class="k">湿度</div>
      <div class="val cool mono"><span id="humid">--</span><small>%</small></div>
    </div>
    <div class="stat">
      <div class="k">运行时长</div>
      <div class="val mono" id="uptime">--:--:--</div>
    </div>
  </section>

  <section class="sec">
    <h2>未来天气 · 大同</h2>
    <div class="list" id="forecast"><div class="empty">读取中…</div></div>
  </section>

  <div class="foot" id="footer">正在连接设备…</div>

</div>

<script>
var ICONS={0:"☀️",1:"🌙",2:"⛅",3:"☁️",4:"☁️",5:"☀️",6:"☀️",7:"☀️",8:"☀️",
           9:"🌧️",10:"🌧️",11:"🌫️",12:"🌦️",13:"🌦️",14:"🌧️",15:"⛈️",16:"🌨️",17:"🌨️",18:"🌧️"};
var WK=["日","一","二","三","四","五","六"];
var clockBase=null, upBase=null;

function $(id){return document.getElementById(id);}
function pad(n){return n<10?"0"+n:""+n;}

function tickClock(){
  if(!clockBase) return;
  var t=Math.floor((clockBase.sec+(Date.now()-clockBase.at)/1000)%86400);
  $("time").textContent=pad(Math.floor(t/3600))+":"+pad(Math.floor(t/60)%60)+":"+pad(t%60);
}

function tickUptime(){
  if(!upBase) return;
  var s=upBase.sec+Math.floor((Date.now()-upBase.at)/1000);
  $("uptime").textContent=pad(Math.floor(s/3600))+":"+pad(Math.floor(s/60)%60)+":"+pad(s%60);
}

function render(d){
  $("temp").textContent=d.indoor.temp;
  $("humid").textContent=d.indoor.humid;

  var p=d.time.split(":");
  clockBase={sec:(+p[0])*3600+(+p[1])*60+(+p[2]),at:Date.now()};
  tickClock();

  var dp=d.date.split("-");
  var dt=new Date(+dp[0],+dp[1]-1,+dp[2]);
  $("date").textContent=d.date+"　星期"+WK[dt.getDay()]+"　"+d.timezone;

  upBase={sec:d.uptimeSec,at:Date.now()};
  tickUptime();

  $("wifiDot").className=d.wifiConnected?"ok":"bad";
  $("wifiText").textContent=d.wifiConnected?(d.ip||"已连接"):"未连接";
  $("ntpDot").className=d.ntpSynced?"ok":"bad";
  $("ntpText").textContent=d.ntpSynced?"时间已同步":"时间未同步";

  var html="";
  if(!d.forecast||d.forecast.length===0){
    html='<div class="empty">暂无天气数据</div>';
  }else{
    d.forecast.forEach(function(f){
      html+='<div class="row">'
          +  '<div class="dt">'+f.date+'</div>'
          +  '<div class="cd"><span class="ico">'+(ICONS[f.code]||"🌤️")+'</span><span>'+f.day+'</span></div>'
          +  '<div class="mt">'+f.wind+' '+f.scale+'级 · 湿度 '+f.humidity+'%</div>'
          +  '<div class="tp"><span class="hi">'+f.high+'°</span><span class="lo">'+f.low+'°</span></div>'
          +'</div>';
    });
  }
  $("forecast").innerHTML=html;

  $("footer").textContent="最后更新 "+d.date+" "+d.time+"　·　共 "+d.forecastCount+" 天预报";
}

function fetchData(){
  fetch("/data").then(function(r){return r.json();}).then(render).catch(function(){
    $("footer").textContent="无法连接设备，正在重试…";
  });
}

fetchData();
setInterval(fetchData,5000);
setInterval(tickClock,1000);
setInterval(tickUptime,1000);
</script>
</body>
</html>
)rawliteral";

void handleNtpRequest() {
  int cb = ntpUDP.parsePacket();
  if (!cb) return;

  IPAddress remoteIP = ntpUDP.remoteIP();
  uint16_t remotePort = ntpUDP.remotePort();

  uint8_t packetBuffer[48];
  ntpUDP.read(packetBuffer, 48);

  uint8_t response[48];
  memcpy(response, NTP_PACKET_BASE, 48);
  response[0] = 0x1C;
  response[1] = 0x01;
  response[2] = 0x04;
  response[3] = 0x00;

  struct tm timeinfo;
  time_t now = 0;
  if (getLocalTime(&timeinfo, 100)) {
    now = mktime(&timeinfo);
  } else {
    now = time(NULL);
  }

  unsigned long secsSince1900 = (unsigned long)now + 2208988800UL;

  response[40] = (secsSince1900 >> 24) & 0xFF;
  response[41] = (secsSince1900 >> 16) & 0xFF;
  response[42] = (secsSince1900 >> 8)  & 0xFF;
  response[43] =  secsSince1900        & 0xFF;

  unsigned long frac = 0;
  response[44] = (frac >> 24) & 0xFF;
  response[45] = (frac >> 16) & 0xFF;
  response[46] = (frac >> 8)  & 0xFF;
  response[47] =  frac        & 0xFF;

  memcpy(&response[32], &packetBuffer[40], 4);

  ntpUDP.beginPacket(remoteIP, remotePort);
  ntpUDP.write(response, 48);
  ntpUDP.endPacket();
}

void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[天气] WiFi 未连接");
    return;
  }

  HTTPClient http;
  String url = String("https://api.seniverse.com/v3/weather/daily.json?key=") +
               apiKey + "&location=" + city + "&language=zh-Hans&unit=c&start=0&days=5";
  http.begin(url);
  http.setTimeout(8000);
  int httpCode = http.GET();

  Serial.printf("[天气] HTTP %d\n", httpCode);

  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      JsonArray daily = doc["results"][0]["daily"];
      forecastCount = daily.size();
      for (int i = 0; i < forecastCount && i < 5; i++) {
        forecastDate[i]      = daily[i]["date"].as<String>();
        forecastDayText[i]   = daily[i]["text_day"].as<String>();
        forecastNightText[i] = daily[i]["text_night"].as<String>();
        forecastCode[i]      = daily[i]["code"].as<String>();
        forecastHigh[i]      = daily[i]["high"].as<int>();
        forecastLow[i]       = daily[i]["low"].as<int>();
        forecastWindDir[i]   = daily[i]["wind_direction"].as<String>();
        forecastWindScale[i] = daily[i]["wind_scale"].as<int>();
        forecastHumidity[i]  = daily[i]["humidity"].as<int>();
        forecastRainfall[i]  = daily[i]["rainfall"].as<float>();
      }
      Serial.printf("[天气] 获取成功, %d 天\n", forecastCount);
    } else {
      Serial.printf("[天气] JSON 解析失败: %s\n", error.c_str());
    }
  } else {
    Serial.printf("[天气] 请求失败: %d\n", httpCode);
  }
  http.end();
}

void updateTime() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 100)) {
    if (!ntpSynced) {
      ntpSynced = true;
      lastNtpSync = millis();
      Serial.println("[NTP] 首次同步成功");
    }
    char timeBuf[10];
    char dateBuf[12];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &timeinfo);
    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", &timeinfo);
    ntpTime = String(timeBuf);
    ntpDate = String(dateBuf);
    int wd = timeinfo.tm_wday;
    const char* wdNames[] = {"日","一","二","三","四","五","六"};
    ntpWeekday = String(wdNames[wd]);
  }
}

String getUptime() {
  unsigned long sec = (millis() - bootMillis) / 1000;
  unsigned long h = sec / 3600;
  unsigned long m = (sec % 3600) / 60;
  unsigned long s = sec % 60;
  char buf[12];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h, m, s);
  return String(buf);
}

void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handleData() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    temperature = t;
    humidity = h;
  }

  updateTime();

  if (millis() - lastWeatherFetch > weatherInterval || lastWeatherFetch == 0 || forecastCount == 0) {
    fetchWeather();
    lastWeatherFetch = millis();
  }

  unsigned long uptimeSec = (millis() - bootMillis) / 1000;

  DynamicJsonDocument doc(2048);
  doc["indoor"]["temp"]  = isnan(temperature) ? "--" : String(temperature, 1);
  doc["indoor"]["humid"] = isnan(humidity) ? "--" : String(humidity, 1);
  doc["time"]          = ntpTime;
  doc["date"]          = ntpDate;
  doc["weekday"]       = ntpWeekday;
  doc["timezone"]      = "UTC+8";
  doc["uptime"]        = getUptime();
  doc["uptimeSec"]     = uptimeSec;
  doc["ntpSynced"]     = ntpSynced;
  doc["wifiConnected"] = (WiFi.status() == WL_CONNECTED);
  doc["wifiSSID"]      = ssid;
  doc["ip"]            = WiFi.localIP().toString();
  doc["forecastCount"] = forecastCount;

  JsonArray arr = doc.createNestedArray("forecast");
  for (int i = 0; i < forecastCount && i < 5; i++) {
    JsonObject item = arr.createNestedObject();
    item["date"]     = forecastDate[i].substring(5);
    item["day"]      = forecastDayText[i];
    item["night"]    = forecastNightText[i];
    item["high"]     = String(forecastHigh[i]);
    item["low"]      = String(forecastLow[i]);
    item["wind"]     = forecastWindDir[i];
    item["scale"]    = String(forecastWindScale[i]);
    item["humidity"] = String(forecastHumidity[i]);
    item["rain"]     = String(forecastRainfall[i], 1);
    item["code"]     = forecastCode[i];
  }

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  bootMillis = millis();

  WiFi.begin(ssid, password);
  Serial.print("连接 WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP 地址: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  ntpUDP.begin(123);
  Serial.println("NTP 服务器已启动 (UDP 123)");

  fetchWeather();
  lastWeatherFetch = millis();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Web 服务器已启动");
}

void loop() {
  server.handleClient();
  handleNtpRequest();
}