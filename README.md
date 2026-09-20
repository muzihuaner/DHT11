# ESP32 DHT11 温湿度监控 + 天气预报

基于 ESP32 的室内温湿度监控系统，DHT11 采集室内数据，心知天气 5 日预报 + NTP 网络时间，网页仪表盘展示。

## 功能特性

- 实时读取 DHT11 温湿度数据
- 心知天气 5 日天气预报（白天/夜间天气、高低温、风向风级、湿度、降雨量）
- NTP 网络校准时间，精确显示
- 网页仪表盘，每 5 秒自动刷新
- 深色渐变 UI，响应式布局适配手机/电脑

## 硬件接线

| DHT11 引脚 | ESP32 引脚 |
|-----------|-----------|
| VCC       | 3.3V      |
| DATA      | GPIO4     |
| GND       | GND       |

> DATA 引脚建议接一个 4.7K~10K 上拉电阻到 3.3V（部分模块板载已集成）

## 依赖库安装

在 Arduino IDE 中打开 **工具 → 管理库**，搜索并安装：

| 库名                  | 作者              | 用途       |
|---------------------|-----------------|----------|
| DHT sensor library  | Adafruit        | DHT11 驱动 |
| Adafruit Unified Sensor | Adafruit    | 传感器抽象层 |
| ArduinoJson         | Benoit Blanchon | JSON 解析  |

## 项目配置

打开 `DHT11_Weather.ino`，修改以下变量：

```cpp
const char* ssid     = "YOUR_WIFI_SSID";   // WiFi 名称
const char* password = "YOUR_WIFI_PASSWORD"; // WiFi 密码
const char* apiKey   = "Sd8_LszRLgx94kS2m"; // 心知天气密钥
const char* city     = "datong";             // 城市（拼音/ID）
```

## 获取心知天气 API Key

1. 访问 https://seniverse.com 注册账号
2. 登录后进入控制台，复制你的 API Key（私钥）
3. 免费版支持实时天气 + 5 日预报

## NTP 时间

使用阿里云 NTP 服务器 `ntp.aliyun.com`，自动同步北京时间 (UTC+8)，无需额外配置。

## 上传烧录

1. Arduino IDE 安装 ESP32 开发板支持
2. **工具 → 开发板** 选择 `ESP32 Dev Module`
3. **工具 → 端口** 选择对应 COM 口
4. 点击上传，完成后打开串口监视器（115200 波特率）
5. 查看输出的 IP 地址，浏览器访问即可

## 网页展示内容

| 区域   | 内容                         |
|------|----------------------------|
| 顶部   | NTP 网络时间                   |
| 室内卡片 | 温度、湿度、当前日期               |
| 预报区域 | 未来 5 天：白天/夜间天气、高低温、风向风级、湿度 |

## 文件结构

```
DHT11_Weather/
├── DHT11_Weather.ino   # 主程序
└── README.md           # 项目说明
```

## License

MIT
