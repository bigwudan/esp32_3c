#include "esp_log.h"

//wifi

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

//http
#include <esp_http_server.h>

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"
#include "wifi_mod.h"


#define ESP_WIFI_SSID "TP-LINK_61D2"         //wifidssid，就是家里路由器的名字
#define ESP_WIFI_PASS "datuo123456"     //wifi密码，家里路由器的密码
#define ESP_MAXIMUM_RETRY 3 //尝试连接wifi的最大次数，若超过这个数则停止连接

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t _wifi_event_group; //声明一个事件组，事件组主要是考虑到很多网络操作需要在我们连接到路由器并拿到IP之后才能进行


#define WIFI_CONNECTED_BIT BIT0 //定义的事件组的0位，标志连接成功
#define WIFI_FAIL_BIT BIT1      //定义的事件组的1位，标志连接失败


static const char *TAG = "WIFI_MOD";

static int s_retry_num = 0;


static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    //WIFI_EVENT_STA_START为初始事件状态，在此事情状态中，调用esp_wifi_connect()操作会连接到路由器（AP）上，
    //连接成功后，事件状态位会变为WIFI_EVENT_STA_CONNECTED，此时会调用DHCP进行请求IP，
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    //如果连接失败，事件状态位会变为WIFI_EVENT_STA_DISCONNECTED，继续连接直到连接次数超过了最大允许值
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            //超过了最大连接次数还连不上，则置位事件组的WIFI_FAIL_BIT标志
            xEventGroupSetBits(_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    //在拿到IP之后事件状态会变更为IP_EVENT_STA_GOT_IP。在这里我们使用了xEventGroupSetBits，设置事件组标志，
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        //将事件传递过来的数据格式化为ip格式
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        //将IP格式华为字符串打印出来
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        //成功获取了IP，置位事件组中的WIFI_CONNECTED_BIT标志
        xEventGroupSetBits(_wifi_event_group, WIFI_CONNECTED_BIT);
    }

}


/*
初始化wifi
*/
esp_err_t wifi_mod_init(){

    esp_err_t res;
    _wifi_event_group = xEventGroupCreate();   //FreeRTOS创建事件组，返回其句柄

    ESP_ERROR_CHECK(esp_netif_init());  //初始化底层TCP/IP堆栈

    ESP_ERROR_CHECK(esp_event_loop_create_default());   //创建默认事件循环，能够默认处理wifi、以太网、ip等事件
    esp_netif_create_default_wifi_sta();    //创建默认的 WIFI STA. 如果出现任何初始化错误，此API将中止。

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();    //指定需要初始化wifi底层的默认参数信息
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));   //用上述信息来初始化WIFI硬件。

	//向上面创建的的事件循环中注册事件和事件处理函数
    //注册一个事件句柄到WIFI_EVENT事件，如果发生任何事件（ESP_EVENT_ANY_ID），则执行回调函数event_handler，无额外参数传递
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    //注册一个事件句柄到IP_EVENT事件，如果发生获取了IP事件（IP_EVENT_STA_GOT_IP），则执行回调函数event_handler，无额外参数传递
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    /*
    wifi_config是WIFI连接时的配置信息，作为STA时只需要考虑sta的参数信息，下述代码只是制定了最基本的ssid和password信息，
    除此之外，还可以指定bssid和channel等相关信息。
    */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));  //设置wifi模式为sta
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));    //设置wifi的配置参数
    ESP_ERROR_CHECK(esp_wifi_start());  //使用当前配置启动wifi工作

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    //FreeRTOS事件组等待wifi连接完成（WIFI_CONNECTED_BIT置位）或者超过最大尝试次数后连接失败（WIFI_FAIL_BIT置位）
    //两种状态的置位都是在上边的event_handler()回调函数中执行的
    EventBits_t bits = xEventGroupWaitBits(_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    //测试一下回调函数把哪一个状态置位了
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
        res = ESP_OK;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
        res = ESP_FAIL;         
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        res = ESP_ERR_TIMEOUT;
    }

    //注销一个事件句柄到WIFI_EVENT事件，如果发生任何事件（IP_EVENT_STA_GOT_IP），不再执行回调函数event_handler，无参数传递
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    //注销一个事件句柄到WIFI_EVENT事件，如果发生任何事件（ESP_EVENT_ANY_ID），不再执行回调函数event_handler，无参数传递
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    //FreeRTOS销毁wifi事件组
    vEventGroupDelete(_wifi_event_group);



    return res;
}


/*
启动wifi
*/
esp_err_t wifi_mod_start(){

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();   //始化NVS存储（非易失性flash）
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    ESP_LOGI(TAG, "wifi_mod_start\n");

    ret = wifi_mod_init();

    return ret;
}