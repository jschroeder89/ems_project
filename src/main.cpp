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


/*BMI160*/
bool ACC_PWRMODE_NORMAL = false;
bool GYRO_PWRMODE_NORMAL = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID			"6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX	"6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX	"6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define SDA						21 //PIN 32 new ESP32
#define SCL						22 //PIN 33 new ESP32
#define SLAVE_ADDR				UINT8_C(0x68)
#define INTERRUPT_PIN			35

/*####### ACC CONF #######*/
#define ACC_CONF_REG			UINT8_C(0x40)
#define ACC_ODR_RATE_25HZ		UINT8_C(0x06)
#define ACC_ODR_RATE_50HZ		UINT8_C(0x07)
#define ACC_ODR_RATE_100HZ		UINT8_C(0x08)
#define ACC_ODR_RATE_200HZ		UINT8_C(0x09)
#define ACC_ODR_RATE_400HZ		UINT8_C(0x0A)
#define ACC_ODR_RATE_800HZ		UINT8_C(0x0B)
#define ACC_ODR_RATE_1600HZ		UINT8_C(0x0C)
#define ACC_ODR_MASK			UINT8_C(0x0F)
#define ACC_BW_NORMAL_MODE		UINT8_C(0x02)
#define ACC_PWR_NORMAL_MODE 	UINT8_C(0x11)
#define ACC_RANGE_REG 			UINT8_C(0x41)
#define ACC_RANGE_2G 			UINT8_C(0x03)
#define ACC_RANGE_4G 			UINT8_C(0x05)
#define ACC_RANGE_8G 			UINT8_C(0x08)
#define ACC_PMU_STATUS_MASK		UINT8_C(0xCF)
/*####### GYRO CONF #######*/


#define INTERRUPT_CONF_BYTE 	0b01010000
#define BMI160_CMD_REG			UINT8_C(0x7E)
#define PMU_STATUS				UINT8_C(0x03)
#define ACC_Z_LOW 				UINT8_C(0x16)
#define ACC_Z_HIGH 				UINT8_C(0x17)

/*####### PROTOTYPES #######*/
void read_reg(uint8_t *data, uint8_t addr, uint8_t len);
void write_reg(uint8_t *data, uint8_t addr, uint8_t len);

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

	class MyCallbacks : public BLECharacteristicCallbacks
	{
		void onWrite(BLECharacteristic *pCharacteristic)
		{
			std::string rxValue = pCharacteristic->getValue();

			if (rxValue.length() > 0)
			{
				Serial.println("*********");
				Serial.print("Received Value: ");
				for (int i = 0; i < rxValue.length(); i++)
					Serial.print(rxValue[i]);

				Serial.println();
				Serial.println("*********");
			}
		}
	};

void check_acc_odr_conf(uint8_t * data) {

}

void check_acc_pwr_mode(uint8_t * data) {
		read_reg(data, PMU_STATUS, 1);
		uint8_t acc_pmu_status = (data[0] & ~ACC_PMU_STATUS_MASK);
		if (acc_pmu_status == 0x10) ACC_PWRMODE_NORMAL = true; else ACC_PWRMODE_NORMAL = false;
		if (ACC_PWRMODE_NORMAL == true) { 
			return;
		} else {
			data[0] = ACC_PWR_NORMAL_MODE;
			write_reg(&data[0], BMI160_CMD_REG, 1);
		}
		return;
}

void initialize_acc(uint8_t * data) {
		check_acc_pwr_mode(data);
		read_reg(data, ACC_CONF_REG, 2);
		check_acc_odr_conf(&data[0]);
}

void initialize_I2C() {
		uint8_t data[2] = {0};
		Wire.begin(SDA, SCL, 2000000);
		initialize_acc(data);
}

void write_reg(uint8_t * data, uint8_t addr, uint8_t len) {
	if ((ACC_PWRMODE_NORMAL == true) || (ACC_PWRMODE_NORMAL == true)) {
		Wire.beginTransmission(SLAVE_ADDR);
		Wire.write(addr);
		for (int i = 0; i < len; i++)
		{
			Wire.write(data[i]);
			Serial.print("data[i]: ");Serial.println(data[i]);
			delay(1);
		}
		Wire.endTransmission(true);
	}
	else
	{
		for (int i = 0; i < len; i++)
		{
			Wire.beginTransmission(SLAVE_ADDR);
			Wire.write(addr);
			Wire.write(data[i]);
			Serial.print("data[i]: ");Serial.println(data[i]);
			delay(1);
			Wire.endTransmission(true);
		}
	}
	return;
}

void read_reg(uint8_t * data, uint8_t addr, uint8_t len) {
	Wire.beginTransmission(SLAVE_ADDR);
	Wire.write(addr);
	Wire.endTransmission(true);
	delay(10);
	Wire.requestFrom(SLAVE_ADDR, len);
	for (int i = 0; i < len; i++)
	{
		data[i] = Wire.read();
		delay(1);
		Serial.print("Data: 0x");
		Serial.print(data[i], HEX);
		Serial.print(" @Adress: 0x");
		Serial.print(addr++, HEX);
		Serial.println();
	}
	return;
}

void interrupt_test() {
	Serial.println("INTERRUPT TRIGGERD");
}

void setup() {
	Serial.begin(115200);
	delay(5000);
	initialize_I2C();
	//attachInterrupt(INTERRUPT_PIN, interrupt_test, CHANGE);
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