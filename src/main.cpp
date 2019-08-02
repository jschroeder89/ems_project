#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include <Wire.h>

/*BLE*/
BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 5;

/*I2C*/
TwoWire i2c = TwoWire(0);

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define SDA 21 //PIN 32 new ESP32
#define SCL 22 //PIN 33 new ESP32
#define INTERRUPT_PIN 36
#define SLAVE_ADDR 0x69
#define ACC_Z_LOW 0x16
#define ACC_Z_HIGH 0x17


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

void writeToReg(uint8_t addr, uint8_t val) {
	uint8_t SLAVE_ADDR_W = SLAVE_ADDR << 1;
	i2c.beginTransmission(SLAVE_ADDR_W);
	i2c.write(addr);
	delay(2);
	i2c.write(val);
	delay(2);
	i2c.endTransmission();
}

uint8_t readFromReg(uint8_t addr, size_t num_bytes) {
	uint8_t SLAVE_ADDR_R = (SLAVE_ADDR << 1) | 0b00000001;
	uint8_t SLAVE_ADDR_W = SLAVE_ADDR << 1;
	i2c.beginTransmission(SLAVE_ADDR_W);
	i2c.write(addr);
	delay(2);
	i2c.endTransmission();
	delay(2);
	i2c.beginTransmission(SLAVE_ADDR_R);
	i2c.requestFrom(SLAVE_ADDR_R, num_bytes);
	uint8_t data = i2c.read();
	i2c.endTransmission();
	return data;
}

void testRead() {
	uint8_t byte1 = 0, byte2 = 0;
	i2c.beginTransmission(0x68);
	i2c.write(0x0C);
	delay(10);
	i2c.write(0x0D);
	i2c.endTransmission(false);
	i2c.requestFrom(0x68, 2);
	if (i2c.available()) {
		byte1 = i2c.read();
		byte2 = i2c.read();
	}
	Serial.print("Byte1: ");
	Serial.println(byte1);
	Serial.print("Byte2: ");
	Serial.println(byte2);
	delay(100);
}

void interrupt_test() {
	Serial.println("INTERRUPT TRIGGERD");
}

void testscan() {
	Serial.println("SCAN BEGIN");
	uint8_t cnt = 0;
	for(uint8_t i=0; i< 128; i++) {
		i2c.beginTransmission(i);
		uint8_t ec = i2c.endTransmission(true);
		if(ec == 0) {
			if(i < 16) Serial.print('0');
			Serial.print(i, HEX);
			cnt++;
		} else Serial.print("..");
		Serial.print(' ');
		if((i & 0x0f) == 0x0f) Serial.println();
	}
	Serial.println("SCAN COMPLETED");
}

void setup() {
	// Initialize Baud rate
	Serial.begin(115200);	
	// Initialize I2C
	i2c.begin(SDA, SCL, 400000);
	// Initialize Interrupt Pin
	attachInterrupt(INTERRUPT_PIN, interrupt_test, RISING);
	// Create the BLE Deqvice
	BLEDevice::init("ESP32");

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
	Serial.println("Waiting a client connection to notify...");
}

void loop() {
	//Serial.println(readFromReg(0x02, 2));

	if (deviceConnected) {	
		pTxCharacteristic->setValue(&txValue, 1);
		pTxCharacteristic->notify();
		//txValue++;
		delay(10); // bluetooth stack will go into congestion, if too many packets are sent
		Serial.println(txValue);
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
	}
}