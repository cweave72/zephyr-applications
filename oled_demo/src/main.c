#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include "WS2812Led.h"
#include "RtosUtils.h"
#include "sensor.h"
#include "WifiConnect.h"
#include "NvParms.h"
#include "Publisher.h"
#include "rpc.h"

#include "Sensor.pb.h"

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

#define STRIP_NODE       DT_ALIAS(led_strip)

static const struct device *const rgbled_dev = DEVICE_DT_GET(STRIP_NODE);

static WS2812Led led;
static MqttClient mqtt;
static Publisher_topic_t hello_topic;
static Publisher_topic_t sensor_topic;

static void
wifi_error(void)
{
    LOG_ERR("Rebooting system.");
    sys_reboot(SYS_REBOOT_COLD);
}

static int
init_wifi(void)
{
    char ssid[32];
    char pass[32];
    int ret, pass_len;

    ret = NvParms_load("ssid", NVPARMS_TYPE_STRING, ssid, sizeof(ssid));
    if (ret <= 0)
    {
        LOG_ERR("Error getting ssid from NV: %d", ret);
        return -1;
    }

    pass_len = NvParms_load("pass", NVPARMS_TYPE_STRING, pass, sizeof(pass));
    if (pass_len <= 0)
    {
        LOG_ERR("Error getting pass from NV: %d", pass_len);
        return -1;
    }

    LOG_DBG("ssid=%s", ssid);
    LOG_DBG("password length=%d", pass_len);

    WifiConnect_connect(ssid, pass, wifi_error);
    return 0;
}


int main(void)
{
    int sensor_ready;
    int ret;
    struct sensor_value temp;
    struct sensor_value hum;
    WS2812Led_Segment *led_seg = &led.seg;
    char *ip, *nm, *gw;

    sensor_ready = init_sensor();
    init_display();

    WS2812LED_INIT_SIMPLE(rgbled_dev, &led, "led", 1);

    /* Start blend around the color wheel. */
    CHSV color1 = WS2812LED_HSV_COLOR(HUE_GREEN, 255, 100);
    CHSV color2 = WS2812LED_HSV_COLOR(color1.h + 255, 255, 100);
    led_seg->show(led_seg);
    led_seg->blend(led_seg, true, &color1, &color2, GRAD_LONGEST, 200, 50);

    ret = NvParms_init();
    if (ret < 0)
    {
        LOG_ERR("NvParms module init error : %d", ret);
        return 0;
    }

    ret = init_wifi();
    if (ret < 0)
    {
        LOG_ERR("Error connecting to network.");
        return 0;
    }
    WifiConnect_getIpInfo(&ip, &nm, &gw);

    rpc_init();
    ret = rpc_start_server();
    if (ret < 0)
    {
        LOG_ERR("Error starting RPC server. %d", ret);
    }

    ret = Publisher_init(ip);
    if (ret < 0)
    {
        while (1) { RTOS_TASK_SLEEP_ms(5000); }
    }

    Publisher_createTopic(&hello_topic, "[hello]: ", PUB_TYPE_STRING);
    Publisher_createTopic(&sensor_topic, "[sensor]: ", PUB_TYPE_PROTOBUF);

    sensor_TempSensor temp_sensor;
    uint8_t buf[128];
    Publisher_setProtobuf(&sensor_topic, (void *)sensor_TempSensor_fields,
        &temp_sensor, buf, sizeof(buf));

    uint32_t loop = 0;

    while (1)
    {
        char json_buffer[64];
        char str[64];
        uint8_t size;
        int32_t temp_16;
        int32_t hum_4;

        RTOS_TASK_SLEEP_ms(100);

        Publisher_setString(&hello_topic, "Hello from %s %u", ip, loop++);
        Publisher_publish(&hello_topic);

        if (sensor_ready == 0)
        {
            get_temp_hum(&temp, &hum);
            update_display(&temp, &hum);

            sensor_conv_to_fixedpt(&temp, &hum, 16, 4, &temp_16, &hum_4);

            temp_sensor.temp_deg = temp_16;
            temp_sensor.temp_deg_fl = 16;
            temp_sensor.has_humid = true;
            temp_sensor.humid = hum_4;
            temp_sensor.humid_fl = 4;

            Publisher_publish(&sensor_topic);
            Publisher_sendMsg_INFO("temp = %d; hum = %d", temp_16, hum_4);

            //int len = encode_json_result(
            //    &temp, &hum, json_buffer, sizeof(json_buffer));
            //if (len > 0)
            //{
            //    MqttClient_publish(&mqtt, &mqtt_topic, json_buffer, len);
            //}
        }
    }
}
