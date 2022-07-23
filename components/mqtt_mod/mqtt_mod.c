#include "esp_log.h"

//wifi

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

//http
#include <esp_http_server.h>

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"


static const char *_mqtt_host = "120.46.207.95";  //mqtt服务器主机mqtt://mqtt.eclipse.org
static const int _mqtt_port = 1883;    //mqtt服务器主机端口
static const char *_mqtt_clientId = "f1612828";   //本设备id
static const char *_mqtt_username = "user";  //接入用户名
static const char *_mqtt_password = "pass";   //接入密码
static const char *_mqtt_substopic = "topic1/";   //订阅的主题
static const char *_mqtt_pubstopic = "topic2/";  //发布的主题

static const char *TAG = "mqtt_mod";


static esp_err_t _mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    // your_context_t *context = event->context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:  //MQTT连上事件
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        //MQTT Client发布主题函数，服务质量qos1，发布的数据是data-3
        //msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        msg_id = esp_mqtt_client_publish(client, _mqtt_pubstopic, "hello mqtt from esp32!", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
    
        //msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);   //MQTT Client订阅主题函数，服务质量qos0
        msg_id = esp_mqtt_client_subscribe(client, _mqtt_substopic, 0);   //MQTT Client订阅主题函数，服务质量qos0
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
    
        // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);   //MQTT Client订阅主题函数，服务质量qos1
        // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
    
        // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");    //MQTT Client取消订阅主题函数
        // ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:   //MQTT断开连接事件
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    
    case MQTT_EVENT_SUBSCRIBED: //MQTT发送订阅成功事件
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        //回个消息，表示成功了
        //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        msg_id = esp_mqtt_client_publish(client, _mqtt_pubstopic, "hello mqtt，now subscribed ok!", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:   //MQTT取消订阅事件
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:  //MQTT发布成功事件
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:   //MQTT接收数据事件
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:  //MQTT错误事件
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:0x%02X", event->event_id);
        break;
    }
    return ESP_OK;
}

static esp_err_t _mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = _mqtt_host,   //MQTT服务器地址
        //.uri = Host,    //MQTT服务器域名
        .port = _mqtt_port,   //MQTT服务器端口
        .client_id = _mqtt_clientId,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);  //MQTT Client初始化
    //注册mqtt事件，mqtt事件句柄是client
    //如果发生任何事件（ESP_EVENT_ANY_ID），则执行回调函数mqtt_event_handler，额外参数传递client句柄的内容
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, _mqtt_event_handler, client);
    esp_mqtt_client_start(client);  //MQTT Client启动
    return ESP_OK;
}

static void _mqtt_connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "_mqtt_connect_handler");
   _mqtt_app_start();
}


static void _disconnect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "_disconnect_handler");

}

/*
启动mqtt
*/
esp_err_t mqtt_mod_start(void){
    ESP_LOGI(TAG, "mqtt_mod_start-------------connection\n");
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &_mqtt_connect_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &_disconnect_handler, NULL));
    return ESP_OK;
}