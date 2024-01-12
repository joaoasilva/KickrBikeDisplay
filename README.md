## ESP32-S3 KICKR BIKE Buttons with bluetooth keyboard mappings and Home Assistant integration

![image](https://github.com/joaoasilva/KickrBikeDisplay/assets/3317453/100b2ff8-7dc2-43d6-ba9f-c584ac252bbc)


- Pressing both buttons of the Lilygo device changes orientation so you can have the usb on the left or right
- Device can be paired as a Bluetooth Keyboard, the name will appear as *KICKR BIKE BTNS* and it uses https://github.com/T-vK/ESP32-BLE-Keyboard

NOTE: I could only make the ESP32-BLE-Keyboard library work by changing the file *BleKeyboard.cpp* line 130 to:
```
pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND); 
```


To change the keyboard mappings change the following:
```
#define TOP_RIGHT_BUTTON KEY_MEDIA_MUTE
#define BOTTOM_RIGHT_BUTTON KEY_NUM_PERIOD
#define SIDE_RIGHT_BUTTON KEY_MEDIA_NEXT_TRACK
#define SIDE_LEFT_BUTTON KEY_MEDIA_PREVIOUS_TRACK
```

To change the sleep time out change:
```
static long sleepTimeOut = 1000 * 60 * 30;
```


If you use Home Assistant you can connect it to your wifi and update a binary sensor https://www.home-assistant.io/integrations/http/#sensor

Just add a token to your user on HA and use it for authentication and update the following fields:
```
static String httpToken = <Secret token>;
const char *AP_SSID = <SSID>;
const char *AP_PWD = <PASSWORD>;
const char *SERVER_NAME = <HA Binary Sensor url>;
```


There's also some 3d printable files

![image](https://github.com/joaoasilva/KickrBikeDisplay/assets/3317453/04ef9c6b-317f-4661-8ad2-4b2459a80c8d)
