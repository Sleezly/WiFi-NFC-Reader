#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>

// LEDs
const short int BUILTIN_LED1 = 2;  //GPIO2
const short int BUILTIN_LED2 = 16; //GPIO16

// Buzzer
const short int BUZZER_PIN = 5; //GPIO5

// WIFI
const char* ssid     = "<secret>"; // WIFI SSDI Network Name
const char* password = "<secret>"; // WIFI password
const char* host = "hassio.local"; // deafult setup is for HomeAssistant. If modifying, exclude the 'http://' portion.

// RC522
#define RST_PIN         2          // Configurable, see typical pin layout from HappyBubbles NFC
#define SS_PIN          15         // Configurable, see typical pin layout from HappyBubbles NFC

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup() 
{
  Serial.begin(74880);
  delay(100);

  pinMode(BUILTIN_LED1, OUTPUT); // Initialize the BUILTIN_LED1 pin as an output
  pinMode(BUILTIN_LED2, OUTPUT); // Initialize the BUILTIN_LED2 pin as an output

  digitalWrite(BUILTIN_LED1, LOW); // Turn the LED ON by making the voltage LOW
  digitalWrite(BUILTIN_LED2, HIGH); // Turn the LED off by making the voltage HIGH

  // Init the RC522
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
    
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  
  delay(500);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); 
}

void loop()
{
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  
  unsigned long uid = getID();
  if (uid == -1)
  {
    return;
  }

  // Turn the LED on
  digitalWrite(BUILTIN_LED2, LOW); 
  
  WiFiClient client;
  const int httpPort = 8123;
  if (!client.connect(host, httpPort))
  {
    Serial.println("connection failed");
    return;
  }

  Serial.print("RFID Detected with UID: ");
  Serial.println(uid);

  String jsonData = "{\"entity_id\": \"input_text.rfid_uid\", \"value\": \"" + String(uid) + "\"}";

  Serial.println("Requesting POST.");
  
  // Send request to the server:
  client.println("POST /api/services/input_text/set_value?api_password=<secret> HTTP/1.1");
  client.println("Host: hassio.local");
  client.println("Accept: */*");
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonData.length());
  client.println();
  client.print(jsonData);

  // Play an audible tone
  tone(BUZZER_PIN, 4000);
  delay(50);
  noTone(BUZZER_PIN);

  // Wait for the client to receive the message
  delay(500);
  
  if (client.connected())
  { 
    Serial.println("Disconnecting from server");
    client.stop();  // DISCONNECT FROM THE SERVER
  }
  
  Serial.println();
  Serial.println("Closing connection");

  // Turn the LED off
  digitalWrite(BUILTIN_LED2, HIGH); 

  // Disallow messages to be sent too frequently back-to-back
  delay(1000);
}

//
// mfrc522.PICC_IsNewCardPresent() should be checked before 
// @return the card UID
//
unsigned long getID()
{
  if (!mfrc522.PICC_ReadCardSerial())
  {
    Serial.println("No PICC_ReadCardSerial()");
    
    // Since a PICC placed get Serial and continue
    return -1;
  }
  
  unsigned long hex_num;
  hex_num =  mfrc522.uid.uidByte[0] << 24;
  hex_num += mfrc522.uid.uidByte[1] << 16;
  hex_num += mfrc522.uid.uidByte[2] <<  8;
  hex_num += mfrc522.uid.uidByte[3];
  mfrc522.PICC_HaltA(); // Stop reading
  return hex_num;
}
