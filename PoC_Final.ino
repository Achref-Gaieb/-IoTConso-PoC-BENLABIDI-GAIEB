#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// Remplacer avec votre identifiant WiFi et mot de passe
#define WLAN_SSID       "Wifi"       
#define WLAN_PASS       "MotDePasse"

// Configuration pour se connecter au serveur Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883  // Port standard non sécurisé
#define AIO_USERNAME    "Username"
#define AIO_KEY         "Key"

// Initialisation du client WiFi et MQTT
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Initialisation des feeds pour publier température et humidité
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");

// Initialisation du feed pour souscrire à un bouton de contrôle
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");

// Fonction pour la connexion MQTT
void MQTT_connect();

// Pression atmosphérique au niveau de la mer
#define SEALEVELPRESSURE_HPA (1013.25)

// Initialisation du capteur BME680 pour mesurer la température, l'humidité, etc.
Adafruit_BME680 bme;

// Paramètres de l'écran LCD
int lcdColumns = 16;
int lcdRows = 2;

// Initialisation de l'écran LCD I2C
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

void setup() {
  // Initialisation du port série pour le débogage
  Serial.begin(115200);

  // Initialisation de l'écran LCD
  lcd.init();
  lcd.backlight();

  // Vérifier si le capteur BME680 est présent
  if (!bme.begin()) {
    Serial.println("Échec de l'initialisation du BME680");
    while (1);
  }

  // Configuration des paramètres du capteur BME680
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // Température du chauffage du gaz à 320°C pendant 150 ms

  // Connexion au WiFi
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connecté au WiFi!");
}

void loop() {
  // Se connecter au serveur MQTT
  MQTT_connect();

  // Lire les données du capteur BME680
  if (!bme.performReading()) {
    Serial.println("Échec de lecture du capteur BME680");
    return;
  }

  // Afficher la température et l'humidité sur le moniteur série
  Serial.print("Température = ");
  Serial.print(bme.temperature);
  Serial.println(" °C");

  Serial.print("Humidité = ");
  Serial.print(bme.humidity);
  Serial.println(" %");

  // Afficher les données sur l'écran LCD
  lcd.setCursor(0, 0);
  lcd.print(bme.temperature);
  lcd.print(" *C");

  lcd.setCursor(0, 1);
  lcd.print(bme.humidity);
  lcd.print(" %");

  // Publier la température sur le feed MQTT
  Serial.print("Envoi de la température: ");
  if (!temperature.publish(bme.temperature)) {
    Serial.println("Échec");
  } else {
    Serial.println("OK!");
  }

  // Publier l'humidité sur le feed MQTT
  Serial.print("Envoi de l'humidité: ");
  if (!humidity.publish(bme.humidity)) {
    Serial.println("Échec");
  } else {
    Serial.println("OK!");
  }

  // Alerte si la température ou l'humidité est trop élevée
  if (bme.temperature > 29)
    Serial.println("Température trop élevée !");
  if (bme.humidity > 60)
    Serial.println("Humidité trop élevée !");

  delay(2000); // Attendre 2 secondes avant la prochaine lecture
}

// Fonction pour se connecter au serveur MQTT
void MQTT_connect() {
  int8_t ret;

  // Ne rien faire si déjà connecté
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connexion à MQTT... ");
  uint8_t retries = 3;

  while ((ret = mqtt.connect()) != 0) { // 0 signifie connecté
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Nouvelle tentative de connexion MQTT dans 5 secondes...");
    mqtt.disconnect();
    delay(5000);  // Attendre 5 secondes avant de réessayer
    retries--;
    
    if (retries == 0) {
      // Redémarrer si échec de connexion après plusieurs tentatives
      while (1);
    }
  }

  Serial.println("Connecté à MQTT!");
}