/*
  Tolentino Cotesta - cotestatnt@yahoo.com
  https://github.com/cotestatnt/esp32-dht-app-inventor

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


#include "DHT.h"  // https://github.com/adafruit/DHT-sensor-library
#define DHTPIN 15         // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22     // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);

// Definite dal protocollo GATT/BLE
const uint16_t temperatureUUID  = 0x2A6E;
const uint16_t humidityUUID     = 0x2A6F;
const uint16_t environmentUUID  = 0x181A;
const uint16_t descriptionUUID   = 0x2901;


// Definizione del server BLE
BLEServer* pServer = NULL;

// Definizione delle caratteristiche per umidità e temepratura
BLECharacteristic temperatureCharact(
           BLEUUID(temperatureUUID),
           BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic humidityCharact(
           BLEUUID(humidityUUID),
           BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

bool deviceConnected = false;
bool oldDeviceConnected = false;


// Funzioni di callback che sarannoe eseguite quando un client si connette/disconnette
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};



void setup() {
  Serial.begin(115200);
  dht.begin();

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(BLEUUID(environmentUUID));

  // Aggiungi la descrizione alla caratteristica
  temperatureCharact.addDescriptor(new BLE2902());
  humidityCharact.addDescriptor(new BLE2902());

  // Aggiungi le caratteristiche su configurate al servizio ed avvia
  pService->addCharacteristic(&temperatureCharact);
  pService->addCharacteristic(&humidityCharact);
  pService->start();

  // Start advertising (facciamoci vedere dal resto del mondo)
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLEUUID(environmentUUID));
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();

  Serial.printf("\n\nBLE service UUID: %s\n", BLEUUID(environmentUUID).toString().c_str());
  Serial.printf("Temperature characteristic UUID: %s\n", BLEUUID(temperatureUUID).toString().c_str());
  Serial.printf("Humidity characteristic UUID: %s\n", BLEUUID(humidityUUID).toString().c_str());

  Serial.println("\nWaiting a client connection to notify...");
}

void loop() {


  // notify changed value every 1000 millisencods
  static uint32_t updateTime;
  if (deviceConnected && millis() - updateTime > 1000) {
    updateTime = millis();
    static int temperature, humidity;
    float val;

    // Aggiona valori di temperatura e umidità se il client è connesso (es. smartphone)
    val = dht.readTemperature();
    if(! isnan(val))
      temperature = (int)(val *100);     // multiply by 100 to get 2 digits mantissa and convert into int

    val = dht.readHumidity();
    if(! isnan(val))
      humidity = (int)(val *100);

    Serial.print("Temperature: ");
    Serial.print((float)temperature/100, 2);
    Serial.print("; Humidity: ");
    Serial.println((float)humidity/100, 2);

    /* Temperature characteristic
      Unit is in degrees Celsius with a resolution of 0.01 degrees Celsius. Exponent: Decimal, -2
    */
    temperatureCharact.setValue(temperature);  // set characteristic value
    temperatureCharact.notify();

    // Stessa cosa per umidità
    humidityCharact.setValue(humidity);  // set characteristic value
    humidityCharact.notify();
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }

  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}