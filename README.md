## This Fork
![image](https://github.com/joaoasilva/KickrBikeDisplay/assets/3317453/100b2ff8-7dc2-43d6-ba9f-c584ac252bbc)

- Pressing both buttons of the Lilygo device changes orientation so you can have the usb on the left or right
- Device can be paired as a Bluetooth Keyboard, it uses https://github.com/T-vK/ESP32-BLE-Keyboard

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
