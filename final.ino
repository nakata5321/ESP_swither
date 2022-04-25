#include <Servo.h>
#include <ESP_EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>

//login for WI-FI
char wifi_ssid[20]; 
char wifi_password[20];

//MQTT server
char mqtt_server[20];
long mqtt_port = 1883;

const char* AIO_USERNAME = "";
const char* AIO_KEY = "";


/* Put your SSID & Password */
const char* ssid = "NodeMCU-jf13uzaeum";  // Enter SSID here


/* Put IP Address details */
IPAddress local_ip(192,168,1,2);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WiFiClient espClient;
PubSubClient client(espClient);
Servo servo;
ESP8266WebServer server(80);


void handle_OnConnect();
void handle_submit();
void handle_NotFound();
void click_on();
void click_off();
void MQTTcallback(char* topic, byte* payload, unsigned int length);



void setup() {
  Serial.begin(9600);
  
  delay(5000);
  EEPROM.begin(100);  //Initialize EEPROM

  int check;
  EEPROM.get(0, check);
  if(check !=0){
    Serial.println("read EEPROM");
    EEPROM.get(4, wifi_ssid);
    Serial.print("wi-fi ssid is: ");
    Serial.println(wifi_ssid);
    
    EEPROM.get(24, wifi_password);
    Serial.print("wi-fi password is: ");
    Serial.println(wifi_password);
    
    EEPROM.get(44, mqtt_server);
    Serial.print("mqtt_server is: ");
    Serial.println(mqtt_server);

    EEPROM.get(64, mqtt_port);
    
  }
  else{
    Serial.println("EEPROM empty");
  }
      

  
  WiFi.begin(wifi_ssid, wifi_password);
  
  for (int i = 0; i < 10; i++){
    if(WiFi.isConnected()){
      Serial.print("Connected to WiFi :");
      Serial.println(WiFi.SSID());
      break;
    }
    else{
      Serial.println("Connecting to WiFi..");
      delay(1000);
    }
  }
  if (WiFi.isConnected()){
    Serial.print("Connected to WiFi :");
    Serial.println(WiFi.SSID());
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(MQTTcallback);
    while (!client.connected()){
      Serial.println("Connecting to MQTT...");
      if (client.connect("ESP8266kfc", AIO_USERNAME, AIO_KEY)){
        Serial.println("connected");
      }
      else
      {
        Serial.print("failed with state ");
        Serial.println(client.state());
        delay(2000);
      }
    }  
  client.subscribe("e6lmdiflxj");
  }
  else{
    WiFi.disconnect(true);
    WiFi.softAPConfig(local_ip, gateway, subnet);
    delay(100);
    WiFi.softAP(ssid);
  
    server.on("/", handle_OnConnect);
    server.on("/submit", handle_submit);
    server.onNotFound(handle_NotFound);
    server.begin();
    Serial.println("local HTTP server started");
  }
}




void loop() {
  if(WiFi.isConnected()){
    client.loop();
  }
  else{
    server.handleClient(); 
  }
}

const char index_html[] = R"rawliteral(
<!DOCTYPE html> <html>
<head></head>
<body width="100vw" height="100vh">
<h1>SAMP switch settings</h1>
<h4><p>Device UID: e6lmdiflxj</p>
<p> Use it to add a new device in mobile app</p>
</h4>

<form action="/submit" method="get">
    <label for="wifi_login">Insert your Wi-Fi SSID:</label>
    <br><input name="wifi_login" id="wifi_login">
  <br>
    <label for="wifi_password">Insert your Wi-Fi password:</label>
    <br>
    <input name="wifi_password" id="wifi_password">
  <br>
    <label for="broker_id">Insert your mqtt broker ip:</label>
    <br>
    <input name="broker_id" id="broker_id">
  <br>
    <label for="broker_port">Insert your mqtt broker port:</label>
    <br>
    <input name="broker_port" id="broker_port" value="1883">
    <br>
    <p>When you are done filling in all forms press restart button.</p> 
    <input type="submit" value="restart">
</form>

</body>
</html>)rawliteral";


void handle_OnConnect() {
  Serial.println("Connect to /");
  server.send(200, "text/html", index_html); 
}

void handle_submit() {
  EEPROM.put(0, 1);
  String str;
  
  //List all parameters
  for (uint8_t i = 0; i < server.args(); i++) {
    
    if (server.argName(i) == "wifi_password"){
      Serial.println("argName is wifi_password");
      str = server.arg(i);
      Serial.print("arg is");
      Serial.println(str);
      
      str.toCharArray(wifi_password , 20);
      EEPROM.put(24, wifi_password);
    }
    
    if (server.argName(i) == "wifi_login"){
      Serial.println("argName is wifi_login");
      str = server.arg(i);
      Serial.print("arg is");
      Serial.println(str);

      str.toCharArray(wifi_ssid , 20);
      EEPROM.put(4, wifi_ssid);
    }

    if (server.argName(i) == "broker_id"){
      Serial.println("argName is mqtt_server");
      str = server.arg(i);
      Serial.print("arg is");
      Serial.println(str);

      str.toCharArray(mqtt_server , 20);
      EEPROM.put(44, mqtt_server);
    }

    if (server.argName(i) == "broker_port"){
      Serial.println("argName is mqtt_port");
      str = server.arg(i);
      Serial.print("arg is");
      Serial.println(str);

      EEPROM.put(64, str.toInt()); 
    }
    
  }

  EEPROM.commit();
  delay(2000);
  server.send(200, "text/plain", "Restarting");
  delay(2000);
  ESP.restart();
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void MQTTcallback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message received in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  String message;
  for (int i = 0; i < length; i++) 
  {
    message = message + (char)payload[i];
  }
  Serial.print(message);
  if (message == "on") 
  { 
    click_on();
  }
  else if (message == "off") 
  {
    click_off();
  }
  Serial.println();
  Serial.println("-----------------------");
}

void click_on(){

  servo.attach(13);
  delay(1000);
  servo.write(140); //ставим вал под 90
  delay(5000); //ждем 2 секунды
  servo.detach();
  delay(1000); 
}

void click_off(){

  servo.attach(13);
  delay(1000);
  servo.write(0); //ставим вал под 0
  delay(5000); //ждем 2 секунды
  servo.detach();
  delay(1000); 
}
