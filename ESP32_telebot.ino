//________________________CONFIGURE__________________________________
#define BOTtoken ""
#define LIGHT_UP_KEY ""
#define TRIGGER_KEY "" 
#define DUMP_ID ""


String your_id = "";
String my_id = ""; 
int ISRpin =15;
//_________________________COLOUR_________________________________
#define R_RECIEVE 170
#define G_RECIEVE 255
#define B_RECIEVE 0
#define R_CHECK 12
#define G_CHECK 20
#define B_CHECK 13
#define R_START 0
#define G_START 255
#define B_START 250
#define R_SEND 170
#define G_SEND 0
#define B_SEND 255
//___________________________________________________________________
#define redPin 5
#define groundPin 18
#define greenPin 19
#define bluePin 21
#define redChannel 0
#define greenChannel 1
#define blueChannel 2
#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h>
#include <UniversalTelegramBot.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiMulti.h>
#define GMT 8
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

bool verify_is_on = false;
int Bot_mtbs = 1000; //mean time between scan messages
long Bot_lasttime = 0;   //last time messages' scan has been done
bool incoming_on = false;

template< typename T, size_t N > size_t ArraySize (T (&) [N]){ return N; }
WiFiMulti wifiMulti;

int red = 0;
int green = 0;
int blue = 0;

bool on_hold = false;

long light_lasttime = 0;

void sendMessage(String to, String msg) {
  ledcWrite(blueChannel, B_SEND);
  ledcWrite(redChannel, R_SEND);
  bot.sendMessage(to, msg, "");
}
void incomingMsg() {
   green = G_RECIEVE;
   red = R_RECIEVE;
   incoming_on = true;
}

/*
 * GREETINGM
 */
String greeting_text = "Greeting";
String greetingM_text = "Morning :)";
String greetingN_text = "Nights (:";
bool greeting_set_on = false;
bool greetingM_set_on = false;
bool greetingN_set_on = false;

void greeting() {
  Serial.println("greeting");
  sendMessage(DUMP_ID, TRIGGER_KEY);
  timeClient.forceUpdate();
  formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  if (timeStamp > "05:30:00" && timeStamp < "10:30:00") {
    sendMessage(your_id, greetingM_text);
        if (verify_is_on && my_id != "") {
          sendMessage(my_id, "You sent: " + greetingM_text);  
      }
  } else if (timeStamp > "00:00:00" && timeStamp <= "05:30:00") {
     sendMessage(your_id, greetingN_text);
        if (verify_is_on && my_id != "") {
          sendMessage(my_id, "You sent: " + greetingN_text);  
      }
  } else {
    sendMessage(your_id, greeting_text);
        if (verify_is_on && my_id != "") {
          sendMessage(my_id, "You sent: " + greeting_text);  
      }  
  }
}

/*
 * ANNOY
 */
String annoy_text = "hey";
bool annoy_set_on = false;

void annoy() {
  Serial.println("annoy");
  sendMessage(DUMP_ID, TRIGGER_KEY);
  sendMessage(your_id, annoy_text);
      if (verify_is_on && my_id != "") {
        sendMessage(my_id, "You sent: " + annoy_text);  
    }
}

/*
 * PARSER
 */
String parser(String input) {
    String ack = "Sorry, that command is not recognised, use /help for available commands";
    Serial.println(input);
    if (input == "/verifyon") {
      verify_is_on = true;
      ack = "Messages you sent with RelationLink will now be forwarded to you";
    } else if (input == "/verifyoff") {
      verify_is_on = false;
      ack = "Messages you sent with RelationLink will not be forwarded to you";
    } else if (annoy_set_on == true) {
      annoy_text = input;
      ack = "annoy_text will now be set as [" + input + "]";
      annoy_set_on = false;
    } else if (input == "/setannoy") {
      annoy_set_on = true;
      ack = "Send your new annoy_text to me!";
    } else if (greeting_set_on == true) {
      greeting_text = input;
      ack = "greeting_text will now be set as [" + input + "]";
      greeting_set_on = false;
    } else if (input == "/setgreet") {
      greeting_set_on = true;
      ack = "Send your new greeting_text to me!";
    } else if (greetingM_set_on == true) {
      greetingM_text = input;
      ack = "morning greeting will now be set as [" + input + "]";
      greetingM_set_on = false;
    } else if (input == "/setmorngreet") {
      greetingM_set_on = true;
      ack = "Send your new greeting_text to me!";
    } else if (greetingN_set_on == true) {
      greetingN_text = input;
      ack = "Night greeting will now be set as [" + input + "]";
      greetingN_set_on = false;
    } else if (input == "/setnightgreet") {
      greetingN_set_on = true;
      ack = "Send your new greeting_text to me!";
    }
    return ack;
}

volatile long last_check = 0;
void recieve_msg() {
    ledcWrite(redChannel, R_CHECK);
    ledcWrite(greenChannel, G_CHECK);
    ledcWrite(blueChannel, B_CHECK);
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1); 
    while(numNewMessages) {
      Serial.println("got response");
      for (int i=0; i<numNewMessages; i++) {
        if(bot.messages[i].type == "channel_post" && bot.messages[i].text == LIGHT_UP_KEY && bot.messages[i].chat_id == DUMP_ID) {
          incomingMsg();
        } else if (my_id == bot.messages[i].chat_id) { //sends back conformation of command
          String reply = parser(bot.messages[i].text);
          sendMessage(bot.messages[i].chat_id, reply);
          Serial.println(reply);
        }
      }
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
}

/*
 * ISR
 */
static volatile int annoy_count = 0;
static volatile long appear_lasttime = 0;
static volatile long disappear_lasttime = 0;
static volatile int greeting_count = 0;
long reset_annoy = 0;

void IRAM_ATTR ISR() {
  incoming_on = false;
  last_check = millis();     
  on_hold = !digitalRead(ISRpin);
    if (digitalRead(ISRpin)) {
      if (millis() - disappear_lasttime > 250) {
        disappear_lasttime = millis(); 
        if (blue < 150 && annoy_count < 7) {
          annoy_count ++;
          reset_annoy = millis();
        } else {
          greeting_count ++;
          annoy_count = 0;
        }
      }
      red = 0; green = 0; blue = 0;
    } else {
      if (blue >= 150) {
        greeting_count ++;
        annoy_count = 0;
      }
      red = 255; green = 0; blue = 0;
    }
}

void setup() {
  Serial.begin(115200);
  pinMode(groundPin, OUTPUT);
  digitalWrite(groundPin, LOW);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(5, 0);
  ledcSetup(1, 5000, 8);
  ledcAttachPin(19, 1);
  ledcSetup(2, 5000, 8);
  ledcAttachPin(21, 2);
  delay(10);
  ledcWrite(greenChannel, 10);
  Serial.println("Connecting Wifi...");
  while(wifiMulti.run() != WL_CONNECTED) {
      ledcWrite(greenChannel, 0);
      delay(300);
      Serial.print(".");
      ledcWrite(greenChannel, 10);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("ready");
  sendMessage(my_id, "RelationLink is ready!");
  pinMode(ISRpin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ISRpin), ISR, CHANGE);
  timeClient.begin();
  timeClient.setTimeOffset(GMT * 3600);
  recieve_msg();
  ledcWrite(redChannel, R_START);
  ledcWrite(blueChannel, B_START);
  ledcWrite(greenChannel, G_START);
  delay(1200);
  last_check = millis() - 10000;
  annoy_count = 0;
  greeting_count = 0;
}
void loop() {
  ledcWrite(redChannel, red);
  ledcWrite(greenChannel, green);
  ledcWrite(blueChannel, blue);
  if (on_hold && blue < 252 && millis() - light_lasttime > 25) {
    red -=2;
    blue +=2;
    light_lasttime = millis();
  }
  while (greeting_count > 0) {
    greeting();
    greeting_count--;
  }
  while (annoy_count > 1) {
    annoy();
    annoy_count-=2;
  }
  if (millis() - reset_annoy > 1500) {
    if (annoy_count % 2 == 1 && annoy_count > 0) {
      annoy_count--;
    }
    if (incoming_on) {
      ledcWrite(redChannel, 0);
      ledcWrite(greenChannel, 0);
      ledcWrite(blueChannel, 0);
      delay(500);
    }
    reset_annoy = millis();
  }
  if (millis() - last_check > 30000) {
    last_check = millis();
    if (annoy_count == 0 && greeting_count == 0 && on_hold == false) { 
      recieve_msg();
    }
  }
}
