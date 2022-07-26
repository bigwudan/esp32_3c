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


// static const char *_mqtt_host = "120.46.207.95";  //mqtt服务器主机mqtt://mqtt.eclipse.org
// static const int _mqtt_port = 1883;    //mqtt服务器主机端口
// static const char *_mqtt_clientId = "f1612828";   //本设备id
// static const char *_mqtt_username = "user";  //接入用户名
// static const char *_mqtt_password = "pass";   //接入密码
static const char *_mqtt_substopic = "topic1/";   //订阅的主题
static const char *_mqtt_pubstopic = "topic2/";  //发布的主题


#define MQTT_URL "120.46.207.95"
#define MQTT_HOST "mqtt://20220711.cn"
#define MQTT_CLIENT_ID "test_no_0"
#define MQTT_USERNAME "534093"
#define MQTT_PASSWORD "version=2018-10-31&res=products%2F534093%2Fdevices%2Ftest_no_0&et=1670377192&method=md5&sign=z2rrvoyuHyYTyxT3zxrrgg%3D%3D"
#define MQTT_PORT 1883

static const char *TAG = "mqtt_mod";
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void _mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    printf("mqtt_event_handler\n");
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id = 0;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");



        //注册命令
        //msg_id = esp_mqtt_client_subscribe(client, "$sys/534093/test_no_0/cmd/request/+", 0);

        msg_id = esp_mqtt_client_subscribe(client, "topic1", 0);
        
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
#if 0     
        msg_id = esp_mqtt_client_publish(client, "$sys/534093/test_no_0/dp/post/json", "{\"id\":123,\"dp\":{\"temp\":[{\"v\":99}]}}", 0, 0, 0);
        ESP_LOGI(TAG, "publish msg_id[%d]\n", msg_id);

#endif        
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static esp_err_t _mqtt_app_start(void)
{
    // esp_mqtt_client_config_t mqtt_cfg = {
    //     .host = MQTT_URL,
    //     .client_id = MQTT_CLIENT_ID,
    //     .username = MQTT_USERNAME,
    //     .password=MQTT_PASSWORD,
    //     .port=MQTT_PORT
    // };
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_HOST,
        .client_id = MQTT_CLIENT_ID,
        .username = MQTT_USERNAME,
        .password=MQTT_PASSWORD,
        .port=MQTT_PORT
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);  //MQTT Client初始化
    //注册mqtt事件，mqtt事件句柄是client
    //如果发生任何事件（ESP_EVENT_ANY_ID），则执行回调函数mqtt_event_handler，额外参数传递client句柄的内容
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, _mqtt_event_handler, NULL);
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
    // _mqtt_app_start();
    // ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &_disconnect_handler, NULL));

    // extern void _mqtt_app();
    // _mqtt_app();
    _mqtt_app_start();
    return ESP_OK;
}