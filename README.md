# Smart Pet Feeder - IoT Pet Feeding System

A smart, automated pet feeding system built with ESP32, featuring remote control through Blynk IoT platform and Telegram bot integration.

## Features

- **Remote Control**: Control feeding through Blynk mobile app
- **Voice Commands**: Telegram bot integration for voice/text commands
- **Weight Monitoring**: Real-time weight sensing of pet bowl and food container
- **Scheduled Feeding**: Set automatic feeding times
- **Manual Feeding**: Instant feeding with customizable portions
- **Data Analytics**: 
  - Weight history charts
  - Feeding history tracking
  - Daily feeding statistics
- **Smart Alerts**: Low food warnings and system status updates
- **Calibration**: Easy weight sensor calibration system

## Hardware Requirements

- **ESP32 Development Board**
- **HX711 Weight Sensor Module** (connected to pet bowl)
- **Servo Motor** (FS5115M or similar for food dispensing mechanism)
- **LED Indicator**
- **Food Container** with dispensing mechanism
- **Pet Bowl**

### Pin Connections

```
ESP32    |  Component
---------|-------------
Pin 5    |  Servo Motor (Signal)
Pin 4    |  HX711 (DOUT)
Pin 7    |  HX711 (SCK)
Pin 2    |  LED Indicator
VCC      |  VCC load cell 3.3V and VCC servo 5V
GND      |  Ground
```

## Software Dependencies

### Arduino Libraries Required:
- `WiFi.h` - ESP32 WiFi connectivity
- `WiFiClientSecure.h` - Secure connection for Telegram
- `BlynkSimpleEsp32.h` - Blynk IoT platform integration
- `ESP32Servo.h` - Servo motor control
- `HX711.h` - Weight sensor communication
- `TimeLib.h` - Time management
- `WidgetRTC.h` - Blynk real-time clock
- `UniversalTelegramBot.h` - Telegram bot integration

## Setup Instructions

### 1. Hardware Assembly
1. Connect the HX711 weight sensor under the pet bowl
2. Install the servo motor in the food dispensing mechanism
3. Connect all components according to the pin diagram above
4. Ensure stable power supply for ESP32

### 2. Software Configuration

#### 2.1 Install Required Libraries
In Arduino IDE, install all the libraries listed above through Library Manager.

#### 2.2 Configure Credentials
Create a `config.h` file and add your credentials:

```cpp
// WiFi Credentials
#define WIFI_SSID "your_wifi_name"
#define WIFI_PASSWORD "your_wifi_password"

// Blynk Configuration
#define BLYNK_TEMPLATE_ID "your_template_id"
#define BLYNK_TEMPLATE_NAME "your_template_name" 
#define BLYNK_AUTH_TOKEN "your_auth_token"

// Telegram Bot Configuration
#define BOT_TOKEN "your_bot_token"
#define CHAT_ID "your_chat_id"
```

#### 2.3 Blynk App Setup
1. Download Blynk IoT app
2. Create a new project
3. Add the following virtual pins:
   - **V1**: Button (Manual Feed)
   - **V2**: Display (Current Weight)
   - **V3**: Slider (Feed Amount, 10-100g)
   - **V4**: Terminal (Status Messages)
   - **V5**: Button (Calibration)
   - **V6**: Menu (Scheduled Feeding Time)
   - **V7**: Gauge (Food Remaining %)
   - **V9**: Chart (Weight History)
   - **V10**: Chart (Feed History) 
   - **V11**: Chart (Daily Feed Total)
   - **V12**: Button (Clear History)
   - **V13**: Button (Reset Container)

#### 2.4 Telegram Bot Setup
1. Message @BotFather on Telegram
2. Create a new bot with `/newbot`
3. Get your bot token
4. Send a message to your bot to get your chat ID

### 3. Upload and Test
1. Connect ESP32 to computer
2. Select correct board and port in Arduino IDE
3. Upload the code
4. Open Serial Monitor to check connection status
5. Test basic functions through Blynk app

## Usage Guide

### Basic Operations

1. **Calibration**: Press calibration button in Blynk app when bowl is empty
2. **Manual Feeding**: 
   - Set desired amount (10-100g) using slider
   - Press "Feed Now" button
3. **Scheduled Feeding**:
   - Select time from dropdown menu
   - System will automatically feed at set time
4. **Telegram Commands**:
   - Send "feed" to trigger feeding
   - Send "open" to open feeding gate
   - Send "close" to close feeding gate

### Monitoring

- **Weight Display**: Real-time weight of pet bowl
- **Food Remaining**: Shows percentage of food left in container
- **Charts**: Track feeding patterns and weight changes over time
- **Status Messages**: Real-time system updates and alerts

## Troubleshooting

### Common Issues:

1. **WiFi Connection Failed**
   - Check SSID and password
   - Ensure ESP32 is in range of router

2. **Blynk Connection Issues**
   - Verify auth token is correct
   - Check internet connection
   - Ensure virtual pins match app configuration

3. **Weight Sensor Inaccurate**
   - Recalibrate using calibration button
   - Check sensor connections
   - Ensure stable mounting

4. **Servo Not Moving**
   - Check power supply (servo may need 5V)
   - Verify pin connections
   - Test servo separately

5. **Telegram Bot Not Responding**
   - Verify bot token and chat ID
   - Check internet connection
   - Ensure bot is properly configured

## Safety Notes

- Ensure all electrical connections are secure
- Use appropriate power supply for your servo motor
- Test feeding mechanism thoroughly before leaving pet unattended
- Set reasonable feeding amounts to prevent overfeeding
- Regularly clean and maintain the feeding mechanism

## Contributing

Feel free to submit issues, feature requests, or pull requests to improve this project.

## License

This project is open source and available under the MIT License.

## Support

If you encounter any issues or have questions, please create an issue in this repository.
