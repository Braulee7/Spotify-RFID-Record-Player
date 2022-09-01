
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>;
#include <IRremote.h>

//certificate for https call to spotify
#define FingerPrint "4A:44:71:F7:6A:8D:D4:BD:54:E9:0E:3D:E8:6C:A6:E0:00:27:BA:D5"

//pin numbers for mfrc522 RFID sensor
//depending on circuit and board these
//pins could change
const int SS_pin = 10;
const int RST_pin = 9;

//credentials for wifi
const char ssid[] = "Network Name";
const char password[] = "Network Password";


//buffer for parsing the cards
const byte BUFFERSIZE = 176;

//pin number for the IR sensor
const int recv = 16;

//main class for all of the api calls
class Spotify {
  public: 
    //constructor, takes in your spotify api credentials
    //and a refresh token, all able to get from the 
    //spotify website
    Spotify(const String ID, const String secret, const String resfresh);

    //gets a new token, ensures no bad token errors
    int getToken();

    //gets the device id, depending on which device you want 
    //and number of devices this part of the code could/should
    //change
    int getDevice();

    //takes in a uri and plays the song/album
    int play(String context_uri);

    //for the IR remote functions
    int getFunc(decode_results code);

  private: 
    //currently playing song or album
    int currVolume;

    //gets the current state of the song / device
    //this allows us to properly change the volume
    String getPlaybackState();
    //media functions
    String skip();
    String previous();
    String pause();
    String setVolume(int percent);
    int getVolume();
    int getPosition();
    bool isPlaying();
    String currentlyPlaying();
    String Resume(String overURI, String trackURI, int pos);
    String addToQueue(String trackURI);
    String seekPos(int pos);
    

    //credentials for the api
    String clientID;
    String clientSecret;
    String deviceID;
    String refreshToken;
    String accessToken;
    const String base64Token =  "Your token from the spotify website"; 
  
};

//function declarations for the leds
void turnOffLeds();
void redLed();
void greenLed();
void blueLed();
void whiteLed();


//initiate the client globally
HTTPClient http;

//initialise the rfid sensor
MFRC522 rfid(SS_pin, RST_pin);


//set up for the IR remote
IRrecv irrecv (recv);
decode_results results;

//pin numbers for the leds
int red[] = {5, 4};
int green[] = {0, 2};
int blue[] = {15, 3};


void setup() {
  Serial.begin(9600);

  Serial.println("starting the program");
  
  //rfid initiation
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("checkpoint");
  

  //set up the receiver 
  irrecv.enableIRIn();

  //connect to wifi
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");

  //set the pin modes for the leds
  for (int i = 0; i < 2; i++)
  {
    pinMode(green[i], OUTPUT);
    pinMode(blue[i], OUTPUT);
    pinMode(red[i], OUTPUT); 
  }
}

void loop() {
  //set up Spotify instance
  Spotify main("API ID", "Client Secret from the dashboard", "Refresh Token used to get new access tokens");

  //check if there is a ir signal received
  if (irrecv.decode(&results))
  {
    //check if the signal corresponds to 
    //a function
    main.getFunc(results);
  }
  //continue to receive signals
  irrecv.resume();
  
  //reset if no new cards present
  if (!rfid.PICC_IsNewCardPresent()){
    return;
    
  }

  //verifiy
if (rfid.PICC_ReadCardSerial())
  {
    //parse the nfc chip to
    //get the uri
    byte dataBuffer[BUFFERSIZE];
    readNFCTagData(dataBuffer);
    rfid.PICC_HaltA();

    Serial.print("Read NFC tag: ");
    String context_uri = parseNFCTagData(dataBuffer);
    Serial.println("tag: " + context_uri);

    //get a new token and play the song
    main.getToken();
    main.play(context_uri);
   
  }
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// *  Functions for the spotify api below
// * * * * * * * * * * * * * * * * * * * * * * * *



//default constructor
Spotify:: Spotify(const String ID, const String secret, const String refresh)
{

  clientID = ID;
  clientSecret = secret;
  refreshToken = refresh;
  
}



int Spotify:: getToken(){

    //set up the body of the requets
    String body = "grant_type=refresh_token&refresh_token=" + refreshToken;

    //create request
    http.begin("https://accounts.spotify.com/api/token", String(FingerPrint)); 
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Authorization", "Basic " + base64Token);

    //send request with body
    int code = http.POST(body);
    
    
    if (code > 0){
      //parse through the JSON
      String payload = http.getString();

      char json[500];
      payload.replace(" ", "");
      payload.replace("\n", "");
      payload.trim();
      payload.toCharArray(json, 500);

      StaticJsonDocument<200> doc;
 
      deserializeJson(doc, json);

      //copy the access tokekn into the variable
      const char* token = doc["access_token"];
      accessToken = String(token);

      
    } else {//if failed terminate the client
      Serial.println("Error on http request");
      http.end();
      return code;
    }
}

int Spotify:: getDevice(){
    //set up request
    String authorization = "Bearer " + accessToken;
    http.begin("https://api.spotify.com/v1/me/player/devices", String(FingerPrint));
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", authorization);

    //send request
    int code = http.GET();

    //check if succeeded
    if (code > 0){
  
      String payload = http.getString();


      //parse json to get id
      char json[750];
      //prepare the string for the parsing
      payload.replace(" ", "");
      payload.replace("\n", "");
      payload.trim();
      payload.remove(0, 12);
      
      
      Serial.println(payload);

      int count = 0;
      //select the correct device
      for (int i = 0; i < payload.length(); i++)
      {
        if (payload.charAt(i) == ',')
        {
          ++count;
        }

        //check every individual device in the array
        //i only have a max of 5 devices in my spotify
        //so I check through each one until the type is 
        //cast audio which only my google home is and I 
        //return that as my device ID, if no Cast Audio
        //is found I default to nothing and no song plays
        if (count == 0 || count == 7 || count == 14 || count == 21)
        {
          String temp = payload;
          char arr[500];
          
          temp.remove(0, i);

          temp.toCharArray(arr, 500);

          StaticJsonDocument<200> stat;

          deserializeJson(stat, arr);

          //is this the correct device
          if (stat["type"] == "CastAudio")
          {
            const char* ID = stat["id"];

            deviceID = String(ID);
            return code;
          }

          
        }
      }

    } else {
      Serial.println("Error on HTTP Request");
      http.end();
    }
    return code;
}

int Spotify:: play(String context_uri){
  //get device ID
  int response = getDevice();

  if (response > 0){
    //set up context uri and authorization
    String body = "{\"context_uri\": \"" + context_uri + "\"}";
    String authorization = "Bearer " + accessToken;
    Serial.println(body);
    //set up request
    http.begin("https://api.spotify.com/v1/me/player/play?device_id=" + deviceID, String(FingerPrint));
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", authorization);

    int code = http.PUT(body);

    if (code > 0) {

      String payload = http.getString();

      Serial.println(payload);
    } else {
      Serial.println("Error");
      http.end();
    }
    return code;
  } else {
    Serial.println("Error on getting deviceID");
    http.end();
  }
}


String Spotify:: getPlaybackState()
{
  String authorization = "Bearer " + accessToken;

  //set up request
  http.begin("https://api.spotify.com/v1/me/player", String(FingerPrint));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", authorization);

  int code = http.GET();

  if (code > 0)
  {
    String payload = http.getString();

      payload.replace(" ", "");
      payload.replace("\n", "");
      payload.trim();

      return payload;
    
  } else {
    Serial.println("failure at the http request for playback state");
    http.end();
  }
}

bool Spotify:: isPlaying()
{
  String playback = getPlaybackState();

  //parse
  playback.remove(0, playback.length() - 18);

  //the second letter of the boolean object
  //is either an a or an r so we return a true
  //or false depending on the letter
  if (playback.charAt(14) == 'a') return false;
  else return true;

  Serial.println(playback);
}

String Spotify:: currentlyPlaying()
{
    String authorization = "Bearer " + accessToken;

    //set up request
    http.begin("https://api.spotify.com/v1/me/player/currently-playing", String(FingerPrint));
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", authorization);

    int code = http.GET();

    if (code > 0)
    {
      String payload = http.getString();

      //prepare for pasing
      payload.replace(" ", "");
      payload.replace("\n", "");
      payload.trim();

      //get the overlaying uri which is either
      //an artist or a playlist
      String overURI = payload;

      int count = 0;
      bool searching = true;
      int place = 0;

      for (int i = 0; i < overURI.length() && searching; i++)
      {
        if (overURI.charAt(i) == ':')
        {
          ++count;
        }

        if (count == 9)
        {
          searching = false;
          place = i;
        }
      }

      place += 2;

      overURI.remove(0, place);

      //remove the backend of the uri
      searching = true;
      count = 0;
      
      for (int i = 0; i < overURI.length() && searching; i++)
      {
        if (overURI.charAt(i) == '"')
        {
          ++count;
        }

        if (count == 1)
        {
          place = i;
          searching = false;
        }
      }

      overURI.remove(place, overURI.length());

      Serial.println(overURI);

      //get the position of the track
      int pos = getPosition();
      
      //get the track uri of the current song
      String trackURI = payload;
      count = 0;
      place = 0;
      searching = true;

      for (int i = 0; i < trackURI.length() && searching; i++)
      {
        if (trackURI.charAt(i) == '}')
        {
          ++count;
        }

        if (count == 13)
        {
          place = i;
          searching = false;
        }
      }

      trackURI.remove(0, place + 36);

      for (int i = 0; i < trackURI.length(); i++)
      {
        if (trackURI.charAt(i) == '"')
        {
          trackURI.remove(i, trackURI.length());
          break;
        }
      }

      trackURI.replace("/", ":");

      String URI = "spotify" + trackURI;
      
      Serial.println(URI); 

      Resume(overURI, URI, pos);
    } else {
      Serial.println("error on http request");
      http.end();
    } 
}

/*
 * The only way I was able to figure out how to resume a song  
 * since spotify's api is only capable of pausing and playing 
 * a song from a playlist/album/artist uri, if we wanted to 
 * resume a song and keep the same album / playlist queued up
 * there is currently no way of doing so, so my solution was
 * get the current playlist or album we were listening to,
 * play it and queue up the current song, that way we can
 * skip to that song and remain within the same playlist,
 * only issue with this method is the speed since we are 
 * making 4 requests not counting the request to get all the 
 * uri's so there is a noticeable gap and delay between playing 
 * the correct song and just playing the playlist. 
 * 
 * I couldn't find a method of resuming a song while keeping
 * the same queue of songs in order so I opted out of putting 
 * this in the final build but kept the code incase anyone doesn't
 * mind the delay
 */

String Spotify:: Resume(String overURI, String trackURI, int pos)
{
  //we play the playlist 
  play(overURI);
  //queue up the current song we were listening to
  addToQueue(trackURI);
  //then skip and seek the position we were at
  //when the song was paused
  skip();
  seekPos(pos);
  
}

//adds a track uri to the queue, the api only 
//allows tracks or episodes to be queued up so 
//no albums or playlists can be queued
String Spotify:: addToQueue(String URI)
{
  String authorization = "Bearer " + accessToken;

  URI.replace(":", "%3A");
  
  String url = "https://api.spotify.com/v1/me/player/queue?uri=" + URI;
  //set up request
  http.begin(url, String(FingerPrint));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", authorization);

  int code = http.POST("");

  if (code > 0)
  {
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.println("error on http request for adding to queue");
    http.end();
  }
  
}

//skip to the millisecond position of a song using an integer input
String Spotify:: seekPos(int pos)
{
  String authorization = "Bearer " + accessToken;
  String url = "https://api.spotify.com/v1/me/player/seek?position_ms=" + String(pos);

  http.begin(url, String(FingerPrint));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", authorization);

  int code = http.PUT("");

  if (code > 0)
  {
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.println("error on http request for seeking to position");
    http.end();
  }
}

//gets the current millisecond position of a song 
int Spotify:: getPosition()
{
  String playback = getPlaybackState();

  //check if anything is playing 
  if (playback.length() < 15) return 0;

  
  //remove garbage from the front
  bool searching = true;
  int count = 0;
  int place;
  
  for (int i = 0; i < playback.length() && searching; i++)
  {
    if (playback.charAt(i) == ',')
    {
      count ++;
    }

    if (count == 14)
    {
      place = i;
      searching = false;
    }
  }

  place += 15;

  playback.remove(0, place);
  playback.remove(10, playback.length());
 
  Serial.println(playback);

  int pos = playback.toInt();

  Serial.println("position: " + String(pos));

   return pos;

}

//gets the current volume of the playback device and returns it
int Spotify:: getVolume()
{

  String playbackState = getPlaybackState();

  //nothing is playing 
  if (playbackState.length() < 15) return 0;

  //parse the playback state to get the volume
  playbackState.remove(0,10);

  int count = 0;
  int place;
  bool searching = true;

  for (int i = 0; i < playbackState.length() && searching; i++)
  {
    if (playbackState.charAt(i) == ',')
    {
      count++;
    }
    if (count == 7)
      {
        place = i;
        searching = false;
      }
  }
  
  playbackState.remove(place, playbackState.length());
  count = 0;
  searching = true;

  for (int i = 0; i < playbackState.length() && searching; i++)
  {
    if (playbackState.charAt(i) == ':')
    {
      ++count;
    }

    if (count == 7)
    {
      place = i;
      searching = false;
    }
  }


  playbackState.remove(0, place + 1);

  int volume = playbackState.toInt();

  //double check the volume and ensure it is correct
  Serial.println("volume: " + String(volume));

  currVolume = volume;
  
  return int(volume);
}

//skips to the next song in the queue
String Spotify:: skip()
{
  String authorization = "Bearer " + accessToken;

  //set up request
  http.begin("https://api.spotify.com/v1/me/player/next", String(FingerPrint));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", authorization);

  int code = http.POST("");

  if (code > 0)
  {
    String payload = http.getString();
    
  } else {
    Serial.println("failed at the skip request");
    http.end();
  }

}

//goes to the previous song in the queue
String Spotify:: previous()
{
    String authorization = "Bearer " + accessToken;

  //set up request
  http.begin("https://api.spotify.com/v1/me/player/previous", String(FingerPrint));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", authorization);

  int code = http.POST("");

  if (code > 0)
  {
    String payload = http.getString();
    
  } else {
    Serial.println("failed at the previous request");
    http.end();
  }

}

//pauses the current playback device
String Spotify:: pause()
{
   String authorization = "Bearer " + accessToken;

  //set up request
  http.begin("https://api.spotify.com/v1/me/player/pause", String(FingerPrint));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", authorization);

  int code = http.PUT("");

  if (code > 0)
  {
    String payload = http.getString();
    
  } else {
    Serial.println("failed at the pause request");
    http.end();
  }  
}

//sets the volume of the playback device
String Spotify:: setVolume(int increment)
{
  String authorization = "Bearer " + accessToken;
  
  //get device id
  getDevice();

  //increase / decrease the volume by the increment specified
  currVolume += increment;
  //create the url
  String volume = "?volume_percent=" + String(currVolume) + "&device_id=" + deviceID;
  //set up request
  Serial.println("https://api.spotify.com/v1/me/player/volume" + volume);
  http.begin("https://api.spotify.com/v1/me/player/volume" + volume, String(FingerPrint));
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", authorization);

  Serial.println("made the request");

  int code = http.PUT("");

  if (code > 0)
  {
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.println("failed at the volume request");
    http.end();
  }
  
}

int Spotify:: getFunc(decode_results code)
{
  Serial.println(code.value, HEX);
  switch (code.value)
  {
    
    /* 
     *  * * * * * * * * *
     * Spotify Controls
     * * * * * * * * * * 
     */
     
    //play/pause
    /*
     * Did not choose to insert the current 
     * resume method as it was to slow
     * and wasn't as clean as wished so only
     * a pause function is implemented
     * 
     * currentDevice();
     */
    case 0xFF02FD:
      getToken();
      pause();
      //this code is placed at the end of 
      //every case to ensure the receiver
      //doesn't restart the microcontroller
      if (irrecv.decode(&code)) {
        irrecv.resume();
      }
      break;
      
      //volume up
     case 0xFF629D:
      getToken();
      getVolume();
      setVolume(10);
      if (irrecv.decode(&code)) {
        irrecv.resume();
      }
      break;

     //volume down
     case 0xFFA857:
      getToken();
      getVolume();
      setVolume(-10);
      if (irrecv.decode(&code)) {
        irrecv.resume();
      }
      break;

     //skip song
    case 0xFFC23D:
     getToken();
     skip();
     if (irrecv.decode(&code)) {
        irrecv.resume();
      }
     break;

    //previous song
    case 0xFF22DD:
      getToken();
      previous();
      if (irrecv.decode(&code)) {
        irrecv.resume();
      }
      break;

    /* * * * * * * *
     * LED controls
     * * * * * * * * 
     */

    //power button
    case 0xFFA25D:
      turnOffLeds();
      getToken();
      pause();
      if (irrecv.decode(&code)) {
        irrecv.resume();
      }
      break;
      
    //remote 1
    case 0xFF30CF:
      whiteLed();
      if (irrecv.decode(&code)) {
        irrecv.resume();
      }
      break;
    //remote 2 
    case 0xFF18E7:
      redLed();
      if (irrecv.decode(&code)) {
        irrecv.resume();
      }
      break;
    //remote 3 
    case 0xFF7A85:
      greenLed();
      if (irrecv.decode(&code)) {
        irrecv.resume();
      }
      break;
    //remote 4 
    case 0xFF10EF:
      blueLed();
      if (irrecv.decode(&code)) {
        irrecv.resume();
      }
      break;  

  }
}


String parseNFCTagData(byte *dataBuffer)
{
  // first 28 bytes is header info
  // data ends with 0xFE
  String retVal = "spotify:";
  for (int i = 28 + 12; i < BUFFERSIZE; i++)
  {
    if (dataBuffer[i] == 0xFE || dataBuffer[i] == 0x00)
    {
      break;
    }
    if (dataBuffer[i] == '/')
    {
      retVal += ':';
    }
    else
    {
      retVal += (char)dataBuffer[i];
    }
  }
  return retVal;
}

bool readNFCTagData(byte *dataBuffer)
{
  MFRC522::StatusCode status;
  byte byteCount;
  byte buffer[18];
  byte x = 0;

  int totalBytesRead = 0;

  // reset the dataBuffer
  for (byte i = 0; i < BUFFERSIZE; i++)
  {
    dataBuffer[i] = 0;
  }

  for (byte page = 0; page < BUFFERSIZE / 4; page += 4)
  {
    // Read pages
    byteCount = sizeof(buffer);
    status = rfid.MIFARE_Read(page, buffer, &byteCount);
    if (status == rfid.STATUS_OK)
    {
      totalBytesRead += byteCount - 2;

      for (byte i = 0; i < byteCount - 2; i++)
      {
        dataBuffer[x++] = buffer[i]; // add data output buffer
      }
    }
    else
    {
      break;
    }
  }
}

/* * * * * * * * * * * * *
 * FUNCTIONS FOR MY LEDS
 * * * * * * * * * * * * *
 */

void greenLed()
{
  for (int i = 0; i < 2; i++)
  {
    digitalWrite(green[i], HIGH);
    digitalWrite(red[i], LOW);
    digitalWrite(blue[i], LOW);
    
  }
}

void redLed()
{
  for (int i = 0; i < 2; i++)
  {
    digitalWrite(green[i], LOW);
    digitalWrite(red[i], HIGH);
    digitalWrite(blue[i], LOW);
    
  }
}

void blueLed()
{
  
  for (int i = 0; i < 2; i++)
  {
    digitalWrite(green[i], LOW);
    digitalWrite(red[i], LOW);
    digitalWrite(blue[i], HIGH);
    
  }
}

void whiteLed()
{
  
  for (int i = 0; i < 2; i++)
  {
    digitalWrite(green[i], HIGH);
    digitalWrite(red[i], HIGH);
    digitalWrite(blue[i], HIGH);
    
  }
}

void turnOffLeds()
{
  
  for (int i = 0; i < 2; i++)
  {
    digitalWrite(green[i], LOW);
    digitalWrite(red[i], LOW);
    digitalWrite(blue[i], LOW);
    
  }
}
