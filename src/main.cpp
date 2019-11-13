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
#define ACC_ODR_25HZ			UINT8_C(0x06)
#define ACC_ODR_50HZ			UINT8_C(0x07)
#define ACC_ODR_100HZ			UINT8_C(0x08)
#define ACC_ODR_200HZ			UINT8_C(0x09)
#define ACC_ODR_400HZ			UINT8_C(0x0A)
#define ACC_ODR_800HZ			UINT8_C(0x0B)
#define ACC_ODR_1600HZ			UINT8_C(0x0C)
#define ACC_ODR_MASK			UINT8_C(0x0F)
#define ACC_US_MASK				UINT8_C(0x7F)
#define ACC_US_ENABLED			UINT8_C(0x80)
#define ACC_US_DISABLED			UINT8_C(0x00)
#define ACC_BW_NORMAL_MODE		UINT8_C(0x20)
#define ACC_BW_OSR2_MODE 		UINT8_C(0x10)
#define ACC_BW_OSR4_MODE		UINT8_C(0x00)
#define ACC_BW_MASK				UINT8_C(0x8F)
#define ACC_PWR_NORMAL_MODE 	UINT8_C(0x11)
#define ACC_RANGE_REG 			UINT8_C(0x41)
#define ACC_RANGE_2G 			UINT8_C(0x03)
#define ACC_RANGE_4G 			UINT8_C(0x05)
#define ACC_RANGE_8G 			UINT8_C(0x08)
#define ACC_RANGE_MASK			UINT8_C(0xF0)
#define ACC_PMU_STATUS_MASK		UINT8_C(0xCF)

/*####### GYRO CONF #######*/
#define GYRO_CONF_REG			UINT8_C(0x42)
#define GYRO_ODR_MASK			UINT8_C(0xF0)
#define GYRO_ODR_3200HZ			UINT8_C(0x0D)
#define GYRO_ODR_1600HZ			UINT8_C(0x0C)
#define GYRO_ODR_800HZ			UINT8_C(0x0B)
#define GYRO_ODR_400HZ			UINT8_C(0x0A)
#define GYRO_ODR_200HZ			UINT8_C(0x09)
#define GYRO_ODR_100HZ			UINT8_C(0x08)
#define GYRO_ODR_50HZ			UINT8_C(0x07)
#define GYRO_ODR_25HZ			UINT8_C(0x06)
#define GYRO_BW_MASK			UINT8_C(0xF0)
#define GYRO_BW_OSR4			UINT8_C(0x00)
#define GYRO_RANGE_REG			UINT8_C(0x43)
#define GYRO_RANGE_MASK			UINT8_C(0xF8)
#define GYRO_RANGE_2000			UINT8_C(0x00)
#define GYRO_RANGE_1000			UINT8_C(0x01)
#define GYRO_RANGE_500			UINT8_C(0x02)
#define GYRO_RANGE_250			UINT8_C(0x03)
#define GYRO_RANGE_125			UINT8_C(0x04)
#define GYRO_PWR_NORMAL_MODE 	UINT8_C(0x15)
#define GYRO_PMU_STATUS_MASK	UINT8_C(0xF3)

/*####### BMI160 REGISTERS & MASKS #######*/
#define RESET_MASK				UINT8_C(0xFF)
#define BMI160_CMD_REG			UINT8_C(0x7E)
#define PMU_STATUS_REG			UINT8_C(0x03)
#define ACC_Z_LOW_REG			UINT8_C(0x16)
#define ACC_Z_HIGH_REG 			UINT8_C(0x17)

#define INTERRUPT_CONF_BYTE 	UINT_C(0x50)

/*####### PROTOTYPES #######*/
void read_reg(uint8_t *data, uint8_t addr, uint8_t len);
void write_reg(uint8_t *data, uint8_t addr, uint8_t len);
void check_acc_range_conf(uint8_t *data);
void check_acc_us_conf(uint8_t * data);
void check_acc_bw_conf(uint8_t * data);
void check_acc_odr_conf(uint8_t * data);
void initialize_acc(uint8_t * data);
void check_gyro_odr_conf(uint8_t * data);
void check_gyro_bw_conf(uint8_t * data);
void check_gyro_range_conf(uint8_t * data);
void initialize_gyro(uint8_t * data);
void check_acc_pwr_mode(uint8_t * data);
void check_gyro_pwr_mode(uint8_t * data);
void initialize_power_mode(uint8_t * data);
void initialize_I2C();
void interrupt_test();

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

void check_acc_range_conf(uint8_t *data) {
	uint8_t acc_range_val = (data[1] & ~ACC_RANGE_MASK);
	if (acc_range_val < 0x03) {
		acc_range_val = ACC_RANGE_2G;
		write_reg(&acc_range_val, ACC_RANGE_REG, 1);
	} else return;
}

void check_acc_us_conf(uint8_t *data) {
	uint8_t acc_conf_val = (data[0] & ~ACC_US_MASK);
	if (acc_conf_val == ACC_US_ENABLED) {
		acc_conf_val = ACC_US_DISABLED;
		write_reg(&acc_conf_val, ACC_CONF_REG, 1);
	} else return;
}

void check_acc_bw_conf(uint8_t *data)
{
	uint8_t acc_conf_val = (data[0] & ~ACC_BW_MASK);
	if ((acc_conf_val == ACC_BW_NORMAL_MODE) || (acc_conf_val == ACC_BW_OSR2_MODE)) {
		acc_conf_val = ACC_BW_OSR4_MODE;
		write_reg(&acc_conf_val, ACC_CONF_REG, 1);
	} else return;
}

void check_acc_odr_conf(uint8_t *data) {
	uint8_t acc_conf_val = (data[0] & ~ACC_ODR_MASK);
	if (acc_conf_val < ACC_ODR_1600HZ) {
		acc_conf_val = ACC_ODR_1600HZ;
		write_reg(&acc_conf_val, ACC_CONF_REG, 1);
	} else return;
}

void check_gyro_odr_conf(uint8_t *data) {
	uint8_t gyro_odr_value = (data[0] & ~GYRO_ODR_MASK);
	if (gyro_odr_value != GYRO_ODR_3200HZ) {
		gyro_odr_value = GYRO_ODR_3200HZ;
		write_reg(&gyro_odr_value, GYRO_CONF_REG, 1);
	} else return;
}

void check_gyro_bw_conf(uint8_t *data) {
	uint8_t gyro_bw_value = (data[0] & ~GYRO_BW_MASK);
	if (gyro_bw_value != GYRO_BW_OSR4) {
		gyro_bw_value = GYRO_BW_OSR4;
		write_reg(&gyro_bw_value, GYRO_CONF_REG, 1);
	} else return;
}

void check_gyro_range_conf(uint8_t *data) {
	uint8_t gyro_range_value = (data[0] & ~GYRO_RANGE_MASK);
	if (gyro_range_value < 0x04) {
		gyro_range_value = GYRO_RANGE_125;
		write_reg(&gyro_range_value, GYRO_RANGE_REG, 1);
		delay(5);
	} else return;
}

void check_acc_pwr_mode(uint8_t *data) {
	uint8_t acc_pmu_status = (data[0] & ~ACC_PMU_STATUS_MASK);
	if (acc_pmu_status == 0x10) ACC_PWRMODE_NORMAL = true;
		else ACC_PWRMODE_NORMAL = false;
	if (ACC_PWRMODE_NORMAL == true) { 
		return;
	} else {
		acc_pmu_status = ACC_PWR_NORMAL_MODE;
		write_reg(&acc_pmu_status, BMI160_CMD_REG, 1);
		delay(7);
	}
	return;
}

void check_gyro_pwr_mode(uint8_t *data) {
	uint8_t gyro_pmu_status = (data[0] & ~GYRO_PMU_STATUS_MASK);
	if (gyro_pmu_status == 0x04) GYRO_PWRMODE_NORMAL = true; else GYRO_PWRMODE_NORMAL = false;
	if (GYRO_PWRMODE_NORMAL == true) {
		return;
	} else {
		gyro_pmu_status = GYRO_PWR_NORMAL_MODE;
		write_reg(&gyro_pmu_status, BMI160_CMD_REG, 1);
		delay(85);
	}
	return;
}

void initialize_gyro(uint8_t *data) {
	read_reg(data, GYRO_CONF_REG, 2);
	check_gyro_odr_conf(&data[0]);
	check_gyro_bw_conf(&data[0]);
	check_gyro_range_conf(&data[1]);
	read_reg(data, GYRO_CONF_REG, 2);
	return;
}

void initialize_acc(uint8_t *data) {
	read_reg(data, ACC_CONF_REG, 2);
	check_acc_us_conf(&data[0]);
	check_acc_bw_conf(&data[0]);
	check_acc_odr_conf(&data[0]);
	check_acc_range_conf(&data[1]);
	return;
}

void initialize_power_mode(uint8_t *data) {
	read_reg(data, PMU_STATUS_REG, 1);
	check_acc_pwr_mode(&data[0]);
	check_gyro_pwr_mode(&data[0]);
	return;
}

void initialize_I2C() {
	uint8_t data[2] = {0};
	Wire.begin(SDA, SCL, 2000000);
	initialize_power_mode(data);
	//initialize_acc(data);
	initialize_gyro(data);
}

void write_reg(uint8_t *data, uint8_t addr, uint8_t len) {
	if ((ACC_PWRMODE_NORMAL == true) || (GYRO_PWRMODE_NORMAL == true)) {
		Wire.beginTransmission(SLAVE_ADDR);
		Wire.write(addr);
		for (int i = 0; i < len; i++) {
			Wire.write(data[i]);
			delay(1);
		}
		Wire.endTransmission(true);
	} else {
		for (int i = 0; i < len; i++) {
			Wire.beginTransmission(SLAVE_ADDR);
			Wire.write(addr);
			Wire.write(data[i]);
			delay(1);
			Wire.endTransmission(true);
		}
	}
	return;
}

void read_reg(uint8_t *data, uint8_t addr, uint8_t len) {
	Wire.beginTransmission(SLAVE_ADDR);
	Wire.write(addr);
	Wire.endTransmission(true);
	delay(10);
	Wire.requestFrom(SLAVE_ADDR, len);
	for (int i = 0; i < len; i++) {
		data[i] = Wire.read();
		delay(1);
		if(data[i] < 0x10) {
			Serial.print("Data: 0x0");
		} else Serial.print("Data: 0x");
		Serial.print(data[i], HEX);
		if(addr < 0x10) {
			Serial.print(" @Adress: 0x0");
		} else Serial.print(" @Adress: 0x");
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
	//BLEDevice::init("ESP32"); //REENABLE

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
	}
}