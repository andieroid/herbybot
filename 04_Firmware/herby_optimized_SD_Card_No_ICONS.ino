/*
 * ESP32-CAM Hydroponic Pipe Inspector - OPTIMIZED FOR SPEED
 * Faster WiFi performance, simplified web interface
 * AUTO-SAVES IMAGES TO SD CARD
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "FS.h"
#include "SD_MMC.h"

// Access Point credentials
const char* ap_ssid = "Herby";
const char* ap_password = "";  // Open network

// Fixed IP
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Motor control pins
#define MOTOR_IN1 12
#define MOTOR_IN2 13

// Movement parameters
#define WHEEL_DIAMETER_CM 4.5
#define DISTANCE_PER_STOP_CM 8.0
#define TOTAL_STOPS 16
#define DEMO_REVERSE_AFTER_STOP 4
#define DEMO_REVERSE_STOPS 3

// Motor timing - CALIBRATE THIS!
#define MOTOR_SPEED_DELAY 800
#define STOP_DURATION 3000

// Camera pins
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WebServer server(80);

// State
int currentStop = 0;
bool isMoving = false;
bool sequenceComplete = false;
bool cameraAvailable = false;
bool sdCardAvailable = false;
int imageCounter = 0;  // For unique filenames

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== HERBY OPTIMIZED ===\n");
  
  // Motor pins
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  stopMotor();
  Serial.println("✓ Motor ready");
  
  // WiFi optimization settings
  WiFi.mode(WIFI_AP);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Max power for better signal
  delay(100);
  
  // Start AP with optimized settings
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  // Channel 1, not hidden, max 2 connections (reduce overhead)
  WiFi.softAP(ap_ssid, ap_password, 1, false, 2);
  
  delay(500);
  Serial.println("✓ WiFi AP: Herby");
  Serial.println("✓ IP: 192.168.4.1");
  
  // Web server - simplified routes
  server.on("/", handleRoot);
  server.on("/img", handleImage);  // Single quick image
  server.on("/f", handleForward);  // Short URLs = faster
  server.on("/r", handleReverse);
  server.on("/s", handleStop);
  server.on("/go", handleStart);
  server.begin();
  Serial.println("✓ Web server ready");
  
  // Camera last
  Serial.println("\nCamera init...");
  delay(300);
  
  if (initCamera()) {
    Serial.println("✓ Camera OK");
    cameraAvailable = true;
  } else {
    Serial.println("⚠ Camera failed (check power)");
    cameraAvailable = false;
  }
  
  // Initialize SD card
  Serial.println("\nSD Card init...");
  if (initSDCard()) {
    Serial.println("✓ SD Card ready");
    Serial.print("  Images will be saved to: /herby/");
    sdCardAvailable = true;
    
    // Create directory for images
    if (!SD_MMC.exists("/herby")) {
      SD_MMC.mkdir("/herby");
      Serial.println("\n  Created /herby folder");
    }
    
    // Count existing images to continue numbering
    imageCounter = countExistingImages();
    Serial.printf("  Starting from image #%d\n", imageCounter + 1);
  } else {
    Serial.println("⚠ SD Card not found");
    Serial.println("  Images won't be saved");
    Serial.println("  Insert SD card and restart");
    sdCardAvailable = false;
  }
  
  Serial.println("\n=== READY ===");
  Serial.println("Connect to: Herby");
  Serial.println("Open: http://192.168.4.1\n");
}

void loop() {
  server.handleClient();  // Always handle web requests first
  
  // Run sequence only once when started
  static bool sequenceRunning = false;
  if (isMoving && !sequenceRunning && !sequenceComplete) {
    sequenceRunning = true;
    runInspectionSequence();
    sequenceRunning = false;
  }
  
  yield();  // Let WiFi stack process
}

// ===== MOTOR =====

void moveForward() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
}

void moveReverse() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, HIGH);
}

void stopMotor() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
}

void moveDistance(bool forward) {
  forward ? moveForward() : moveReverse();
  delay(MOTOR_SPEED_DELAY);
  stopMotor();
  delay(50);
}

// ===== CAMERA =====

bool initCamera() {
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // OPTIMIZED: Small, fast images
  config.frame_size = FRAMESIZE_QVGA;  // 320x240 - very small
  config.jpeg_quality = 25;             // Lower quality = faster (0-63, higher = lower quality)
  config.fb_count = 1;                  // Single buffer
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    return false;
  }
  
  // Fast camera settings
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    s->set_framesize(s, FRAMESIZE_QVGA);  // Force small size
    s->set_quality(s, 25);                // Fast JPEG compression
    s->set_brightness(s, 1);              // Slightly brighter
    s->set_contrast(s, 0);
    s->set_saturation(s, -1);             // Less saturation = faster processing
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_wb_mode(s, 0);
    s->set_exposure_ctrl(s, 1);
    s->set_aec2(s, 0);
    s->set_gain_ctrl(s, 1);
    s->set_agc_gain(s, 0);
    s->set_bpc(s, 0);
    s->set_wpc(s, 1);
    s->set_raw_gma(s, 1);
    s->set_lenc(s, 1);
    s->set_hmirror(s, 0);
    s->set_vflip(s, 0);
    s->set_dcw(s, 1);
  }
  
  return true;
}

// ===== SD CARD =====

bool initSDCard() {
  // Mount SD card in 1-bit mode (4-bit conflicts with camera flash LED)
  if (!SD_MMC.begin("/sdcard", true)) {
    return false;
  }
  
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    return false;
  }
  
  // Print SD card info
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("  SD Card Size: %lluMB\n", cardSize);
  
  return true;
}

int countExistingImages() {
  // Count how many images already exist to continue numbering
  int count = 0;
  File root = SD_MMC.open("/herby");
  if (!root) {
    return 0;
  }
  
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      String filename = String(file.name());
      if (filename.endsWith(".jpg")) {
        count++;
      }
    }
    file = root.openNextFile();
  }
  
  return count;
}

bool saveImageToSD(camera_fb_t *fb) {
  if (!sdCardAvailable || !fb) {
    return false;
  }
  
  imageCounter++;
  
  // Create filename: /herby/IMG_0001.jpg
  char filename[32];
  sprintf(filename, "/herby/IMG_%04d.jpg", imageCounter);
  
  File file = SD_MMC.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return false;
  }
  
  // Write image data
  file.write(fb->buf, fb->len);
  file.close();
  
  Serial.printf("✓ Saved: %s (%d bytes)\n", filename, fb->len);
  return true;
}

// ===== SEQUENCE =====

void runInspectionSequence() {
  Serial.println("\n=== START SEQUENCE ===");
  
  for (int stop = 1; stop <= TOTAL_STOPS && isMoving; stop++) {  // Check isMoving flag
    currentStop = stop;
    Serial.printf("\n--- Stop %d/%d ---\n", stop, TOTAL_STOPS);
    
    moveDistance(true);
    
    // Capture and save image at each stop
    if (cameraAvailable && isMoving) {  // Check before capture
      camera_fb_t * fb = esp_camera_fb_get();
      if (fb) {
        saveImageToSD(fb);
        esp_camera_fb_return(fb);
      }
    }
    
    // Allow stop command during delay
    for (int i = 0; i < STOP_DURATION / 100 && isMoving; i++) {
      server.handleClient();  // Process web requests
      delay(100);
    }
    
    if (!isMoving) {
      Serial.println("\n*** STOPPED BY USER ***");
      break;
    }
    
    // Demo reverse
    if (stop == DEMO_REVERSE_AFTER_STOP && isMoving) {
      Serial.println("\n*** DEMO: Reversing... ***");
      for (int i = 0; i < DEMO_REVERSE_STOPS && isMoving; i++) {
        Serial.printf("Reverse %d/%d\n", i+1, DEMO_REVERSE_STOPS);
        moveDistance(false);
        
        // Capture during reverse too
        if (cameraAvailable && isMoving) {
          camera_fb_t * fb = esp_camera_fb_get();
          if (fb) {
            saveImageToSD(fb);
            esp_camera_fb_return(fb);
          }
        }
        
        // Allow stop during delay
        for (int j = 0; j < STOP_DURATION / 100 && isMoving; j++) {
          server.handleClient();
          delay(100);
        }
      }
      
      if (!isMoving) {
        Serial.println("\n*** STOPPED BY USER ***");
        break;
      }
      
      Serial.println("Returning to position...");
      for (int i = 0; i < DEMO_REVERSE_STOPS && isMoving; i++) {
        moveDistance(true);
        for (int j = 0; j < 10 && isMoving; j++) {  // 1 second = 10 x 100ms
          server.handleClient();
          delay(100);
        }
      }
    }
  }
  
  if (!isMoving) {
    Serial.println("\n=== SEQUENCE STOPPED ===");
  } else {
    Serial.println("\nReturning to start...");
    delay(2000);
    
    for (int i = 0; i < TOTAL_STOPS && isMoving; i++) {
      Serial.printf("Return %d/%d\n", i+1, TOTAL_STOPS);
      moveDistance(false);
      
      // Allow stop during return
      for (int j = 0; j < 10 && isMoving; j++) {
        server.handleClient();
        delay(100);
      }
    }
    
    Serial.println("\n=== SEQUENCE COMPLETE ===");
    Serial.printf("Total images saved: %d\n", imageCounter);
    Serial.println("Remove SD card to view images on computer\n");
    
    sequenceComplete = true;
  }
  
  isMoving = false;
  currentStop = 0;
}

// ===== WEB - ULTRA LIGHTWEIGHT =====

void handleRoot() {
  // Minimal HTML - no fancy styling, loads instantly
  String html = "<!DOCTYPE html><html><head><title>Herby</title>"
    "<meta name='viewport' content='width=device-width'>"
    "<style>"
    "body{font-family:Arial;margin:20px;text-align:center}"
    "button{padding:20px;margin:5px;font-size:18px;border:none;border-radius:5px;color:#fff}"
    ".g{background:#27ae60}.b{background:#3498db}.r{background:#e74c3c}"
    "img{max-width:100%;margin-top:10px;border:2px solid #ccc}"
    ".info{background:#ecf0f1;padding:10px;margin:10px 0;border-radius:5px;font-size:14px}"
    "</style></head><body>"
    "<h2>HERBY INSPECTOR</h2>";
  
  // Status info
  html += "<div class='info'>";
  if (cameraAvailable) {
    html += "Camera: OK";
  } else {
    html += "Camera: <span style='color:#e74c3c'>Offline</span>";
  }
  html += " | ";
  if (sdCardAvailable) {
    html += "SD: OK (" + String(imageCounter) + " imgs)";
  } else {
    html += "SD: <span style='color:#f39c12'>None</span>";
  }
  html += "</div>";
  
  if (cameraAvailable) {
    html += "<p><button class='b' onclick='document.getElementById(\"im\").src=\"/img?\"+Date.now()'>CAPTURE</button></p>";
    html += "<img id='im' src='/img' width='320' height='240'>";
  } else {
    html += "<p style='color:#f39c12'>Camera Offline</p>";
  }
  
  html += "<p><strong>";
  if (isMoving) {
    html += "RUNNING " + String(currentStop) + "/" + String(TOTAL_STOPS);
  } else if (sequenceComplete) {
    html += "COMPLETE - " + String(imageCounter) + " saved";
  } else {
    html += "READY";
  }
  html += "</strong></p>";
  
  html += "<p><button class='b' onclick='fetch(\"/go\")'>START</button>"
    "<button class='r' onclick='fetch(\"/s\")'>STOP</button></p>";
  
  html += "<p><button class='g' onclick='fetch(\"/f\")'>FORWARD</button><br>"
    "<button class='g' onclick='fetch(\"/r\")'>REVERSE</button></p>";
  
  if (sdCardAvailable) {
    html += "<p style='font-size:11px;color:#7f8c8d'>Auto-saves: /herby/IMG_####.jpg</p>";
  }
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleImage() {
  if (!cameraAvailable) {
    server.send(503, "text/plain", "No camera");
    return;
  }
  
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Capture failed");
    return;
  }
  
  // Send image directly, no caching
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  
  esp_camera_fb_return(fb);
}

void handleForward() {
  if (!isMoving) {
    Serial.println("Manual: FORWARD");
    moveDistance(true);
    
    // Capture and save if available
    if (cameraAvailable) {
      camera_fb_t * fb = esp_camera_fb_get();
      if (fb) {
        saveImageToSD(fb);
        esp_camera_fb_return(fb);
      }
    }
    
    server.send(200, "text/plain", "OK");
  } else {
    server.send(200, "text/plain", "Busy");
  }
}

void handleReverse() {
  if (!isMoving) {
    Serial.println("Manual: REVERSE");
    moveDistance(false);
    
    // Capture and save if available
    if (cameraAvailable) {
      camera_fb_t * fb = esp_camera_fb_get();
      if (fb) {
        saveImageToSD(fb);
        esp_camera_fb_return(fb);
      }
    }
    
    server.send(200, "text/plain", "OK");
  } else {
    server.send(200, "text/plain", "Busy");
  }
}

void handleStop() {
  isMoving = false;
  stopMotor();
  Serial.println("STOP");
  server.send(200, "text/plain", "OK");
}

void handleStart() {
  if (!isMoving) {
    isMoving = true;
    sequenceComplete = false;
    currentStop = 0;
    Serial.println("START SEQUENCE");
    server.send(200, "text/plain", "OK");
  } else {
    server.send(200, "text/plain", "Running");
  }
}
