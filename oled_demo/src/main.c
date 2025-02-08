#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include "WS2812Led.h"
#include "RtosUtils.h"
#include "sensor.h"
#include "WifiConnect.h"
#include "NvParms.h"
#include "MqttClient.h"

/** @brief Initialize the logging module. */
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

#define STRIP_NODE       DT_ALIAS(led_strip)

static const struct device *const rgbled_dev = DEVICE_DT_GET(STRIP_NODE);

static WS2812Led led;
static MqttClient mqtt;
static MqttClient_PubTopic mqtt_topic;

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

    WifiConnect_init();
    WifiConnect_connect(ssid, pass);
    return 0;
}


int main(void)
{
    int sensor_ready;
    int ret;
    struct sensor_value temp;
    struct sensor_value hum;
    WS2812Led_Segment *led_seg = &led.seg;

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

    RTOS_TASK_SLEEP_ms(1000);
    ret = MqttClient_init(&mqtt, "zephyr_test");
    if (ret == 0)
    {
        MqttClient_setTopic(&mqtt_topic, "room/temp", MQTT_QOS_0_AT_MOST_ONCE);
    }

    while (1)
    {
        uint8_t pay[20];

        RTOS_TASK_SLEEP_ms(5000);

        if (sensor_ready == 0)
        {
            double deg_c;
            double deg_f;

            get_temp_hum(&temp, &hum);
            update_display(&temp, &hum);

            deg_c = temp.val1 + (double)(temp.val2)*0.000001;
            deg_f = deg_c*9/5 + 32.;

            sprintf(pay, "%d", (unsigned int)(deg_f + 0.5));
            MqttClient_publish(&mqtt, &mqtt_topic, pay, strlen(pay));
        }
    }
}
