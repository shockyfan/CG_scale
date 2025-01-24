// ESPNow
#ifdef ESP32
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>  // Nur für esp_wifi_set_channel()
esp_now_peer_info_t slave;
#else
#include <ESP8266WiFi.h>
#include <espnow.h>
uint8_t slave_peer_addr[6];  // MAC-Adresse für ESP8266
#endif

// #define ESPNOW_DEBUG true;

#define CHANNEL 1
#define PRINTSCANRESULTS 0
#define DELETEBEFOREPAIR 0

typedef struct struct_message {
  float U_Lipo;
  int Bat_Type;
  float Model_Weight;
  float Model_CG;
  float Model_CG_Trans;
  unsigned char type;  // type 1 = angle measurement; type 2 = cg
} struct_ESPNowMessage;

struct_ESPNowMessage ESPNowData;

/*---------------------------------------------------------------------------------------------------------*/
// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() != 0) {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
    return;
  }
  Serial.println("ESPNow Init Success");
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
}

/*---------------------------------------------------------------------------------------------------------*/
// Scan for slaves in AP mode
bool ScanForSlave() {
#ifdef ESP32
  int16_t scanResults = WiFi.scanNetworks(false, false, false, 300, CHANNEL);  // Nur ein Kanal scannen
#else
  int16_t scanResults = WiFi.scanNetworks(false, false, CHANNEL, NULL);
#endif

  bool slaveFound = false;

#ifdef ESP32
  memset(&slave, 0, sizeof(slave));
#endif

  if (scanResults == 0) {
    Serial.println("No WiFi devices in AP Mode found");
  } else {
#ifdef ESPNOW_DEBUG
    Serial.printf("Found %d devices\n", scanResults);
#endif

    for (int i = 0; i < scanResults; ++i) {
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);

      if (SSID.indexOf("Slave") == 0) {
#ifdef ESPNOW_DEBUG
        Serial.printf("Found Slave: %s [%s] (%d)\n", SSID.c_str(), BSSIDstr.c_str(), RSSI);
#endif

        int mac[6];
        if (6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])) {
#ifdef ESP32
          for (int ii = 0; ii < 6; ++ii) {
            slave.peer_addr[ii] = (uint8_t)mac[ii];
          }
          slave.channel = CHANNEL;
          slave.encrypt = 0;
#else
          for (int ii = 0; ii < 6; ++ii) {
            slave_peer_addr[ii] = (uint8_t)mac[ii];
          }
#endif
        }
        slaveFound = true;
        break;
      }
    }
  }

  WiFi.scanDelete();

#ifdef ESPNOW_DEBUG
  if (slaveFound) {
    Serial.println("Slave Found, processing...");
  } else {
    Serial.println("Slave Not Found, trying again.");
  }
#endif

  return slaveFound;
}

/*---------------------------------------------------------------------------------------------------------*/
void deletePeer() {
#ifdef ESP32
  esp_err_t delStatus = esp_now_del_peer(slave.peer_addr);
  if (delStatus == ESP_OK)
#else
  uint8_t delStatus = esp_now_del_peer(slave_peer_addr);
  if (delStatus == 0)
#endif
  {
    Serial.println("Success");
  } else {
    Serial.println("Failed to delete peer");
  }
}

bool manageSlave() {
#ifdef ESP32
  if (slave.channel == CHANNEL) {
    if (DELETEBEFOREPAIR) {
      deletePeer();
    }
    bool exists = esp_now_is_peer_exist(slave.peer_addr);
    if (exists) {
#ifdef ESPNOW_DEBUG
      Serial.println("Already Paired");
#endif
      return true;
    } else {
      esp_err_t addStatus = esp_now_add_peer(&slave);
      return addStatus == ESP_OK;
    }
  }
#else
  if (DELETEBEFOREPAIR) {
    deletePeer();
  }
  bool exists = esp_now_is_peer_exist(slave_peer_addr);
  if (exists) {
#ifdef ESPNOW_DEBUG
    Serial.println("Already Paired");
#endif
    return true;
  } else {
    bool addStatus = esp_now_add_peer(slave_peer_addr, ESP_NOW_ROLE_SLAVE, CHANNEL, NULL, 0);
    // Serial.print("AddStatus: ");
    // Serial.println(addStatus);
    return addStatus == 0;
  }
#endif

  Serial.println("No Slave found to process");
  return false;
}

void sendData() {
  const uint8_t *peer_addr;

#ifdef ESP32
  peer_addr = slave.peer_addr;  // ESP32: Verwenden von esp_now_peer_info_t
  esp_err_t result;             // ESP32 verwendet esp_err_t als Rückgabewert
#else
  peer_addr = slave_peer_addr;  // ESP8266: Verwenden eines einfachen Arrays für die MAC-Adresse
  uint8_t result;               // ESP8266 verwendet uint8_t als Rückgabewert
#endif

#ifdef ESPNOW_DEBUG
  Serial.printf("Sending: %f (size: %d)\n", ESPNowData.Model_Weight, sizeof(ESPNowData.Model_Weight));
#endif

// Fehlerbehandlung für ESPNow
#ifdef ESP32
  result = esp_now_send(peer_addr, (uint8_t *)&ESPNowData, sizeof(ESPNowData));  // ESP32 API
#else
  result = esp_now_send((u8 *)peer_addr, (uint8_t *)&ESPNowData, sizeof(ESPNowData));  // ESP8266 API
#endif

#ifdef ESPNOW_DEBUG
  Serial.print("Send Status: ");
#endif

// Fehlerbehandlung für beide Plattformen
#ifdef ESP32
  if (result == ESP_OK) {
#else
  if (result == 0) {  // ESP8266 verwendet 0 für Erfolg
#endif
#ifdef ESPNOW_DEBUG
    Serial.println("Success");
#endif
  } else {
// Fehlerbehandlung für beide Plattformen
#if defined(ESP32)
    switch (result) {
      case ESP_ERR_ESPNOW_NOT_INIT:
        Serial.println("ESPNOW not initialized.");
        break;
      case ESP_ERR_ESPNOW_ARG:
        Serial.println("Invalid argument.");
        break;
      case ESP_ERR_ESPNOW_INTERNAL:
        Serial.println("Internal error.");
        break;
      case ESP_ERR_ESPNOW_NO_MEM:
        Serial.println("No memory.");
        break;
      case ESP_ERR_ESPNOW_NOT_FOUND:
        Serial.println("Peer not found.");
        break;
      default:
        Serial.println("Unknown ESP32 error.");
    }
#elif defined(ESP8266)
    switch (result) {
      case 1:  // Fehler: Argument ungültig
        Serial.println("Invalid argument.");
        break;
      case 2:  // Fehler: Interner Fehler
        Serial.println("Internal error.");
        break;
      case 3:  // Fehler: Kein Speicher
        Serial.println("No memory.");
        break;
      case 4:  // Fehler: Peer nicht gefunden
        Serial.println("Peer not found.");
        break;
      default:
        Serial.print("Unknown ESP8266 error: ");
        Serial.println(result);
    }
#endif
  }
}

/*---------------------------------------------------------------------------------------------------------*/
// callback when data is sent from Master to Slave
#ifdef ESP32
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
#else
void OnDataSent(uint8_t *mac_addr, uint8_t status)
#endif
{
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2],
           mac_addr[3], mac_addr[4], mac_addr[5]);

#ifdef ESPNOW_DEBUG
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
#ifdef ESP32
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
#else
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == 0 ? "Delivery Success" : "Delivery Fail");
#endif
#endif
}
/*---------------------------------------------------------------------------------------------------------*/
// void setup() {
//   Serial.begin(115200);

//   WiFi.mode(WIFI_STA);
//   InitESPNow();

// #ifdef ESP32
//   esp_now_register_send_cb(OnDataSent);
// #else
//   esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
//   esp_now_register_send_cb(OnDataSent);
// #endif

//   ScanForSlave();
//   manageSlave();
// }

// void loop() {
//   sendData();
//   delay(1000);
// }
