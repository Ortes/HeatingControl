/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <math.h>
#include <time.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>

#include <esp_http_server.h>
#include "driver/gpio.h"

#include "utils.h"

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 * The examples use simple WiFi configuration that you can set via
 * 'make menuconfig'.
 * If you'd rather not, just change the below entries to strings
 * with the config you want -
 * ie. #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD
#define ENABLE_GPIO 26

static const char *TAG="APP";

// {interval, duration}
static double steps[2] = {0, INFINITY};
#define DURATION steps[1]
#define INTERVAL steps[0]

esp_err_t pourcent_get_handler(httpd_req_t *req)
{
  char buffer[0x10];
  int pourcent = DURATION / (DURATION + INTERVAL) * 100;
  pourcent = pourcent > 100 ? 100 : pourcent;
  snprintf(buffer, 0x10, "%i", pourcent);
  httpd_resp_sendstr_chunk(req, buffer);
  httpd_resp_sendstr_chunk(req, NULL);
  return ESP_OK;
}

esp_err_t settings_get_handler(httpd_req_t *req)
{
  char buffer[0x200];
  if (strchr(req->uri, '?') == NULL) {
    if (INTERVAL == INFINITY)
      snprintf(buffer, 0x100, "{ \"interval\" : null,  \"duration\" : %f }", DURATION);
    else if (DURATION == INFINITY)
      snprintf(buffer, 0x100, "{ \"interval\" : %f,  \"duration\" : null }", INTERVAL);
    else
      snprintf(buffer, 0x100, "{ \"interval\" : %f,  \"duration\" : %f }", INTERVAL, DURATION);
    httpd_resp_sendstr_chunk(req, buffer);
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
  }
  int done = 0;
  if (get_value(req->uri, "interval", buffer, 0x200) == ESP_OK) {
    done = 1;
    INTERVAL = atof(buffer);
    INTERVAL = INTERVAL == NAN ? 100 : INTERVAL;
    INTERVAL = INTERVAL < .2 ? .2 : INTERVAL;
    ESP_LOGI(TAG, "Interval changed to %f", INTERVAL);
  }
  if (get_value(req->uri, "duration", buffer, 0x200) == ESP_OK) {
    done = 1;
    DURATION = atof(buffer);
    DURATION = DURATION == NAN ? 100 : DURATION;
    DURATION = DURATION < .2 ? .2 : DURATION;
    ESP_LOGI(TAG, "Duration changed to %f", DURATION);
  }
  if (get_value(req->uri, "pourcent", buffer, 0x200) == ESP_OK) {
    float p = atof(buffer);
    if (p != NAN && p >= 0 && p <= 100) {
      ESP_LOGI(TAG, "Pourcent changed to %f", p);
      if (p == 100) {
        DURATION = INFINITY;
        INTERVAL = .2;
      } else if (p == 0) {
        DURATION = .2;
        INTERVAL = INFINITY;
      } else {
        p = (p * .99 + .5) * 1.8; // scale to 5 mins
        DURATION = p;
        INTERVAL = 180 - p;
      }
      done = 1;
    }
  }
  if (!done)
    httpd_resp_sendstr_chunk(req, "FAILED");
  else
    httpd_resp_sendstr_chunk(req, "OK");

  httpd_resp_sendstr_chunk(req, NULL);
  return ESP_OK;
}

httpd_uri_t pourcent = {
    .uri       = "/pourcent",
    .method    = HTTP_GET,
    .handler   = pourcent_get_handler,
};

httpd_uri_t settings = {
    .uri       = "/*",
    .method    = HTTP_GET,
    .handler   = settings_get_handler,
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &pourcent);
        httpd_register_uri_handler(server, &settings);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void heating_handling() {
  clock_t last = clock();
  int step = 1;
  ESP_ERROR_CHECK(gpio_set_level(ENABLE_GPIO, 1));
  for (;;) {
    if (((double)(clock() - last) / CLOCKS_PER_SEC) > steps[step]) {
      step ^= 1;
      last = clock();
      ESP_ERROR_CHECK(gpio_set_level(ENABLE_GPIO, step));
      ESP_LOGI(TAG, "SET TO %i", step);
    }
    vTaskDelay(1);
  }
}

void stop_webserver(httpd_handle_t server)
{
    httpd_stop(server);
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    httpd_handle_t *server = (httpd_handle_t *) ctx;
    int power = 0;
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        ESP_LOGI(TAG, "Got IP: '%s'",
                ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

        /* Start the web server */
        if (*server == NULL) {
            *server = start_webserver();
        }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_ERROR_CHECK(esp_wifi_get_max_tx_power((int8_t*)&power));
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED power = %i", power);
        ESP_ERROR_CHECK(esp_wifi_connect());

        /* Stop the web server */
        if (*server) {
            stop_webserver(*server);
            *server = NULL;
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void *arg)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, arg));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());

}

void app_main()
{
    static httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(nvs_flash_init());
    initialise_wifi(&server);
    gpio_pad_select_gpio(ENABLE_GPIO);
    gpio_set_direction(ENABLE_GPIO, GPIO_MODE_OUTPUT);
    ESP_ERROR_CHECK(gpio_set_level(ENABLE_GPIO, 0));
    vTaskDelay(300 / portTICK_PERIOD_MS);
    ESP_ERROR_CHECK(gpio_set_level(ENABLE_GPIO, 1));

    xTaskCreate(&heating_handling, "heating_handle_task", 3 * 1024, NULL, 5, NULL);
}
