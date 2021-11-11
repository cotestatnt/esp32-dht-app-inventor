/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


// Definite dal protocollo GATT/BLE
const uint16_t temperatureUUID  = 0x2A6E;
const uint16_t humidityUUID     = 0x2A6F;
const uint16_t environmentUUID  = 0x181A;
const uint16_t descriptionUUID   = 0x2901;


// Variabili che saranno esposte dal servizio BLE
float temperature = -15.0F;
float humidity = 45.8F;

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

  // notify changed value
  if (deviceConnected) {

    // Aggiona valori di temperatura e umidità se il client è connesso (es. smartphone)
    // temperature = dht.getTemperature();
    // humidity = dht.getHumidity();

    /* Temperature characteristic
      Unit is in degrees Celsius with a resolution of 0.01 degrees Celsius
      Exponent: Decimal, -2
    */
    uint16_t charValue;
    charValue = (int16_t)(temperature *100); // multiply by 100 to get 2 digits mantissa and convert into uint16_t
    temperatureCharact.setValue(charValue);  // set characteristic value
    temperatureCharact.notify();

    // Stessa cosa per umidità
    charValue = (int16_t)(humidity *100); // multiply by 100 to get 2 digits mantissa and convert into uint16_t
    humidityCharact.setValue(charValue);  // set characteristic value
    humidityCharact.notify();

    delay(5); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
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
