/************************************************************************************
* Wahoo Kickr Display 
* Joao Silva 02.01.2024
* Based on Claus Jensen project from 24.12.2023
****************************************************************************************/

#include "BLEDevice.h"
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <BleKeyboard.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

WiFiMulti wifiMulti;

BleKeyboard bleKeyboard("KICKR BIKE BTNS", "QWERTYnd", 100);

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
TFT_eSprite img = TFT_eSprite(&tft); // define a frame buffer

// RESOLUTION = 170 x 320 ... LILYGO T-Display-S3 ESP32-S3
#define RESOLUTION_X 320
#define RESOLUTION_Y 170

// LILYGO BUTTONS
#define BOTTOM_BUTTON_PIN 0
#define TOP_BUTTON_PIN 14

// KICKR BUTTONS
#define TOP_RIGHT_BUTTON KEY_MEDIA_NEXT_TRACK
#define BOTTOM_RIGHT_BUTTON KEY_MEDIA_PREVIOUS_TRACK
#define SIDE_RIGHT_BUTTON KEY_NUM_PERIOD
#define SIDE_LEFT_BUTTON KEY_MEDIA_MUTE

typedef enum 
{
  STATE_INIT,
  STATE_FOUND,
  STATE_CONNECTED,
  STATE_CONNECT_FAILED,
  STATE_DISCONNECTED,
  NOF_STATES
} STATE;

static STATE state = STATE_INIT;

/* BLE Services --------------------------------------------------------------*/
// Gearing
// BLE Service (UUID is case sensitive)
static BLEUUID serviceGearingUUID("a026ee0d-0a7d-4ab3-97fa-f1500f9feb8b");
static BLEUUID charGearingUUID("a026e03a-0a7d-4ab3-97fa-f1500f9feb8b");
static BLEUUID charButtonsUUID("a026e03c-0a7d-4ab3-97fa-f1500f9feb8b");
//for currentGradient
static BLEUUID serviceGradientUUID("a026ee0b-0a7d-4ab3-97fa-f1500f9feb8b");
static BLEUUID charGradientUUID("a026e037-0a7d-4ab3-97fa-f1500f9feb8b");
/* --------------------------------------------------------------------------*/

static BLERemoteCharacteristic *p_gearingCharacteristic;
static BLERemoteCharacteristic *p_buttonsCharacteristic;
static BLERemoteCharacteristic *p_gradientCharacteristic;

/* Blutooth device connection */
static BLEScan *p_bleScan;
static uint32_t scanCount = 0;
static BLEAdvertisedDevice* p_connectedDevice;
static String bleDeviceName = "";

/* Buttons */
static bool topButtonState = true;
static bool bottomButtonState = true;
static uint8_t screenRotation = 1;

/*** Gradient ***/
static bool tiltLock = true;
static int16_t currentGradient = 0;

/*** Gearing ***/
static uint8_t frontGear = 1;
static uint8_t rearGear = 3;
static uint8_t nofFrontGears = 2;
static uint8_t nofRearGears = 12;
static bool    gearingLimitReached = false;

/* Buttons */
static String buttonConnectionMessage = "";
static bool sideLeftPressing = false;
static bool sideRightPressing = false;
static bool topRightPressing = false;
static bool bottomRightPressing = false;

/* SLEEP MODE */
static long lastUsedTime = millis();
static long sleepTimeOut = 1000 * 60 * 30; // Sleep after 30 minutes if no buttons pressed or gradient not changed

static void updateButtonMessage(void) {
  img.setTextColor(TFT_ORANGE, TFT_BLACK);
  img.drawString(buttonConnectionMessage, RESOLUTION_X/2, RESOLUTION_Y/2 - 15, 2);
  img.pushSprite(0,0);
}

static String httpToken = <Secret token>;
const char *AP_SSID = <SSID>;
const char *AP_PWD = <PASSWORD>;
const char *SERVER_NAME = <HA Binary Sensor url>;

/*************************************************************
 * Data received functions...
 ************************************************************/
static void gradientReceived(uint8_t* p_data, size_t nofBytes) 
{
  if(nofBytes == 3 && p_data[0] == 0xfd && p_data[1] == 0x33) 
  {
    tiltLock = (p_data[2] == 0x01);
  } 
  else if(nofBytes == 4 && p_data[0] == 0xfd && p_data[1] == 0x34) 
  {
    uint16_t temp;

    temp  = p_data[3];
    temp  = temp << 8;
    temp += p_data[2];
    currentGradient = (int16_t)temp;
  }
  updateInteraction();
}

static void updateInteraction(void) {
  sendToHA(true);
  lastUsedTime = millis();
}

static void buttonsReceived(uint8_t* p_data, size_t nofBytes)
{
  if (bleKeyboard.isConnected()) 
  {
    // Top right
    if (p_data[0] == 0x00 && p_data[1] == 0x01) {
      if (topRightPressing) {
        bleKeyboard.release(TOP_RIGHT_BUTTON);
        topRightPressing = false;
        buttonConnectionMessage = "";
      } else {
        bleKeyboard.press(TOP_RIGHT_BUTTON);
        topRightPressing = true;
        buttonConnectionMessage = "Top Right Pressed";
      }
      updateInteraction();
    }
    // Bottom right
    if (p_data[0] == 0x80 && p_data[1] == 0x00) {
      if (bottomRightPressing) {
        bleKeyboard.release(BOTTOM_RIGHT_BUTTON);
        bottomRightPressing = false;
        buttonConnectionMessage = "";
      } else {
        bleKeyboard.press(BOTTOM_RIGHT_BUTTON);
        bottomRightPressing = true;
        buttonConnectionMessage = "Bottom Right Pressed";
      }
      updateInteraction();
    }
    // Side right
    if (p_data[0] == 0x00 && p_data[1] == 0x08) {
      if (sideRightPressing) {
        bleKeyboard.release(SIDE_RIGHT_BUTTON);
        sideRightPressing = false;
        buttonConnectionMessage = "";
      } else {
        bleKeyboard.press(SIDE_RIGHT_BUTTON);
        sideRightPressing = true;
        buttonConnectionMessage = "Side Right Pressed";
      }
      updateInteraction();
    }
    // Side left
    if (p_data[0] == 0x20 && p_data[1] == 0x00) {
      if (sideLeftPressing) {
        bleKeyboard.release(SIDE_LEFT_BUTTON);
        sideLeftPressing = false;
        buttonConnectionMessage = "";
      } else {
        bleKeyboard.press(SIDE_LEFT_BUTTON);
        sideLeftPressing = true;
        buttonConnectionMessage = "Side Left Pressed";
      }
      updateInteraction();
    }
  }
  else
  {
    buttonConnectionMessage = "Buttons not paired";
  }
}

static void gearingReceived(uint8_t* p_data, size_t nofBytes) 
{
  // Gears
  if(nofBytes >= 5)
  { 
    frontGear = p_data[2] + 1;
    rearGear  = p_data[3] + 1;
    if(nofBytes >= 6) // Make sure data is here before using it.
    { 
      nofFrontGears = p_data[4];
      nofRearGears  = p_data[5];
    }
    // Decided to only base red gears on the cassette for now...
    /*if (((frontGear == 1) && (rearGear == 1)) || 
        ((frontGear == nofFrontGears) && (rearGear == nofRearGears))) {*/
    if ((rearGear == 1) || (rearGear == nofRearGears)) 
    {
      gearingLimitReached = true;
    } 
    else 
    {
      gearingLimitReached = false;
    }
  }
}

/*************************************************************
 * Display update functions...
 ************************************************************/
static uint32_t getGradientColor(void) 
{
  uint16_t red = 0, green = 0, blue = 0;

  if(currentGradient < 0) 
  {
      blue = 31;
  } 
  else 
  {
    uint16_t absGradient = abs(currentGradient);

    absGradient = absGradient / 10;
    if(absGradient < 30) 
    {
      green = 34 + absGradient;
    } 
    else if(absGradient < 60) 
    {
      red = ((absGradient - 30) + 1);
      green = 63;
    } 
    else if(absGradient < 100) 
    {
      red = 31;
      green = (63 - (((absGradient - 60) * 16) / 10));
    } 
    else if(absGradient < 130) 
    {
      red = (31 - ((absGradient - 100) / 2));
    }
    else 
    {
      red = 16;
    }
  }
  return((uint32_t)((red << (5 + 6)) | (green << 5) | blue));
}

static void updateGradientTriangle(void)
{
  if((currentGradient / 10) == 0) 
  {
    // Flat, we draw a grey road...
    img.fillRect((RESOLUTION_X / 2), ((RESOLUTION_Y - 1) - 5), (RESOLUTION_X / 2), 5, TFT_DARKGREY);
  } 
  else 
  {
    int32_t x1, y1, x2, y2, x3, y3;
    uint32_t gradientColor;

    x1 = (RESOLUTION_X / 2);
    y1 = ((RESOLUTION_Y - 1) - 5);

    if(currentGradient > 0)
      x2 = (RESOLUTION_X - 1);
    else
      x2 = (RESOLUTION_X / 2);

    y2 = ((RESOLUTION_Y - 1) - 5) - (abs(currentGradient) / 10);
    if(y2 < 50)
      y2 = 50;

    x3 = (RESOLUTION_X - 1);
    y3 = ((RESOLUTION_Y - 1) - 5);

    gradientColor = getGradientColor();

    img.fillRect((RESOLUTION_X / 2), ((RESOLUTION_Y - 1) - 5), (RESOLUTION_X / 2), 5, gradientColor);
    img.fillTriangle(x1, y1, x2, y2, x3, y3, gradientColor);
  }
}

static void updateGradient(void) 
{
  int16_t absGradient;
  uint8_t spacesInFront = 0;
  String str_prefix;
  String str_currentGradient;
  
  if(currentGradient < 0) 
  {
    absGradient = (abs(currentGradient) / 10);
    str_prefix = "-";
    if(absGradient <= 99) 
      spacesInFront = 1;
  } 
  else 
  {
    absGradient = (currentGradient / 10);
    str_prefix = "";
    if(absGradient > 99)
      spacesInFront = 1;
    else 
      spacesInFront = 2;
  }
  str_currentGradient = str_prefix + String(absGradient / 10) + "." + String(absGradient % 10);

  img.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  img.drawString(str_currentGradient, int(RESOLUTION_X/2 + (spacesInFront * 40)), 0, 7);
  if(tiltLock == true)
  {
    img.setTextColor(TFT_RED, TFT_BLACK);
    img.drawString("% Gradient (locked)", int(RESOLUTION_X/2 + 20), 52, 2);
  } 
  else 
  {
    img.setTextColor(TFT_SKYBLUE, TFT_BLACK);
    img.drawString("% Gradient", int(RESOLUTION_X/2 + 80), 52, 2);
  }
  updateGradientTriangle();
}

#define chainringXcoord(chainring) (10 + ((chainring - 1) * 7))
#define chainringYcoord(chainring) (80 - ((chainring - 1) * 20))
#define chainringWidth(chainring)  (5)
#define chainringHeight(chainring) (60 + ((chainring - 1) * 40))

#define sprocketXcoord(sprocket) (40 + ((sprocket - 1) * 6))
#define sprocketYcoord(sprocket) (60 + ((sprocket - 1) * 3))
#define sprocketWidth(sprocket)  (5)
#define sprocketHeight(sprocket) (100 - ((sprocket - 1) * 6))

static void updateGear(void) 
{
  uint8_t count;
  String gearRatio = String(frontGear) + ":" + String(rearGear);

  // Gear ratio string at the top - RED if gearing limit reached.
  if(gearingLimitReached == true) 
    img.setTextColor(TFT_RED, TFT_BLACK);
  else 
    img.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  img.drawString(gearRatio, 0, 0, 7);  

  // Front chainrings
  //TODO: Handle 1 to 3 chainrings and always draw the biggest ones, so skip 1 or 2...
  for(count = 1; count <= nofFrontGears ; count++) 
  {
    if(count != frontGear) 
      img.drawRect(chainringXcoord(count), chainringYcoord(count), chainringWidth(count), chainringHeight(count), TFT_WHITE);
    else
      img.fillRect(chainringXcoord(count), chainringYcoord(count), chainringWidth(count), chainringHeight(count), (gearingLimitReached == true) ? TFT_RED : TFT_GREEN);
  }

  // Rear cassette
  for(count = 1; count <= nofRearGears ; count++) 
  {
    if(count != rearGear) 
      img.drawRect(sprocketXcoord(count), sprocketYcoord(count), sprocketWidth(count), sprocketHeight(count), TFT_WHITE);
    else
      img.fillRect(sprocketXcoord(count), sprocketYcoord(count), sprocketWidth(count), sprocketHeight(count), (gearingLimitReached == true) ? TFT_RED : TFT_GREEN);
  }
}

static void updateDisplay(void) 
{
  static STATE previousState = NOF_STATES;
  static String buttonString;

  if((bottomButtonState == false) && (topButtonState == false)) {
      // Inverting the display
      if (screenRotation == 1) {
        screenRotation = 3;
      }
      else 
      {
        screenRotation = 1;
      }
      tft.setRotation(screenRotation);
      buttonString = String(screenRotation);
  }
  else if (bottomButtonState == false)
  {
    sendToHA(false);
    buttonString = "HA Turn off will sleep in 2 seconds";
    delay(2000);
    // Power off the ESP32
    esp_deep_sleep_start();
  }
  else if (topButtonState == false)
  {
    sendToHA(true);
    buttonString = "HA Turn on ";
  }
  else 
    buttonString = "";

  // We clear everything every time...
  img.fillRect(0, 0, RESOLUTION_X, RESOLUTION_Y, TFT_BLACK);
  
  // And put our discrete debug-info for buttons in the corner...
  img.setTextColor(TFT_ORANGE, TFT_BLACK);
  img.drawString(buttonString, 65, RESOLUTION_Y - 16, 2);

  if(state == STATE_CONNECTED) 
  {
    // Connected - just update all values
    updateGear();
    updateGradient();
    updateButtonMessage();
  } 
  else 
  {
    String stateString;

    switch(state) 
    {
      case STATE_INIT: stateString += "Scanning for device..."; break;
      case STATE_FOUND: stateString += "Connecting to device:"; break;
      case STATE_CONNECTED: stateString += "Connected to device:"; break;
      case STATE_CONNECT_FAILED: stateString += "Connection failed, rescanning..."; break;
      case STATE_DISCONNECTED: stateString += "Disconnected, rescanning... "; break;
    }
    img.setTextColor(TFT_SKYBLUE, TFT_BLACK);
    img.drawString("Wahoo Kickr Display", 0, 0, 4);
    img.drawString(stateString, 0, 52, 4);

    if((state != STATE_CONNECTED) && (state != STATE_FOUND))
    {
      img.drawString("Scan Count: " + String(scanCount), 0, 78, 4);
    }
    else
    {
      img.drawString(bleDeviceName, 0, 78, 4);
    }

    if(wifiMulti.run() == WL_CONNECTED)
    {
      img.drawString("Wifi connected", 0, 108, 4);
    }
    else
    {
      img.drawString("Wifi disconnected", 0, 108, 4);
    }
  }
  
  img.pushSprite(0,0);
}

/***************************************************************************************************************************
  Class for bluetooth connection callback functions
***************************************************************************************************************************/
class MyClientCallback : public BLEClientCallbacks 
{
  void onConnect(BLEClient* pclient) 
  {
  }

  void onDisconnect(BLEClient* pclient) 
  {
    state = STATE_DISCONNECTED;
  }
};

/***************************************************************************************************************************
  Class for bluetooth scanning / discovery functions
***************************************************************************************************************************/
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks 
{
  // Called for each advertising BLE server.
  void onResult(BLEAdvertisedDevice advertisedDevice) 
  {
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceGearingUUID)) 
    {
      // Server advertises something we can use, stop scanning and let the main loop handle a connection...
      p_bleScan->stop();
      bleDeviceName = advertisedDevice.getName().c_str();
      p_connectedDevice = new BLEAdvertisedDevice(advertisedDevice);
      state = STATE_FOUND;
    } 
  } 
}; 

/*************************************************************
  Bluetooth functions
*************************************************************/
static bool connectToDevice(void) 
{
  BLERemoteService *p_remoteService;
  BLEClient *p_client;

  p_client = BLEDevice::createClient();
  p_client->setClientCallbacks(new MyClientCallback());
  p_client->connect(p_connectedDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)

  // Obtain a reference to the gearubg service in the remote BLE server.
  p_remoteService = p_client->getService(serviceGearingUUID);
  if (p_remoteService == nullptr) 
  {
    p_client->disconnect();
    return false;
  }
  // Obtain a reference to the characteristic in the service of the remote BLE server.
  p_gearingCharacteristic = p_remoteService->getCharacteristic(charGearingUUID);
  if (p_gearingCharacteristic == nullptr) 
  {
    p_client->disconnect();
    return false;
  }
  // Register notify-function
  if(p_gearingCharacteristic->canNotify())
  {
    p_gearingCharacteristic->registerForNotify(notifyCallbackGearing);
  }

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  p_buttonsCharacteristic = p_remoteService->getCharacteristic(charButtonsUUID);
  if (p_buttonsCharacteristic == nullptr) 
  {
    p_client->disconnect();
    return false;
  }
  // Register notify-function
  if(p_buttonsCharacteristic->canNotify())
  {
    p_buttonsCharacteristic->registerForNotify(notifyCallbackButtons);
  }

  // Obtain a reference to the gradient servicein the remote BLE server.
  p_remoteService = p_client->getService(serviceGradientUUID);
  if (p_remoteService == nullptr) 
  {
    p_client->disconnect();
    return(false);
  }
  // Obtain a reference to the characteristic in the service of the remote BLE server.
  p_gradientCharacteristic = p_remoteService->getCharacteristic(charGradientUUID);
  if (p_gradientCharacteristic == nullptr) 
  {
    p_client->disconnect();
    return(false);
  }
  // Register notify-function
  if(p_gradientCharacteristic->canNotify())
   {
    p_gradientCharacteristic->registerForNotify(notifyCallbackGradient);       
  }

  // Read the gearing values
  if(p_gearingCharacteristic->canRead()) 
  {
    uint8_t rxDataArray[32];
    std::string value = p_gearingCharacteristic->readValue();
    std::copy(value.begin(), value.end(), std::begin(rxDataArray));
    gearingReceived(rxDataArray, value.size());
  }
  // Read the buttons values
  if(p_buttonsCharacteristic->canRead()) 
  {
    uint8_t rxDataArray[32];
    std::string value = p_buttonsCharacteristic->readValue();
    std::copy(value.begin(), value.end(), std::begin(rxDataArray));
    buttonsReceived(rxDataArray, value.size());
  }
  // Read the gradient values
  if(p_gradientCharacteristic->canRead()) 
  {
    uint8_t rxDataArray[32];
    std::string value = p_gradientCharacteristic->readValue();
    std::copy(value.begin(), value.end(), std::begin(rxDataArray));
    gradientReceived(rxDataArray, value.size());
  }
  return(true);
}

static void notifyCallbackGradient(BLERemoteCharacteristic *p_BLERemoteCharacteristic2, uint8_t* p_data, size_t nofBytes, bool isNotify) 
{
  gradientReceived(p_data, nofBytes);
}

static void notifyCallbackGearing(BLERemoteCharacteristic *p_BLERemoteCharacteristic, uint8_t* p_data, size_t nofBytes, bool isNotify) 
{
  gearingReceived(p_data, nofBytes);
}

static void notifyCallbackButtons(BLERemoteCharacteristic *p_BLERemoteCharacteristic, uint8_t* p_data, size_t nofBytes, bool isNotify) 
{
  buttonsReceived(p_data, nofBytes);
}

/***************************************************************
  House keeping functions... 
****************************************************************/
static void checkButtonState(void) 
{
  bool topButton;
  bool bottomButton;

  topButton = (digitalRead(TOP_BUTTON_PIN) == HIGH);
  bottomButton = (digitalRead(BOTTOM_BUTTON_PIN) == HIGH);
  if((topButton != topButtonState) || (bottomButton != bottomButtonState))
  {
    delay(50); // Debounce Time
    if(topButton != topButtonState) 
    {
      topButton = (digitalRead(TOP_BUTTON_PIN) == HIGH);
      if(topButton != topButtonState) 
        topButtonState = topButton;
    }
    if(bottomButton != bottomButtonState) 
    {
      bottomButton = (digitalRead(BOTTOM_BUTTON_PIN) == HIGH);
      if(bottomButton != bottomButtonState)
        bottomButtonState = bottomButton;
    }
  }
}

void setup(void) 
{
  bleKeyboard.begin();
  tft.init();
  tft.setRotation(screenRotation);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  img.createSprite(RESOLUTION_X, RESOLUTION_Y);
  updateDisplay();

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the scan to run for 5 seconds.
  p_bleScan = BLEDevice::getScan();
  p_bleScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  p_bleScan->setInterval(997);
  p_bleScan->setWindow(996);
  p_bleScan->setActiveScan(true);
  p_bleScan->start(11, false);
  updateDisplay();

  wifiMulti.addAP(AP_SSID, AP_PWD);
}

void loop(void) 
{
  // Check if the ESP32 has been used
  if (millis() - lastUsedTime > sleepTimeOut) {
    sendToHA(false);
    delay(1000);
    // Power off the ESP32
    esp_deep_sleep_start();
    return;
  }

  if(state == STATE_FOUND) 
  {
    updateDisplay();
    if(connectToDevice() == true) 
      state = STATE_CONNECTED;
    else
      state = STATE_CONNECT_FAILED;
  }
  if((state == STATE_CONNECT_FAILED) || (state == STATE_DISCONNECTED) || (state == STATE_INIT)) 
  {
    // Either not found, disconnection or connection failed...
    scanCount++;
    updateDisplay();
    p_bleScan->start(11, false);
  }
  checkButtonState();
  updateDisplay();
  delay(100); // Delay 100 milliseconds
}

void sendToHA(bool state){
  if (millis() - lastUsedTime > httpCallTime)
  {
    if(wifiMulti.run() == WL_CONNECTED)
    {
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      http.begin(String(SERVER_NAME));
      http.addHeader("Content-Type", "application/json");
      http.setConnectTimeout(1000);
      Serial.println("Posting to HA");
      String bearer = "Bearer "+httpToken;
      http.addHeader("Authorization", bearer);

      StaticJsonDocument<200> data;
      if (state) {
        data["state"] = "on";
      } else {
        data["state"] = "off";
      }
      
      String requestBody;
      serializeJson(data, requestBody);
      int httpResponseCode  = http.POST(requestBody);
      httpCallTime = 1000 * 60; // Only do an http call every minute max

      http.end();
    }
  }
}
