#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include <Wire.h>
#include <BMI160.hpp>

/*BLE*/
BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 5;
const int ledPin = 33;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID			"6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX	"6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX	"6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define SDA						21 //PIN 32 new ESP32
#define SCL						22 //PIN 33 new ESP32
#define INTERRUPT_PIN			35

void interrupt_test();

BMI160 bmi160;

class MyServerCallbacks : public BLEServerCallbacks
{
	void onConnect(BLEServer *pServer)
	{
		deviceConnected = true;
	};

	void onDisconnect(BLEServer *pServer)
	{
		deviceConnected = false;
	}
};

class MyCallbacks : public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic *pCharacteristic) {
		std::string rxValue = pCharacteristic->getValue();
		if (rxValue.length() > 0) {
			Serial.println("*********");
			Serial.print("Received Value: ");
			for (int i = 0; i < rxValue.length(); i++)
				Serial.print(rxValue[i]);

			Serial.println();
			Serial.println("*********");
		}
	}
};


void setup() {
	Serial.begin(115200);
	delay(5000);
	pinMode(14, OUTPUT);
	bmi160.initialize_I2C();
	//attachInterrupt(INTERRUPT_PIN, interrupt_test, CHANGE);
	// Create the BLE Deqvice
	//BLEDevice::init("ESP32"); //REENABLE
	
	/*
	// Create the BLE Server
	pServer = BLEDevice::createServer();
	pServer->setCallbacks(new MyServerCallbacks());

	// Create the BLE Service
	BLEService *pService = pServer->createService(SERVICE_UUID);

	// Create a BLE Characteristic
	pTxCharacteristic = pService->createCharacteristic(
		CHARACTERISTIC_UUID_TX,
		BLECharacteristic::PROPERTY_NOTIFY);

	pTxCharacteristic->addDescriptor(new BLE2902());

	BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
		CHARACTERISTIC_UUID_RX,
		BLECharacteristic::PROPERTY_WRITE);

	pRxCharacteristic->setCallbacks(new MyCallbacks());

	// Start the service
	pService->start();

	// Start advertising
	pServer->getAdvertising()->addServiceUUID(pService->getUUID());
	pServer->getAdvertising()->start();
	Serial.println("Waiting a client connection to notify...");*/
}
uint8_t data = 0;
void loop() {
	bmi160.get_sensor_data();
	//bmi160.publish_sensor_data();
	if(digitalRead(14) == LOW) 
	{
		digitalWrite(14, HIGH);
	} else
		digitalWrite(14, LOW);
	//delay(1);
	
	//if(digitalRead(14) == HIGH) 
	//delay(1);
	//Serial.println("DATA START");
	//bmi160.get_sensor_data();
	//bmi160.publish_sensor_data();
	//delay(10);
	//Serial.println("DATA END");
	//digitalWrite(ledPin, HIGH);
	//if(digitalRead(ledPin) == HIGH)
	//{
	//} 
	//delay(500);
	//digitalWrite(ledPin, LOW);
	/*if (deviceConnected) {	
		pTxCharacteristic->setValue(&txValue, 1);
		pTxCharacteristic->notify();
		//txValue++;
		delay(10); // bluetooth stack will go into congestion, if too many packets are sent
		//Serial.println(txValue);
	}

	// disconnecting
	if (!deviceConnected && oldDeviceConnected) {
		delay(500);					 // give the bluetooth stack the chance to get things ready
		pServer->startAdvertising(); // restart advertising
		Serial.println("start advertising");
		oldDeviceConnected = deviceConnected;
	}
	// connecting
	if (deviceConnected && !oldDeviceConnected) {
		// do stuff here on connecting
		oldDeviceConnected = deviceConnected;
	}*/
}