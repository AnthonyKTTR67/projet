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
  if (client) {                                   // Vérifie si un client est connecté au serveur
    Serial.println("Nouvelle connexion HTTP.");   // Affiche un message dans la console série pour indiquer qu'une nouvelle connexion HTTP a été établie.
    String request = "";                          // Initialise une chaîne de caractères vide pour stocker la requête HTTP reçue du client.
    while (client.connected()) {                  // Boucle tant que le client reste connecté au serveur.
      if (client.available()) {                   // Vérifie si des données sont disponibles à lire dans le flux du client.
        char c = client.read();                   // Lit le prochain caractère envoyé par le client
        request += c;                             // Ajoute le caractère lu à la chaîne `request` pour construire la requête complète

        // Fin de la requête HTTP
        if (c == '\n' && request.length() == 1) {
          // Réponse HTTP
          client.println("HTTP/1.1 200 OK");          // Envoie le statut HTTP 200 OK pour indiquer que la requête a été traitée avec succès
          client.println("Content-Type: text/html");  // Spécifie que le contenu de la réponse sera en HTML
          client.println("Connection: close");        // Indique que la connexion sera fermée après la réponse
          client.println();                           // Ligne vide nécessaire pour séparer les en-têtes HTTP du corps de la réponse
          client.println("<!DOCTYPE HTML>");          // Déclare le type de document HTML5
          client.println("<html>");                   // Début du document HTML
          client.println("<h1>Capteur DHT11</h1>");   // Ajoute un titre principal pour afficher "Capteur DHT11"
          client.println("<p>Température actuelle : " + String(lastTemperature) + " °C</p>");
          client.println("</html>");                  // Fin du document HTML
          break;
        }
      }
    }
    delay(1);
    client.stop(); // Terminer la connexion avec le client
    Serial.println("Client déconnecté.");
  }
}

void reconnectMQTT() {                  // Fonction pour tenter de reconnecter le client MQTT si la connexion est perdue
  while (!clientMQTT.connected()) {     // Boucle tant que le client MQTT n'est pas connecté
    Serial.print("Tentative de connexion MQTT..."); // Affiche dans le moniteur série un message indiquant qu'une tentative de connexion MQTT est en cours
    
    // Créer un identifiant unique pour le client
    String clientId = "arduinoClient-";   // Initialiser l'identifiant du client MQTT avec un préfixe "arduinoClient-"
    clientId += String(random(0xffff), HEX);  // Ajoute une valeur hexadécimal aléatoire pour garantir que chaque identifiant est unique

    if (clientMQTT.connect(clientId.c_str())) {
      Serial.println("Connecté au serveur MQTT.");     // Si la connexion réussit, affiche un message indiquant que la connexion est établie.
    } else {
      Serial.print("Échec de la connexion, état : ");   // Affiche un message d'erreur indiquant que la connexion n'a pas pu être établie.
      Serial.println(clientMQTT.state());               // Affiche l'état ou le code d'erreur renvoyé par le client MQTT pour comprendre la cause de l'échec
      delay(5000); // Attente avant de réessayer
    }
  }
}
