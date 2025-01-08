#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h> // Inclure la bibliothèque MQTT

// Configuration du DHT11
#define DHTPIN 3                  // Définition de la broche de connexion du capteur DHT11
#define DHTTYPE DHT11             // Définition du type de capteur (DHT11)
DHT dht(DHTPIN, DHTTYPE);         // Initialisation du capteur DHT11

// Configuration du réseau Ethernet
byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x6F, 0x4D }; // Adresse MAC unique pour l’Ethernet Shield
IPAddress ip(10, 129, 251, 1);                       // Adresse IP de l’Arduino
IPAddress webServerIP(10, 129, 251, 253);            // Adresse IP du serveur web

// Configuration du client Ethernet
EthernetClient ethernetClient; // Client Ethernet pour se connecter à MQTT

// Configuration du client MQTT
IPAddress mqttServer(10, 129, 251, 253); // Adresse IP du serveur MQTT
const int mqttPort = 1883;  // Port par défaut du serveur MQTT
PubSubClient clientMQTT(ethernetClient); // Initialisation du client MQTT avec EthernetClient

// Configuration du serveur web
EthernetServer server(80); // Serveur web sur le port 80

// Dernière température mesurée
float lastTemperature = 0.0;

void setup() {
  Serial.begin(9600);            // Initialisation de la communication série pour le débogage
  Serial.println("Initialisation...");
  dht.begin();                   // Initialisation du capteur DHT11
  Serial.println("Capteur DHT initialisé.");

  // Initialisation de l’Ethernet Shield avec l'adresse MAC et l'adresse IP
  Ethernet.begin(mac, ip);
  delay(1000);

  // Afficher l'adresse IP de l'Arduino
  Serial.print("Adresse IP de l'Arduino : ");
  Serial.println(Ethernet.localIP());

  // Configurer le client MQTT
  clientMQTT.setServer(mqttServer, mqttPort);

  // Démarrer le serveur web
  server.begin();
  Serial.println("Serveur web démarré à l'adresse IP 10.129.251.253");
}

void loop() {
  if (!clientMQTT.connected()) {
    reconnectMQTT(); // Reconnexion au serveur MQTT si nécessaire
  }
  clientMQTT.loop(); // Maintenir la connexion avec le serveur MQTT

  // Lecture de la température depuis le capteur DHT11
  float temp = dht.readTemperature();

  if (!isnan(temp)) {
    lastTemperature = temp; // Stocker la dernière température valide

    // Convertir la température en chaîne
    char tempStr[8];
    dtostrf(temp, 1, 2, tempStr);

    // Publier la température sur le serveur MQTT
    if (clientMQTT.publish("temp", tempStr)) {
      Serial.println("Température envoyée à MQTT.");
    } else {
      Serial.println("Erreur lors de l'envoi de la température.");
    }
  } else {
    Serial.println("Échec de la lecture du capteur DHT11 !");
  }

  // Gérer les requêtes HTTP
  handleWebServer();

  delay(5000); // Attente de 5 secondes avant la prochaine lecture
}

void handleWebServer() {
  EthernetClient client = server.available(); // Vérifier les connexions entrantes
  if (client) {
    Serial.println("Nouvelle connexion HTTP.");
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;

        // Fin de la requête HTTP
        if (c == '\n' && request.length() == 1) {
          // Réponse HTTP
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<h1>Capteur DHT11</h1>");
          client.println("<p>Température actuelle : " + String(lastTemperature) + " °C</p>");
          client.println("</html>");
          break;
        }
      }
    }
    delay(1);
    client.stop(); // Terminer la connexion avec le client
    Serial.println("Client déconnecté.");
  }
}

void reconnectMQTT() {
  while (!clientMQTT.connected()) {
    Serial.print("Tentative de connexion MQTT...");
    
    // Créer un identifiant unique pour le client
    String clientId = "arduinoClient-";
    clientId += String(random(0xffff), HEX);

    if (clientMQTT.connect(clientId.c_str())) {
      Serial.println("Connecté au serveur MQTT.");
    } else {
      Serial.print("Échec de la connexion, état : ");
      Serial.println(clientMQTT.state());
      delay(5000); // Attente avant de réessayer
    }
  }
}
