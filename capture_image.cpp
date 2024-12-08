#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_camera.h>
#include "Base64.h"  // Include Base64 library

// Wi-Fi credentials
const char* ssid = "rednote";
const char* password = "amir@1212";

// MQTT broker details
const char* mqtt_server = "122.166.149.171";
const int mqtt_port = 1883;
const char* mqtt_topic = "cam";

WiFiClient espClient;
PubSubClient client(espClient);

#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27

#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM    5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

// 4 for flash led or 33 for normal led
#define LED_GPIO_NUM   4

void setup_camera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA; 
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    while (1);
  }
}

void setup_wifi() {
  delay(10);
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("ESP32CAM")) {
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.print("Failed to connect, rc=");
      Serial.print(client.state());
      Serial.println(" Trying again in 5 seconds...");
      delay(5000);
    }
  }
}

void publish_image_in_chunks(const uint8_t* buffer, size_t length, size_t chunk_size) {
  size_t offset = 0;
  while (offset < length) {
    size_t to_send = min(chunk_size, length - offset);
    if (!client.publish(mqtt_topic, buffer + offset, to_send, false)) {
      Serial.println("Failed to publish chunk");
      return;
    }
    offset += to_send;
    delay(10);  // Small delay to avoid overwhelming the broker
  }
  Serial.println("Image published in chunks");
}

void capture_and_send_image() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.print("Captured image size: ");
  Serial.print(fb->len);
  Serial.println(" bytes");

  if (client.connected()) {
    publish_image_in_chunks(fb->buf, fb->len, 10240);  // Chunk size of 10 KB
  } else {
    Serial.println("MQTT client not connected");
  }

  esp_camera_fb_return(fb);
}

// binary 

// void capture_and_send_image() {
//   camera_fb_t *fb = esp_camera_fb_get();
//   if (!fb) {
//     Serial.println("Camera capture failed");
//     return;
//   }

//   if (client.connected()) {
//     // Publish the image buffer as binary data
//     if (client.publish(mqtt_topic, (const uint8_t*)fb->buf, fb->len)) {
//       Serial.println("Image published to MQTT topic");
//     } else {
//       Serial.println("Failed to publish image");
//     }
//   } else {
//     Serial.println("MQTT client not connected");
//   }
//   esp_camera_fb_return(fb);
// }

// void publish_in_chunks(String& data, const char* topic, size_t chunk_size) {
//   size_t data_len = data.length();
//   for (size_t i = 0; i < data_len; i += chunk_size) {
//     String chunk = data.substring(i, i + chunk_size);
//     if (!client.publish(topic, chunk.c_str())) {
//       Serial.println("Failed to publish chunk");
//       return;
//     }
//     delay(10); // Small delay to ensure broker processes chunks
//   }
//   Serial.println("All chunks published successfully");
// }

// base64 

// void capture_and_send_image() {
//   camera_fb_t *fb = esp_camera_fb_get();
//   if (!fb) {
//     Serial.println("Camera capture failed");
//     return;
//   }

//   // Encode image to Base64
//   String image_base64 = base64::encode((const unsigned char*)fb->buf, fb->len);
//   esp_camera_fb_return(fb);

//   if (client.connected()) {
//     publish_in_chunks(image_base64, mqtt_topic, 1024); // Publish in 1KB chunks
//   } else {
//     Serial.println("MQTT client not connected");
//   }
// }


// void capture_and_send_image() {
//   camera_fb_t *fb = esp_camera_fb_get();
//   if (!fb) {
//     Serial.println("Camera capture failed");
//     return;
//   }

//   // Encode image to Base64
//   String image_base64 = base64::encode(fb->buf, fb->len);
//   esp_camera_fb_return(fb);

//   if (client.connected()) {
//     if (client.publish(mqtt_topic, image_base64.c_str())) {
//       Serial.println("Image published to MQTT topic");
//     } else {
//       Serial.println("Failed to publish image");
//     }
//   }
// }

void setup() {
  Serial.begin(115200);

  setup_camera();
  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  capture_and_send_image();
  

  delay(10000); // Delay between image captures (10 seconds)
}