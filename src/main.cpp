#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include <SPI.h>


/*BLE*/
BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 5;

/*SPI*/
static const int SPI_CLK = 1000000; 

SPIClass *vspi = NULL;
SPIClass *hspi = NULL;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define VSPICLK 18
#define VSPICS0 5
#define VSPIQ 19
#define VSPID 23
#define CHIPSEL 5
#define SPI_READ_INIT 0x7F
#define ACC_Z_LOW 0x16
#define ACC_Z_HIGH 0x17
#define READ_BYTE 0b00000001

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

uint8_t spi_test() {
	byte data = 0;
	for (size_t i = 0; i < 120; i++) {
		vspi->beginTransaction(SPISettings(SPI_CLK, MSBFIRST, SPI_MODE0));
		digitalWrite(CHIPSEL, LOW);
		//vspi->transfer(0x01); //READBIT
		data = vspi->transfer(0x96);
		Serial.println(data);
		digitalWrite(CHIPSEL, HIGH);
		vspi->endTransaction();
		delay(100);
	}
	return data;
}

void setup() {
	Serial.begin(115200);
	uint8_t acc_z_low = 0x16;
	uint8_t read = 0x01;
	uint8_t data[4] = {0};
	uint8_t data_to_send = 0;
	data_to_send = 0x16 | 0x01;
	digitalWrite(CHIPSEL, LOW);
	digitalWrite(CHIPSEL, HIGH);
	SPI.begin(VSPICLK, VSPIQ, VSPID, VSPIQ);
	pinMode(CHIPSEL, OUTPUT);
	SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));
	digitalWrite(CHIPSEL, LOW);
	data[0] = SPI.transfer(0x16 | 0x01);
	
	
	/*data[0] = SPI.transfer(0x16);
	data[1] = SPI.transfer(0x17);
	data[2] = SPI.transfer(0x18);
	data[3] = SPI.transfer(0x19);*/
	digitalWrite(CHIPSEL, HIGH);
	SPI.endTransaction();
	delay(5000);
	Serial.println(data[0]);
	Serial.println(data[1]);
	Serial.println(data[2]);
	Serial.println(data[3]);
	/*vspi->beginTransaction(SPISettings(SPI_CLK, MSBFIRST, SPI_MODE0));
	digitalWrite(5, LOW);
	vspi->write(SPI_READ_INIT);
	digitalWrite(5, HIGH);
	vspi->endTransaction();*/
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
	//spi_test();
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