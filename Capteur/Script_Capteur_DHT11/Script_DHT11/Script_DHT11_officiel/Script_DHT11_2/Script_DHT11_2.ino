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
IPAddress webServerIP(10, 129, 251, 1);              // Adresse IP du serveur web

// Configuration du client Ethernet
EthernetClient ethernetClient; // Client Ethernet pour se connecter à MQTT

// Configuration du client MQTT
IPAddress mqttServer(10, 129, 251, 253); // Adresse IP du serveur MQTT
const int mqttPort = 1883;               // Port par défaut du serveur MQTT
PubSubClient clientMQTT(ethernetClient); // Initialisation du client MQTT avec EthernetClient

// Configuration du serveur web
EthernetServer server(1904);             // Serveur web sur le port 1904

// Dernière température mesurée
float lastTemperature = 0.0;

// Variables pour contrôler la fréquence des mises à jour MQTT
unsigned long lastMQTTSendTime = 0;  // Temps de la dernière publication MQTT
const unsigned long mqttInterval = 5000; // Intervalle entre chaque publication (5 secondes)

void setup() {
  Serial.begin(9600);            // Initialisation de la communication série pour le débogage
  Serial.println("Initialisation...");
  dht.begin();                   // Initialisation du capteur DHT11
  Serial.println("Capteur DHT initialisé.");

  // Initialisation de l’Ethernet Shield avec l'adresse MAC et l'adresse IP
  Ethernet.begin(mac, ip);
  delay(1000);

  // Afficher l'adresse IP de l'Arduino
  server.begin();
  Serial.print("Serveur web démarré sur IP : ");
  Serial.println(Ethernet.localIP());   // Affichage de l'adresse IP du serveur dans le moniteur série

  // Configurer le client MQTT
  clientMQTT.setServer(mqttServer, mqttPort);
}

void loop() {
  // Assurez-vous que la connexion MQTT est active
  if (!clientMQTT.connected()) {
    reconnectMQTT(); // Reconnexion au serveur MQTT si nécessaire
  }
  clientMQTT.loop(); // Maintenir la connexion avec le serveur MQTT

  // Vérifiez si le temps d'envoyer les données au MQTT est atteint
  if (millis() - lastMQTTSendTime > mqttInterval) {
    sendTemperatureToMQTT(); // Envoyer la température au serveur MQTT
    lastMQTTSendTime = millis();
  }
}

// Fonction pour envoyer la température au serveur MQTT
void sendTemperatureToMQTT() {
  // Lecture de la température depuis le capteur DHT11
  float temp = dht.readTemperature();

  if (!isnan(temp)) {
    lastTemperature = temp; // Stocker la dernière température valide

    // Convertir la température en chaîne
    char tempStr[8];
    dtostrf(temp, 1, 2, tempStr);

    // Publier la température sur le serveur MQTT
    if (clientMQTT.publish("temp", tempStr)) {
      Serial.println("Température de la salle serveur envoyée à MQTT : " + String(temp) + " °C");
    } else {
      Serial.println("Erreur lors de l'envoi de la température.");
    }
  } else {
    Serial.println("Échec de la lecture du capteur DHT11 !");
  }
}

// Fonction pour reconnecter le client MQTT si déconnecté
void reconnectMQTT() {
  while (!clientMQTT.connected()) {
    Serial.print("Tentative de connexion MQTT...");
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

// Fonction pour gérer les requêtes HTTP et afficher une interface web
void handleWebServer() {
  EthernetClient client = server.available(); // Vérifie si un client est connecté
  if (client) {
    Serial.println("Nouvelle connexion HTTP.");
    String request = "";
    
    // Lire la requête du client
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;

        // Détecte la fin de la requête HTTP
        if (c == '\n' && request.endsWith("\r\n\r\n")) {
          // Prépare la réponse HTTP avec une interface web simple
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          
          // Contenu HTML
          client.println("<!DOCTYPE html>");
          client.println("<html>");
          client.println("<head>");
          client.println("<title>Température Salle Serveur</title>");
          client.println("<style>");
          client.println("body { font-family: Arial, sans-serif; text-align: center; margin: 50px; }");
          client.println(".temperature { font-size: 48px; color: #333; }");
          client.println("</style>");
          client.println("</head>");
          client.println("<body>");
          client.println("<h1>Température Salle Serveur</h1>");
          client.println("<p class='temperature'>Température actuelle : 24°C</p>");


          break; // Arrête la lecture une fois que la réponse est envoyée
        }
      }
    }
    delay(1); // Pause pour s'assurer que le client reçoit toutes les données
    client.stop(); // Fermer la connexion avec le client
    Serial.println("Client déconnecté.");
  }
}
