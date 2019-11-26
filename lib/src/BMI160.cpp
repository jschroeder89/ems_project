#include "BMI160.hpp"
#include <Wire.h>

BMI160::BMI160()
{
    for (size_t i = 0; i < BMI160_DATA.size(); i++)
    {
        BMI160_DATA.add(0);
    }
}

BMI160::~BMI160()
{
    
}

void BMI160::check_acc_range_conf(uint8_t *data) {
    uint8_t acc_range_val = (data[1] & ~ACC_RANGE_MASK);
    if (acc_range_val < 0x03)
    {
        acc_range_val = ACC_RANGE_2G;
        write_reg(&acc_range_val, ACC_RANGE_REG, 1);
    }
    else
        return;
}

void BMI160::check_acc_us_conf(uint8_t *data)
{
    uint8_t acc_conf_val = (data[0] & ~ACC_US_MASK);
    if (acc_conf_val == ACC_US_ENABLED)
    {
        acc_conf_val = ACC_US_DISABLED;
        write_reg(&acc_conf_val, ACC_CONF_REG, 1);
    }
    else
        return;
}

void BMI160::check_acc_bw_conf(uint8_t *data)
{
    uint8_t acc_conf_val = (data[0] & ~ACC_BW_MASK);
    if ((acc_conf_val == ACC_BW_NORMAL_MODE) || (acc_conf_val == ACC_BW_OSR2_MODE))
    {
        acc_conf_val = ACC_BW_OSR4_MODE;
        write_reg(&acc_conf_val, ACC_CONF_REG, 1);
    }
    else
        return;
}

void BMI160::check_acc_odr_conf(uint8_t *data)
{
    uint8_t acc_conf_val = (data[0] & ~ACC_ODR_MASK);
    if (acc_conf_val < ACC_ODR_1600HZ)
    {
        acc_conf_val = ACC_ODR_1600HZ;
        write_reg(&acc_conf_val, ACC_CONF_REG, 1);
    }
    else
        return;
}

void BMI160::check_gyro_odr_conf(uint8_t *data)
{
    uint8_t gyro_odr_value = (data[0] & ~GYRO_ODR_MASK);
    if (gyro_odr_value != GYRO_ODR_3200HZ)
    {
        gyro_odr_value = GYRO_ODR_3200HZ;
        write_reg(&gyro_odr_value, GYRO_CONF_REG, 1);
    }
    else
        return;
}

void BMI160::check_gyro_bw_conf(uint8_t *data)
{
    uint8_t gyro_bw_value = (data[0] & ~GYRO_BW_MASK);
    if (gyro_bw_value != GYRO_BW_OSR4)
    {
        gyro_bw_value = GYRO_BW_OSR4;
        write_reg(&gyro_bw_value, GYRO_CONF_REG, 1);
    }
    else
        return;
}

void BMI160::check_gyro_range_conf(uint8_t *data)
{
    uint8_t gyro_range_value = (data[0] & ~GYRO_RANGE_MASK);
    if (gyro_range_value < 0x04)
    {
        gyro_range_value = GYRO_RANGE_125;
        write_reg(&gyro_range_value, GYRO_RANGE_REG, 1);
        delay(5);
    }
    else
        return;
}

void BMI160::check_acc_pwr_mode(uint8_t *data)
{
    uint8_t acc_pmu_status = (data[0] & ~ACC_PMU_STATUS_MASK);
    if (acc_pmu_status == 0x10)
        ACC_PWRMODE_NORMAL = true;
    else
        ACC_PWRMODE_NORMAL = false;
    if (ACC_PWRMODE_NORMAL == true)
    {
        return;
    }
    else
    {
        acc_pmu_status = ACC_PWR_NORMAL_MODE;
        write_reg(&acc_pmu_status, BMI160_CMD_REG, 1);
        delay(7);
    }
    return;
}

void BMI160::check_gyro_pwr_mode(uint8_t *data)
{
    uint8_t gyro_pmu_status = (data[0] & ~GYRO_PMU_STATUS_MASK);
    if (gyro_pmu_status == 0x04)
        GYRO_PWRMODE_NORMAL = true;
    else
        GYRO_PWRMODE_NORMAL = false;
    if (GYRO_PWRMODE_NORMAL == true)
    {
        return;
    }
    else
    {
        gyro_pmu_status = GYRO_PWR_NORMAL_MODE;
        write_reg(&gyro_pmu_status, BMI160_CMD_REG, 1);
        delay(85);
    }
    return;
}

void BMI160::initialize_gyro(uint8_t *data)
{
    read_reg(data, GYRO_CONF_REG, 2);
    check_gyro_odr_conf(&data[0]);
    check_gyro_bw_conf(&data[0]);
    check_gyro_range_conf(&data[1]);
    read_reg(data, GYRO_CONF_REG, 2);
    return;
}

void BMI160::initialize_acc(uint8_t *data)
{
    read_reg(data, ACC_CONF_REG, 2);
    check_acc_us_conf(&data[0]);
    check_acc_bw_conf(&data[0]);
    check_acc_odr_conf(&data[0]);
    check_acc_range_conf(&data[1]);
    return;
}

void BMI160::initialize_power_mode(uint8_t *data)
{
    read_reg(data, PMU_STATUS_REG, 1);
    check_acc_pwr_mode(&data[0]);
    check_gyro_pwr_mode(&data[0]);
    return;
}

void BMI160::initialize_I2C()
{
    uint8_t data[2] = {0};
    Wire.begin(SDA, SCL, 1000000);
    initialize_power_mode(data);
    initialize_acc(data);
    initialize_gyro(data);
}

void BMI160::write_reg(uint8_t *data, uint8_t addr, uint8_t len)
{
    if ((ACC_PWRMODE_NORMAL == true) || (GYRO_PWRMODE_NORMAL == true))
    {
        Wire.beginTransmission(SLAVE_ADDR);
        Wire.write(addr);
        for (int i = 0; i < len; i++)
        {
            Wire.write(data[i]);
            //delay(1);
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
            //delay(1);
            Wire.endTransmission(true);
        }
    }
    return;
}

void BMI160::read_reg(uint8_t *data, uint8_t addr, uint8_t len)
{
    Wire.beginTransmission(SLAVE_ADDR);
    Wire.write(addr);
    Wire.endTransmission(true);
    //delay(10);
    Wire.requestFrom(SLAVE_ADDR, len);
    for (int i = 0; i < len; i++)
    {
        data[i] = Wire.read();
        //delay(1);
        /*
        if (data[i] < 0x10)
        {
            Serial.print("Data: 0x0");
        }
        else
            Serial.print("Data: 0x");
        Serial.print(data[i], HEX);
        if (addr < 0x10)
        {
            Serial.print(" @Adress: 0x0");
        }
        else
            Serial.print(" @Adress: 0x");
        Serial.print(addr++, HEX);
        Serial.println();*/
    }
    return;
}

void BMI160::get_sensor_data()
{
    uint8_t data[6] = {0};
    get_acc_data(data);
    get_gyro_data(data);
    return;
}

void BMI160::get_acc_data(uint8_t *data) 
{
    uint8_t lsb;
    uint8_t msb;
    uint8_t id = 0;
    read_reg(&data[0], 0x12, 6);
    lsb = data[id++];
    msb = data[id++];
    BMI160_DATA[ACC_X] = ((uint16_t)((msb << 8) | lsb));
    lsb = data[id++];
    msb = data[id++];
    BMI160_DATA[ACC_Y] = ((uint16_t)((msb << 8) | lsb));
    lsb = data[id++];
    msb = data[id++];
    BMI160_DATA[ACC_Z] = ((uint16_t)((msb << 8) | lsb));
    return;
}

void BMI160::get_gyro_data(uint8_t *data)
{
    uint8_t lsb;
    uint8_t msb;
    uint8_t id = 0;
    read_reg(&data[0], 0x0C, 6);
    lsb = data[id++];
    msb = data[id++];
    BMI160_DATA[GYRO_X] = ((uint16_t)((msb << 8) | lsb));
    lsb = data[id++];
    msb = data[id++];
    BMI160_DATA[GYRO_Y] = ((uint16_t)((msb << 8) | lsb));
    lsb = data[id++];
    msb = data[id++];
    BMI160_DATA[GYRO_Z] = ((uint16_t)((msb << 8) | lsb));
    return;
}

size_t BMI160::publish_sensor_data() 
{
    return serializeMsgPack(BMI160_DATA, Serial);
    //return serializeJson(BMI160_DATA, Serial);
}