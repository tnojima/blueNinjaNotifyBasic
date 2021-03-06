/**
 * @file BLE.c
 * @breaf Cerevo CDP-TZ01B Instant firmware.
 * BLE
 *
 * @author Cerevo Inc.
 */

/*
Copyright 2015 Cerevo Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

See this page to get basic information
https://github.com/cerevo/hyourowgan/wiki/BLE_Profile

If BLELIB_ERROR_BUSY has occured, change following value or disable some notify to reduce number of calling BLE API
    uint16_t average_count_airp=5;
    uint16_t average_count_motion=10;
    uint16_t count_adc=5;
    uint16_t count_gpio_in=10;

#define BLELIB_ERROR_BUSY				(-4)			Cannot do API because of too many API call

*/

#include <stdio.h>
#include "TZ10xx.h"
#include "PMU_TZ10xx.h"
#include "GPIO_TZ10xx.h"
#include "RNG_TZ10xx.h"

#include "twic_interface.h"
#include "blelib.h"

#include "TZ01_system.h"
#include "TZ01_console.h"
#include "ADCC12_TZ10xx.h"
#include "MPU-9250.h"
#include "BMP280.h"
#include "pwm_out.h"

typedef union {
    float   val;
    uint8_t buf[4];
}   FLOAT_BUF;

#define BNMSG_MTU    (40)
static uint16_t current_mtu = 23;

extern TZ10XX_DRIVER_PMU  Driver_PMU;
extern TZ10XX_DRIVER_RNG  Driver_RNG;
extern TZ10XX_DRIVER_GPIO Driver_GPIO;
extern TZ10XX_DRIVER_ADCC12 Driver_ADCC12;

static uint8_t msg[80];

static uint64_t hrgn_bdaddr  = 0xc00100000000;   //

static void init_io_state(void);

/*--- GATT profile definition ---*/
uint8_t bnmsg_gap_device_name[] = "BLE_SAMPLE00";
uint8_t bnmsg_gap_appearance[] = {0x00, 0x00};

const uint8_t bnmsg_di_manufname[] = "Cerevo";
const uint8_t bnmsg_di_fw_version[] = "0.1";
const uint8_t bnmsg_di_sw_version[] = "0.1";
const uint8_t bnmsg_di_model_string[] = "CDP-TZ01B";

/* BLElib unique id. */
enum {
    GATT_UID_GAP_SERVICE = 0,
    GATT_UID_GAP_DEVICE_NAME,
    GATT_UID_GAP_APPEARANCE,
    
    GATT_UID_DI_SERVICE,
    GATT_UID_DI_MANUF_NAME,
    GATT_UID_DI_FW_VERSION,
    GATT_UID_DI_SW_VERSION,
    GATT_UID_DI_MODEL_STRING,
    /* BlueNinja GPIO Service */
    GATT_UID_GPIO_SERVICE,
    GATT_UID_GPIO_READ,
    GATT_UID_GPIO_WRITE,
    GATT_UID_GPIO_DESC,
    /* BlueNinja PWM Service */
    GATT_UID_PWM_SERVICE,
    GATT_UID_PWM_0_ONOFF,
    GATT_UID_PWM_0_CLOCK,
    GATT_UID_PWM_0_DUTY,
    GATT_UID_PWM_1_ONOFF,
    GATT_UID_PWM_1_CLOCK,
    GATT_UID_PWM_1_DUTY,
    /* BlueNinja UART Service */
    /*
    GATT_UID_UART_SERVICE,
    */
    /* BlueNinja ADC Service */
    GATT_UID_ADC_SERVICE,
    GATT_UID_ADC,
    GATT_UID_ADC_DESC,
    /* BlueNinja Motion sensor Service */
    GATT_UID_MOTION_SERVICE,
    GATT_UID_MOTION,
    GATT_UID_MOTION_DESC,
    /* BlueNinja Airpressure sensor Service */
    GATT_UID_AIRP_SERVICE,
    GATT_UID_AIRP,
    GATT_UID_AIRP_DESC,
    
    /* BlueNinja Battry charger Service */
    GATT_UID_BATT_SERVICE,
    GATT_UID_BATT,
    GATT_UID_BATT_DESC,
};

/* GAP */
const BLELib_Characteristics gap_device_name = {
    GATT_UID_GAP_DEVICE_NAME, 0x2a00, 0, BLELIB_UUID_16,
    BLELIB_PROPERTY_READ,
    BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
    bnmsg_gap_device_name, sizeof(bnmsg_gap_device_name),
    NULL, 0
};
const BLELib_Characteristics gap_appearance = {
    GATT_UID_GAP_APPEARANCE, 0x2a01, 0, BLELIB_UUID_16,
    BLELIB_PROPERTY_READ,
    BLELIB_PERMISSION_READ,
    bnmsg_gap_appearance, sizeof(bnmsg_gap_appearance),
    NULL, 0
};
const BLELib_Characteristics *const gap_characteristics[] = { &gap_device_name, &gap_appearance };
const BLELib_Service gap_service = {
    GATT_UID_GAP_SERVICE, 0x1800, 0, BLELIB_UUID_16,
    true, NULL, 0,
    gap_characteristics, 2
};

/* DIS(Device Informatin Service) */
const BLELib_Characteristics di_manuf_name = {
    GATT_UID_DI_MANUF_NAME, 0x2a29, 0, BLELIB_UUID_16,
    BLELIB_PROPERTY_READ,
    BLELIB_PERMISSION_READ,
    bnmsg_di_manufname, sizeof(bnmsg_di_manufname),
    NULL, 0
};
const BLELib_Characteristics di_fw_version = {
    GATT_UID_DI_FW_VERSION, 0x2a26, 0, BLELIB_UUID_16,
    BLELIB_PROPERTY_READ,
    BLELIB_PERMISSION_READ,
    bnmsg_di_fw_version, sizeof(bnmsg_di_fw_version),
    NULL, 0
};
const BLELib_Characteristics di_sw_version = {
    GATT_UID_DI_SW_VERSION, 0x2a28, 0, BLELIB_UUID_16,
    BLELIB_PROPERTY_READ,
    BLELIB_PERMISSION_READ,
    bnmsg_di_sw_version, sizeof(bnmsg_di_sw_version),
    NULL, 0
};
const BLELib_Characteristics di_model_string = {
    GATT_UID_DI_MODEL_STRING, 0x2a24, 0, BLELIB_UUID_16,
    BLELIB_PROPERTY_READ,
    BLELIB_PERMISSION_READ,
    bnmsg_di_model_string, sizeof(bnmsg_di_model_string),
    NULL, 0
};
const BLELib_Characteristics *const di_characteristics[] = {
    &di_manuf_name, &di_fw_version, &di_sw_version, &di_model_string
};
const BLELib_Service di_service = {
    GATT_UID_DI_SERVICE, 0x180a, 0, BLELIB_UUID_16,
    true, NULL, 0,
    di_characteristics, 4
};

/* BlueNinja GPIO Service */
static uint8_t gpio_read_val;
static uint8_t gpio_write_val;
static uint16_t gpio_enable_val;

//GPIO DATA READ CHARACTERISTICS
const BLELib_Descriptor gpio_desc = {
    GATT_UID_GPIO_DESC, 0x2902, 0, BLELIB_UUID_16,
    BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
    (uint8_t *)&gpio_enable_val, sizeof(gpio_enable_val)
};
const BLELib_Descriptor *const gpio_descs[] = { &gpio_desc };
const BLELib_Characteristics gpio_read_char = {
    GATT_UID_GPIO_READ, 0x988ef07959ddcdfb, 0x00010001672711e5, BLELIB_UUID_128,
    BLELIB_PROPERTY_NOTIFY,
    BLELIB_PERMISSION_READ,
    (uint8_t *)&gpio_read_val, sizeof(gpio_read_val),
    gpio_descs, 1
};
/*
//GPIO DATA WRITE CHARACTERISTICS
const BLELib_Characteristics gpio_srite_char = {
    GATT_UID_GPIO_WRITE, 0x988ef07959ddcdfb, 0x00010002672711e5, BLELIB_UUID_128,
    BLELIB_PROPERTY_WRITE,
    BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
    (uint8_t *)&gpio_write_val, sizeof(gpio_write_val),
    NULL, 0
};


//Service
const BLELib_Characteristics *const gpio_characteristics[] = {
    &gpio_read_char, &gpio_srite_char
};
const BLELib_Service gpio_service = {
    GATT_UID_GPIO_SERVICE, 0x988ef07959ddcdfb, 0x00010000672711e5, BLELIB_UUID_128,
    true, NULL, 0,
    gpio_characteristics, 2
};
*/
//Service
const BLELib_Characteristics *const gpio_characteristics[] = {
    &gpio_read_char
};
const BLELib_Service gpio_service = {
    GATT_UID_GPIO_SERVICE, 0x988ef07959ddcdfb, 0x00010000672711e5, BLELIB_UUID_128,
    true, NULL, 0,
    gpio_characteristics, 1
};

/* BlueNinja PWM Service */
static uint8_t  pwm_0_onoff_val = 0;
static uint32_t pwm_0_clock_val = 1000;
static float    pwm_0_duty_val  = 0.5;

static uint8_t  pwm_1_onoff_val = 0;
static uint32_t pwm_1_clock_val = 1000;
static float    pwm_1_duty_val  = 0.5;

//PWM0 ON/OFF
const BLELib_Characteristics pwm_0_onoff = {
    GATT_UID_PWM_0_ONOFF, 0x988ef07959ddcdfb, 0x00020001672711e5, BLELIB_UUID_128,
    BLELIB_PROPERTY_READ | BLELIB_PROPERTY_WRITE,
    BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
    &pwm_0_onoff_val, sizeof(pwm_0_onoff_val),
    NULL, 0
};
//PWM0 Clock
const BLELib_Characteristics pwm_0_clock = {
    GATT_UID_PWM_0_CLOCK, 0x988ef07959ddcdfb, 0x00020002672711e5, BLELIB_UUID_128,
    BLELIB_PROPERTY_READ | BLELIB_PROPERTY_WRITE,
    BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
    (uint8_t *)&pwm_0_clock_val, sizeof(pwm_0_clock_val),
    NULL, 0
};
//PWM0 Duty
const BLELib_Characteristics pwm_0_duty = {
    GATT_UID_PWM_0_DUTY, 0x988ef07959ddcdfb, 0x00020003672711e5, BLELIB_UUID_128,
    BLELIB_PROPERTY_READ | BLELIB_PROPERTY_WRITE,
    BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
    (uint8_t *)&pwm_0_duty_val, sizeof(pwm_0_duty_val),
    NULL, 0
};
//PWM1 ON/OFF
const BLELib_Characteristics pwm_1_onoff = {
    GATT_UID_PWM_1_ONOFF, 0x988ef07959ddcdfb, 0x00020101672711e5, BLELIB_UUID_128,
    BLELIB_PROPERTY_READ | BLELIB_PROPERTY_WRITE,
    BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
    &pwm_1_onoff_val, sizeof(pwm_1_onoff_val),
    NULL, 0
};
//PWM1 Clock
const BLELib_Characteristics pwm_1_clock = {
    GATT_UID_PWM_1_CLOCK, 0x988ef07959ddcdfb, 0x00020102672711e5, BLELIB_UUID_128,
    BLELIB_PROPERTY_READ | BLELIB_PROPERTY_WRITE,
    BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
    (uint8_t *)&pwm_1_clock_val, sizeof(pwm_1_clock_val),
    NULL, 0
};
//PWM1 Duty
const BLELib_Characteristics pwm_1_duty = {
    GATT_UID_PWM_1_DUTY, 0x988ef07959ddcdfb, 0x00020103672711e5, BLELIB_UUID_128,
    BLELIB_PROPERTY_READ | BLELIB_PROPERTY_WRITE,
    BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
    (uint8_t *)&pwm_1_duty_val, sizeof(pwm_1_duty_val),
    NULL, 0
};
//Service
const BLELib_Characteristics *const pwm_characteristics[] = {
    &pwm_0_onoff, &pwm_0_clock, &pwm_0_duty,
    &pwm_1_onoff, &pwm_1_clock, &pwm_1_duty
};
const BLELib_Service pwm_service = {
    GATT_UID_PWM_SERVICE, 0x988ef07959ddcdfb, 0x00020000672711e5, BLELIB_UUID_128,
    true, NULL, 0,
    pwm_characteristics, 6
};

/* BlueNinja UART Serivece */

/* BlueNinja ADC Service */
static uint16_t adc_enable_val;
static uint8_t adc_val[8];
//ADC
const BLELib_Descriptor adc_desc = {
    GATT_UID_ADC_DESC, 0x2902, 0, BLELIB_UUID_16,
    BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
    (uint8_t *)&adc_enable_val, sizeof(adc_enable_val)
};
const BLELib_Descriptor *const adc_descs[] = { &adc_desc };
const BLELib_Characteristics adc_char = {
    GATT_UID_ADC, 0x988ef07959ddcdfb, 0x00040001672711e5, BLELIB_UUID_128,
    BLELIB_PROPERTY_NOTIFY,
    BLELIB_PERMISSION_READ,
    adc_val, sizeof(adc_val),
    adc_descs, 1
};
//Service
const BLELib_Characteristics *const adc_characteristics[] = {
    &adc_char
};
const BLELib_Service adc_service = {
    GATT_UID_ADC_SERVICE, 0x988ef07959ddcdfb, 0x00040000672711e5, BLELIB_UUID_128,
    true, NULL, 0,
    adc_characteristics, 1
};

/* BlueNinja Motion sensor Service */
static uint16_t motion_enable_val;
static uint8_t motion_val[36];
static int32_t gx, gy, gz, ax, ay, az;
//Motion sensor
const BLELib_Descriptor motion_desc = {
    GATT_UID_MOTION_DESC, 0x2902, 0, BLELIB_UUID_16,
    BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
    (uint8_t *)&motion_enable_val, sizeof(motion_enable_val)
};
const BLELib_Descriptor *const motion_descs[] = { &motion_desc };
const BLELib_Characteristics motion_char = {
    GATT_UID_MOTION, 0x988ef07959ddcdfb, 0x00050001672711e5, BLELIB_UUID_128,
    BLELIB_PROPERTY_NOTIFY,
    BLELIB_PERMISSION_READ,
    motion_val, sizeof(motion_val),
    motion_descs, 1
};
//Service
const BLELib_Characteristics *const motion_characteristics[] = {
    &motion_char
};
const BLELib_Service motion_service = {
    GATT_UID_MOTION_SERVICE, 0x988ef07959ddcdfb, 0x00050000672711e5, BLELIB_UUID_128,
    true, NULL, 0,
    motion_characteristics, 1
};

/* BlueNinja Airpressure sensor Service */
static uint16_t airp_enable_val;
static uint8_t airp_val[6];
//Airp
const BLELib_Descriptor airp_desc = {
    GATT_UID_AIRP_DESC, 0x2902, 0, BLELIB_UUID_16,
    BLELIB_PERMISSION_READ | BLELIB_PERMISSION_WRITE,
    (uint8_t *)&airp_enable_val, sizeof(airp_enable_val)
};
const BLELib_Descriptor *const airp_descs[] = { &airp_desc };
const BLELib_Characteristics airp_char = {
    GATT_UID_AIRP, 0x988ef07959ddcdfb, 0x00060001672711e5, BLELIB_UUID_128,
    BLELIB_PROPERTY_NOTIFY,
    BLELIB_PERMISSION_READ,
    airp_val, sizeof(airp_val),
    airp_descs, 1
};
//Service
const BLELib_Characteristics *const airp_characteristics[] = {
    &airp_char
};
const BLELib_Service airp_service = {
    GATT_UID_AIRP_SERVICE, 0x988ef07959ddcdfb, 0x00060000672711e5, BLELIB_UUID_128,
    true, NULL, 0,
    airp_characteristics, 1
};
/* BlueNinja Battry charger Service */

/* Service list */
const BLELib_Service *const hrgn_service_list[] = {
    &gap_service, &di_service, &gpio_service, &pwm_service, &motion_service, &airp_service, &adc_service
};

/*- INDICATION data -*/
uint8_t bnmsg_advertising_data[] = {
    0x02, /* length of this data */
    0x01, /* AD type = Flags */
    0x06, /* LE General Discoverable Mode = 0x02 */
    /* BR/EDR Not Supported (i.e. bit 37
     * of LMP Extended Feature bits Page 0) = 0x04 */

    0x0d, /* length of this data */
    0x08, /* AD type = Short local name */
     'B','L','E', '_', 'S', 'A', 'M', 'P', 'L', 'E', '0', '0', /* BLE_SAMPLE00 */

    0x05, /* length of this data */
    0x03, /* AD type = Complete list of 16-bit UUIDs available */
    0x00, /* Generic Access Profile Service 1800 */
    0x18,
    0x0A, /* Device Information Service 180A */
    0x18,
};

uint8_t bnmsg_scan_resp_data[] = {
    0x02, /* length of this data */
    0x01, /* AD type = Flags */
    0x06, /* LE General Discoverable Mode = 0x02 */
    /* BR/EDR Not Supported (i.e. bit 37
     * of LMP Extended Feature bits Page 0) = 0x04 */

    0x02, /* length of this data */
    0x0A, /* AD type = TX Power Level (1 byte) */
    0x00, /* 0dB (-127...127 = 0x81...0x7F) */

    0x0d, /* length of this data */
    0x09, /* AD type = Complete local name */
    'B','L','E', '_', 'S', 'A', 'M', 'P', 'L', 'E', '0', '0' /* BLE_SAMPLE00 */
};

/*=== BlueNinja messenger application ===*/
static uint64_t central_bdaddr;

/*= BLElib callback functions =*/
void connectionCompleteCb(const uint8_t status, const bool master, const uint64_t bdaddr, const uint16_t conn_interval)
{
    central_bdaddr = bdaddr;

    BLELib_requestMtuExchange(BNMSG_MTU);

    TZ01_system_tick_start(USRTICK_NO_BLE_MAIN, 100);
}

void connectionUpdateCb(const uint8_t status, const uint16_t conn_interval, const uint16_t conn_latency)
{
}

void disconnectCb(const uint8_t status, const uint8_t reason)
{
    pwm_out_stop(PWM_OUT_0);
    pwm_out_stop(PWM_OUT_1);
    
    for (int gpio = 20; gpio <= 23; gpio++) {
        Driver_GPIO.WritePin(gpio, 0);
    }
    
    init_io_state();
    
    Driver_GPIO.WritePin(11, 0);
    TZ01_system_tick_stop(USRTICK_NO_BLE_MAIN);
}

BLELib_RespForDemand mtuExchangeDemandCb(const uint16_t client_rx_mtu_size, uint16_t *resp_mtu_size)
{
    *resp_mtu_size = BNMSG_MTU;
    sprintf(msg, "%s(): client_rx_mtu_size=%d resp_mtu_size=%d\r\n", __func__, client_rx_mtu_size, *resp_mtu_size);
    TZ01_console_puts(msg);
    return BLELIB_DEMAND_ACCEPT;
}

void mtuExchangeResultCb(const uint8_t status, const uint16_t negotiated_mtu_size)
{
    sprintf(msg, "Negotiated MTU size:%d\r\n", negotiated_mtu_size);
    current_mtu = negotiated_mtu_size;
    TZ01_console_puts(msg);
}

void notificationSentCb(const uint8_t unique_id)
{
}

void indicationConfirmCb(const uint8_t unique_id)
{
}

void updateCompleteCb(const uint8_t unique_id)
{
}

void queuedWriteCompleteCb(const uint8_t status)
{
}

BLELib_RespForDemand readoutDemandCb(const uint8_t *const unique_id_array, const uint8_t unique_id_num)
{
    return BLELIB_DEMAND_ACCEPT;
}

BLELib_RespForDemand writeinDemandCb(const uint8_t unique_id, const uint8_t *const value, const uint8_t value_len)
{
    uint32_t  clock;
    FLOAT_BUF duty;
    uint8_t len;
    uint8_t pin;
    BLELib_RespForDemand ret = BLELIB_DEMAND_REJECT;
    
    switch (unique_id) {
    case GATT_UID_GPIO_WRITE:
        //GPIO20
        pin = (value[0] & 0x10) ? 1 : 0;
        Driver_GPIO.WritePin(20, pin);
        //GPIO21
        pin = (value[0] & 0x20) ? 1 : 0;
        Driver_GPIO.WritePin(21, pin);
        //GPIO22
        pin = (value[0] & 0x40) ? 1 : 0;
        Driver_GPIO.WritePin(22, pin);
        //GPIO23
        pin = (value[0] & 0x80) ? 1 : 0;
        Driver_GPIO.WritePin(23, pin);
        
        if ((value[0] & 0xf0) != (gpio_write_val & 0xf0)) {
            //変化あったので反映
            gpio_write_val &= 0x0f;
            gpio_write_val |= (value[0] & 0xf0);
            BLELib_updateValue(unique_id, &gpio_write_val, sizeof(gpio_write_val));
        }
        ret = BLELIB_DEMAND_ACCEPT;
        break;
    case GATT_UID_PWM_0_ONOFF:
        if (value[0] == 1) {
            if (pwm_out_start(PWM_OUT_0, pwm_0_clock_val, pwm_0_duty_val)) {
                pwm_0_onoff_val = 1;
                BLELib_updateValue(unique_id, &pwm_0_onoff_val, 1);
                ret = BLELIB_DEMAND_ACCEPT;
            }
        } else {
            if (pwm_out_stop(PWM_OUT_0)) {
                pwm_0_onoff_val = 0;
                BLELib_updateValue(unique_id, &pwm_0_onoff_val, 1);
                ret = BLELIB_DEMAND_ACCEPT;
            }
        }
        break;
    case GATT_UID_PWM_0_CLOCK:
        if (value_len < 4) {
            break;
        }
        clock = ((uint32_t)value[3] << 24) | ((uint32_t)value[2] << 16) | ((uint32_t)value[1] << 8) | value[0];
        if ((clock < PWM_MIN_FREQ) || (clock > PWM_MAX_FREQ)) {
            break;
        }
        pwm_0_clock_val = clock;
        if (pwm_0_onoff_val == 1) {
            pwm_out_start(PWM_OUT_0, pwm_0_clock_val, pwm_0_duty_val);
        }
        BLELib_updateValue(unique_id, (uint8_t *)&pwm_0_clock_val, sizeof(pwm_0_clock_val));
        ret = BLELIB_DEMAND_ACCEPT;
        break;
    case GATT_UID_PWM_0_DUTY:
        if (value_len < 4) {
            break;
        }
        memcpy(duty.buf, value, 4);        
        if ((duty.val < 0.0) || (duty.val > 1.0)) {
            break;
        }
        pwm_0_duty_val = duty.val;
        if (pwm_0_onoff_val == 1) {
            pwm_out_start(PWM_OUT_0, pwm_0_clock_val, pwm_0_duty_val);
        }
        BLELib_updateValue(unique_id, (uint8_t *)&pwm_0_duty_val, sizeof(pwm_0_duty_val));
        ret = BLELIB_DEMAND_ACCEPT;
        break;
    case GATT_UID_PWM_1_ONOFF:
        if (value[0] == 1) {
            if (pwm_out_start(PWM_OUT_1, pwm_1_clock_val, pwm_1_duty_val)) {
                pwm_1_onoff_val = 1;
                BLELib_updateValue(unique_id, &pwm_1_onoff_val, 1);
                ret = BLELIB_DEMAND_ACCEPT;
            }
        } else {
            if (pwm_out_stop(PWM_OUT_1)) {
                pwm_1_onoff_val = 0;
                BLELib_updateValue(unique_id, &pwm_1_onoff_val, 1);
                ret = BLELIB_DEMAND_ACCEPT;
            }
        }
        break;
    case GATT_UID_PWM_1_CLOCK:
        if (value_len < 4) {
            break;
        }
        clock = ((uint32_t)value[3] << 24) | ((uint32_t)value[2] << 16) | ((uint32_t)value[1] << 8) | value[0];
        if ((clock < PWM_MIN_FREQ) || (clock > PWM_MAX_FREQ)) {
            break;
        }
        pwm_1_clock_val = clock;
        if (pwm_1_onoff_val == 1) {
            pwm_out_start(PWM_OUT_1, pwm_1_clock_val, pwm_1_duty_val);
        }
        BLELib_updateValue(unique_id, (uint8_t *)&pwm_1_clock_val, sizeof(pwm_1_clock_val));
        ret = BLELIB_DEMAND_ACCEPT;
        break;
    case GATT_UID_PWM_1_DUTY:
        if (value_len < 4) {
            break;
        }
        memcpy(duty.buf, value, 4);        
        if ((duty.val < 0) || (duty.val > 1)) {
            break;
        }
        pwm_1_duty_val = duty.val;
        if (pwm_1_onoff_val == 1) {
            pwm_out_start(PWM_OUT_1, pwm_1_clock_val, pwm_1_duty_val);
        }
        BLELib_updateValue(unique_id, (uint8_t *)&pwm_1_duty_val, sizeof(pwm_1_duty_val));
        ret = BLELIB_DEMAND_ACCEPT;
        break;
    case GATT_UID_MOTION_DESC:
        motion_enable_val = value[0] | (value[1] << 8);
        ret = BLELIB_DEMAND_ACCEPT;
        break;
    case GATT_UID_AIRP_DESC:
        airp_enable_val = value[0] | (value[1] << 8);
        ret = BLELIB_DEMAND_ACCEPT;
        break;
    case GATT_UID_ADC_DESC:
        adc_enable_val = value[0] | (value[1] << 8);
        ret = BLELIB_DEMAND_ACCEPT;
        break;
    case GATT_UID_GPIO_DESC:
        gpio_enable_val = value[0] | (value[1] << 8);
        ret = BLELIB_DEMAND_ACCEPT;
        sprintf(msg, "%dGATT_UID_GPIO_DESC\r\n", gpio_enable_val);
	    TZ01_console_puts(msg);
        break;
    }
    
    return ret;
}

void writeinPostCb(const uint8_t unique_id, const uint8_t *const value, const uint8_t value_len)
{
}

void isrNewEventCb(void)
{
    /* this sample always call BLELib_run() */
}

void isrWakeupCb(void)
{
    /* this callback is not used currently */
}

BLELib_CommonCallbacks tz01_common_callbacks = {
    connectionCompleteCb,
    connectionUpdateCb,
    mtuExchangeResultCb,
    disconnectCb,
    NULL,
    isrNewEventCb,
    isrWakeupCb
};

BLELib_ServerCallbacks tz01_server_callbacks = {
    mtuExchangeDemandCb,
    notificationSentCb,
    indicationConfirmCb,
    updateCompleteCb,
    queuedWriteCompleteCb,
    readoutDemandCb,
    writeinDemandCb,
    writeinPostCb,
};

/** Global **/

int BLE_init_dev(void)
{
    if (TZ1EM_STATUS_OK != tz1emInitializeSystem())
        return 1; /* Must not use UART for LOG before twicIfLeIoInitialize. */
        
    /* create random bdaddr */
    uint32_t randval;
    Driver_PMU.SetPowerDomainState(PMU_PD_ENCRYPT, PMU_PD_MODE_ON);
    Driver_RNG.Initialize();
    Driver_RNG.PowerControl(ARM_POWER_FULL);
    Driver_RNG.Read(&randval);
    Driver_RNG.Uninitialize();
    hrgn_bdaddr |= (uint64_t)randval;
    
    return 0;
}

int BLE_init(uint8_t id)
{
    if (id > 3) {
        return 1;   //invalid id
    }
    /* Set id to Adverising data */
    bnmsg_advertising_data[16] = id + '0';
    /* Set id to Scan respons data*/
    bnmsg_scan_resp_data[19] = id + '0';
    
    bnmsg_gap_device_name[11] = id + '0';
    
    //GPIO等ペリフェラルを初期状態に設定
    init_io_state();
    
    return 0;
}

static uint8_t hist_di16, hist_di17, hist_di18, hist_di19;
static void di_state_update(void)
{
    uint32_t pin;
    
    Driver_GPIO.ReadPin(16, &pin);
    hist_di16 = ((hist_di16 << 1) | pin) & 0x0f;
    
    Driver_GPIO.ReadPin(17, &pin);
    hist_di17 = ((hist_di17 << 1) | pin) & 0x0f;

    Driver_GPIO.ReadPin(18, &pin);
    hist_di18 = ((hist_di18 << 1) | pin) & 0x0f;

    Driver_GPIO.ReadPin(19, &pin);
    hist_di19 = ((hist_di19 << 1) | pin) & 0x0f;
}

static void ble_online_gpio_update_val(void)
{
    uint32_t pin;
    uint8_t di;
    
    di = gpio_read_val;
    if (hist_di16 == 0x00) {
        di &= ~0x01;
    } else if (hist_di16 == 0x0f) {
        di |= 0x01;
    }
    if (hist_di17 == 0x00) {
        di &= ~0x02;
    } else {
        di |= 0x02;
    }
    if (hist_di18 == 0x00) {
        di &= ~0x04;
    } else {
        di |= 0x04;
    }
    if (hist_di19 == 0x00) {
        di &= ~0x08;
    } else {
        di |= 0x08;
    }

    if ((gpio_read_val & 0x0f) != di) {
        //変化あったので反映
        gpio_read_val &= 0xf0;
        gpio_read_val |= di;
       //int ret =  BLELib_updateValue(GATT_UID_GPIO_READ, &gpio_read_val, sizeof(gpio_read_val));
    }
     
    int ret = BLELib_notifyValue(GATT_UID_GPIO_READ, &gpio_read_val, sizeof(gpio_read_val));
    if (ret != BLELIB_OK) {
        sprintf(msg, "BLERET: %d, GPIO:%x\r\n", ret,gpio_read_val);
        TZ01_console_puts(msg);
    }
}

////===================================================
// ADCONVERTER 
////===================================================


void init_adcc12(void)
{
    /* ADCC12へ供給するクロックの設定 */
    //クロックソースの選択
    Driver_PMU.SelectClockSource(PMU_CSM_ADCC12, PMU_CLOCK_SOURCE_SIOSC4M);
    //プリスケーラの設定
    Driver_PMU.SetPrescaler(PMU_CD_ADCC12, 1);

    /* ADCC12ドライバの初期化 */
    Driver_ADCC12.Initialize(NULL, 0);

    Driver_ADCC12.SetScanMode(ADCC12_SCAN_MODE_CYCLIC, ADCC12_CHANNEL_0);
    Driver_ADCC12.SetScanMode(ADCC12_SCAN_MODE_CYCLIC, ADCC12_CHANNEL_1);
    Driver_ADCC12.SetScanMode(ADCC12_SCAN_MODE_CYCLIC, ADCC12_CHANNEL_2);
    Driver_ADCC12.SetScanMode(ADCC12_SCAN_MODE_CYCLIC, ADCC12_CHANNEL_3);

    Driver_ADCC12.SetDataFormat(ADCC12_CHANNEL_0, ADCC12_UNSIGNED);
    Driver_ADCC12.SetDataFormat(ADCC12_CHANNEL_1, ADCC12_UNSIGNED);
    Driver_ADCC12.SetDataFormat(ADCC12_CHANNEL_2, ADCC12_UNSIGNED);
    Driver_ADCC12.SetDataFormat(ADCC12_CHANNEL_3, ADCC12_UNSIGNED);

    Driver_ADCC12.SetComparison(ADCC12_CMP_DATA_0, 0x00, ADCC12_CMP_NO_COMPARISON, ADCC12_CHANNEL_0);
    Driver_ADCC12.SetComparison(ADCC12_CMP_DATA_0, 0x00, ADCC12_CMP_NO_COMPARISON, ADCC12_CHANNEL_1);
    Driver_ADCC12.SetComparison(ADCC12_CMP_DATA_0, 0x00, ADCC12_CMP_NO_COMPARISON, ADCC12_CHANNEL_2);
    Driver_ADCC12.SetComparison(ADCC12_CMP_DATA_0, 0x00, ADCC12_CMP_NO_COMPARISON, ADCC12_CHANNEL_3);
    
    Driver_ADCC12.SetFIFOOverwrite(ADCC12_FIFO_MODE_STREAM);
    Driver_ADCC12.SetSamplingPeriod(ADCC12_SAMPLING_PERIOD_1MS);

    Driver_ADCC12.PowerControl(ARM_POWER_FULL);
    sprintf(
       msg, "ADC INITIALIZED\r\n");
    TZ01_console_puts(msg);
}

static void ble_online_adc_sample_notify(void)
{
    int ret;
    uint16_t adcval=0;
    //Ch0
    Driver_ADCC12.ReadData(ADCC12_CHANNEL_0, &adcval);
    adcval=adcval>>4;
    memcpy(&adc_val[0], &adcval, 2);
    adcval=0;
    //Ch1
    Driver_ADCC12.ReadData(ADCC12_CHANNEL_1, &adcval);
    adcval=adcval>>4;
    memcpy(&adc_val[2], &adcval, 2);
    adcval=0;
    //Ch2
    Driver_ADCC12.ReadData(ADCC12_CHANNEL_2, &adcval);
    adcval=adcval>>4;
    memcpy(&adc_val[4], &adcval, 2);
    adcval=0;
    //Ch3
    Driver_ADCC12.ReadData(ADCC12_CHANNEL_3, &adcval);
    adcval=adcval>>4;
    memcpy(&adc_val[6], &adcval, 2);
    adcval=0;

    ret = BLELib_notifyValue(GATT_UID_ADC, adc_val, sizeof(adc_val));
    if (ret != BLELIB_OK) {
        sprintf(msg, "GATT_UID_ADC: Notify failed. ret=%d\r\n", ret);
        TZ01_console_puts(msg);
    }
}

////===================================================
// AIR PRESSURE AND TEMPARATURE SENSOR
// RAW VALUE WILL BE
//  BMP280_drv_temp_get() -> will return temparature value in 0.01digC
// BMP280_drv_press_get() -> will return air pressure value in 1/256 Pa
////===================================================
static uint16_t temparature;  ///< buffer for average
static uint32_t airpressure;  ///< buffer for average
    
////=====================================================
///  @brief temparature and air pressure sampling and accumulate for average
////=====================================================
static void ble_online_airp_sample_accumulate(void)
{
    temparature += BMP280_drv_temp_get();
    airpressure += BMP280_drv_press_get();
}

////=====================================================
///  @brief temparature and air pressure average and notify
///  @param [in] cont int count number of accmulation cycle
////=====================================================
static void ble_online_airp_average_notify(const int count)
{
    int ret;
    uint16_t temp;
    uint32_t airp;
    
    temp = temparature / count;
    airp = airpressure / count;
    //temparature(0.01digC)
    airp_val[0] = (temp & 0xff);
    airp_val[1] = (temp >> 8) & 0xff;
    //air pressure(1/256Pa)
    airp_val[2] = (airp & 0xff);
    airp_val[3] = (airp >> 8) & 0xff;
    airp_val[4] = (airp >> 16) & 0xff;
    airp_val[5] = (airp >> 24) & 0xff;
    
    ret = BLELib_notifyValue(GATT_UID_AIRP, airp_val, sizeof(airp_val));
    if (ret != BLELIB_OK) {
        sprintf(msg, "GATT_UID_AIRP: Notify failed. ret=%d\r\n", ret);
        TZ01_console_puts(msg);
    }
    temparature = 0;
    airpressure = 0;
}
////=====================================================
///  @brief temparature and air pressure sampling and notify
////=====================================================
static void ble_online_airp_notify(void)
{
    int ret;
    uint16_t temp;
    uint32_t airp;
    
    temp = BMP280_drv_temp_get();
    airp = BMP280_drv_press_get();
    //温度(0.01digC単位)
    airp_val[0] = (temp & 0xff);
    airp_val[1] = (temp >> 8) & 0xff;
    //気圧(1/256Pa単位)
    airp_val[2] = (airp & 0xff);
    airp_val[3] = (airp >> 8) & 0xff;
    airp_val[4] = (airp >> 16) & 0xff;
    airp_val[5] = (airp >> 24) & 0xff;
    
    ret = BLELib_notifyValue(GATT_UID_AIRP, airp_val, sizeof(airp_val));
    if (ret != BLELIB_OK) {
        sprintf(msg, "GATT_UID_AIRP: Notify failed. ret=%d\r\n", ret);
        TZ01_console_puts(msg);
    }
}
////===================================================
// Motion  SENSOR and Magneto sensor
// RAW VALUE WILL BE
//  MPU9250_drv_read_gyro() -> will return gyro value in 1/16.4 deg/s (MAX 2000deg/s)
//  MPU9250_accel_val() -> will return acceleration value in 1/2048 G (MAX 16G)
////===================================================
static void ble_online_motion_sample_accumulate(void)
{
    MPU9250_gyro_val  gyro;
    MPU9250_accel_val acel;
    uint8_t offset=0;
    
    if (MPU9250_drv_read_gyro(&gyro)) {
        gx += (int16_t)gyro.raw_x;
        gy += (int16_t)gyro.raw_y;
        gz += (int16_t)gyro.raw_z;
    }
    if (MPU9250_drv_read_accel(&acel)) {
        ax += (int16_t)acel.raw_x;
        ay += (int16_t)acel.raw_y;
        az += (int16_t)acel.raw_z;
    }
}

static void ble_online_motion_average(uint8_t cnt)
{
    uint8_t offset=0;
    int16_t ave;
    int16_t div;
    MPU9250_magnetometer_val magm;

    //Gyro: X
    ave = gx / cnt;
    memcpy(&motion_val[0  + offset], &ave, 2);
    //Gyro: Y
    ave = gy / cnt;
    memcpy(&motion_val[2  + offset], &ave, 2);
    //Gyro: Z
    ave = gz / cnt;
    memcpy(&motion_val[4  + offset], &ave, 2);
    //Accel: X
    ave = ax / cnt;
    memcpy(&motion_val[6  + offset], &ave, 2);
    //Accel: Y
    ave = ay / cnt;
    memcpy(&motion_val[8  + offset], &ave, 2);
    //Accel: Z
    ave = az / cnt;
    memcpy(&motion_val[10 + offset], &ave, 2);
    
    gx = 0;
    gy = 0;
    gz = 0;
    ax = 0;
    ay = 0;
    az = 0;
    
    /* Magnetometer */
    if (MPU9250_drv_read_magnetometer(&magm)) {
        //Magnetometer: X
        memcpy(&motion_val[12 + offset], &magm.raw_x, 2);
        //Magnetometer: Y
        memcpy(&motion_val[14 + offset], &magm.raw_y, 2);
        //Magnetometer: Z
        memcpy(&motion_val[16 + offset], &magm.raw_z, 2);
    } else {
        TZ01_console_puts("MPU9250_drv_read_magnetometer() failed.\r\n");
    }
}

static void ble_online_motion_notify(void)
{
    int ret;
    int val_len=18;
    for (int i = 0; i < (sizeof(motion_val) / 2); i++) {
        if (motion_val[i] != 0) {
            //計測結果が保持られてる
            ret = BLELib_notifyValue(GATT_UID_MOTION, motion_val, val_len);
            if (ret != BLELIB_OK) {
                sprintf(msg, "GATT_UID_MOTION: Notify failed. ret=%d\r\n", ret);
                TZ01_console_puts(msg);
            }
            break;
        }
    }
}

static bool is_adv = false;
static bool is_online=false;
static bool is_reg = false;
static uint8_t led_blink = 0;
static uint8_t cnt = 0;

int BLE_main(void)
{
    int ret, res = 0;
    BLELib_State state;
    bool has_event;
    uint32_t pin;
    uint16_t average_count_airp=5;
    uint16_t average_count_motion=10;
    uint16_t count_adc=5;
    uint16_t count_gpio_in=10;
    
    state = BLELib_getState();
    has_event = BLELib_hasEvent();

    switch (state) {
        case BLELIB_STATE_UNINITIALIZED:
            is_reg = false;
            is_adv = false;
            is_online=false;
            TZ01_console_puts("BLELIB_STATE_UNINITIALIZED\r\n");
            ret = BLELib_initialize(hrgn_bdaddr, BLELIB_BAUDRATE_2304, &tz01_common_callbacks, &tz01_server_callbacks, NULL, NULL);
            if (ret != BLELIB_OK) {
                TZ01_console_puts("BLELib_initialize() failed.\r\n");
                return -1;  //Initialize failed
            }
            break;
        case BLELIB_STATE_INITIALIZED:
            if (is_reg == false) {
                TZ01_console_puts("BLELIB_STATE_INITIALIZED\r\n");
                BLELib_setLowPowerMode(BLELIB_LOWPOWER_ON);
                if (BLELib_registerService(hrgn_service_list, 7) == BLELIB_OK) {
                    is_reg = true;
                } else {
                    return -1;  //Register failed
                }
            }
            break;
        case BLELIB_STATE_READY:
            if (is_adv == false) {
                TZ01_console_puts("BLELIB_STATE_READY\r\n");
                ret = BLELib_startAdvertising(bnmsg_advertising_data, sizeof(bnmsg_advertising_data), bnmsg_scan_resp_data, sizeof(bnmsg_scan_resp_data));
                if (ret == BLELIB_OK) {
                    is_adv = true;
                    Driver_GPIO.WritePin(11, 1);
                    cnt = 0;
                }
            }
            break;
        case BLELIB_STATE_ADVERTISING:
            is_adv = false;
            break;
        case BLELIB_STATE_ONLINE:
            if(is_online == false){
                TZ01_console_puts("BLELIB_STATE_ONLINE\r\n");
                is_online=true;
            }
            if (TZ01_system_tick_check_timeout(USRTICK_NO_BLE_MAIN)) {
                TZ01_system_tick_start(USRTICK_NO_BLE_MAIN, 10);
                
                //LED点滅(0, 200, 400, 600, 800ms)
                if ((cnt % 20) == 0) {
                    led_blink = (led_blink == 0) ? 1 : 0;
                    Driver_GPIO.WritePin(11, led_blink);
                }
                
                //GPIO入力サンプリング/通知(count_gpio_in*10ms毎)
                if (gpio_enable_val == 1) {
                    di_state_update();
                    if ((cnt % count_gpio_in) == 0) {
                        ble_online_gpio_update_val();
                    }
                }
                
                //ADC通知
                if (adc_enable_val == 1) {
                    if ((cnt % count_adc) == 0) {
                        ble_online_adc_sample_notify();
                    }
                }   
                //気圧センサー読み取り(毎回)/通知(average_count_airp*10ms)
                if (airp_enable_val == 1) {
                    ble_online_airp_sample_accumulate();
                    if ((cnt % average_count_airp) == 0) {
                        ble_online_airp_average_notify(average_count_airp);
//                        ble_online_airp_notify();
                    }
                }

                //モーションセンサ通知
                if (motion_enable_val == 1) {
                    ble_online_motion_sample_accumulate();
                    if ((cnt % average_count_motion) == 0) {
                        ble_online_motion_average(average_count_motion);
                        ble_online_motion_notify();
                    }
                }                
                
                //1000msでアップラウンド
                cnt = (cnt + 1) % 100;
            }
            break;
        default:
            break;
    }
    
    if (has_event) {
        ret = BLELib_run();
        if (ret != BLELIB_OK) {
            res = -1;
            sprintf(msg, "BLELib_run() ret: %d\r\n", ret);
            TZ01_console_puts(msg);
        }
    }

    return res;
}

void BLE_stop(void)
{
    switch (BLELib_getState()) {
        case BLELIB_STATE_ADVERTISING:
            BLELib_stopAdvertising();
            break;
        case BLELIB_STATE_ONLINE:
            BLELib_disconnect(central_bdaddr);
            break;
        default:
            break;
    }
    BLELib_finalize();
    Driver_GPIO.WritePin(11, 0);
}

/** local **/
static void init_io_state(void)
{
    /* Initialize values. */
    //GPIO
    gpio_read_val = 0;
    gpio_write_val = 0;
    BLELib_updateValue(GATT_UID_GPIO_WRITE, &gpio_write_val, 1);
    
    //PWM0
    pwm_0_onoff_val = 0;
    pwm_0_clock_val = 1000;
    pwm_0_duty_val  = 0.5;
    BLELib_updateValue(GATT_UID_PWM_0_ONOFF, &pwm_0_onoff_val, 1);
    BLELib_updateValue(GATT_UID_PWM_0_CLOCK, (uint8_t *)&pwm_0_clock_val, sizeof(pwm_0_clock_val));
    BLELib_updateValue(GATT_UID_PWM_0_DUTY, (uint8_t *)&pwm_0_duty_val, sizeof(pwm_0_duty_val));
    //PWM1
    pwm_1_onoff_val = 0;
    pwm_1_clock_val = 1000;
    pwm_1_duty_val  = 0.5;
    BLELib_updateValue(GATT_UID_PWM_1_ONOFF, &pwm_1_onoff_val, 1);
    BLELib_updateValue(GATT_UID_PWM_1_CLOCK, (uint8_t *)&pwm_1_clock_val, sizeof(pwm_1_clock_val));
    BLELib_updateValue(GATT_UID_PWM_1_DUTY, (uint8_t *)&pwm_1_duty_val, sizeof(pwm_1_duty_val));
    //Motion sensor
    motion_enable_val = 0;
    memset(motion_val, 0, sizeof(motion_val));
    //Airpressure sensor
    airp_enable_val = 0;
    memset(airp_val, 0, sizeof(airp_val));
    
    init_adcc12();  //12bit ADCの初期化
    memset(adc_val, 0, sizeof(adc_val));
    
    gpio_read_val=0x00;
    gpio_write_val=0x00;
    gpio_enable_val=0;

    Driver_ADCC12.Start();
}

