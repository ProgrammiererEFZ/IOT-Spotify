//JSON Parser
#include <ArduinoJson.h>

//Base 64 encode
#include <base64.h>

//WiFI
#include <ESP8266WiFi.h>

//HTTP Server
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecureBearSSL.h>

#include "Web_Fetch.h"
#include "index.h"

//Display
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// WiFi credentials
#define WIFI_SSID ""
#define PASSWORD ""

// Spotify API credentials
#define CLIENT_ID ""
#define CLIENT_SECRET ""
#define REDIRECT_URI "http://192.168.1.42/callback"

// Button Pins
#define PLAY_BUTTON 0
#define FORWARD_BUTTON 2
#define BACK_BUTTON 16

int lastVolRead = 0;

/**
 * @brief This function is used to parse the HTTP response from the Spotify API.
 *
 * @param http An HTTPClient object that represents the HTTP connection.
 * @param key A String that represents the key to look for in the HTTP response.
 *
 * @return A String that represents the value associated with the given key in the HTTP response.
 *
 * The function reads the HTTP response character by character, looking for the specified key.
 * Once the key is found, it starts to build a return string (`ret_str`) with the characters following the key
 * until it encounters a newline character (`\n`). If the last character of the return string is a comma, it is removed.
 * The function then returns the built string.
 *
 * This function is used to extract specific values from the HTTP response, such as the song name, artist name, album name, etc.
 */
String getValue(HTTPClient &http, String key) {
    bool found = false, look = false, seek = true;
    int ind = 0;
    String ret_str = "";

    int len = http.getSize();
    char char_buff[1];
    WiFiClient *stream = http.getStreamPtr();
    while (http.connected() && (len > 0 || len == -1)) {
        size_t size = stream->available();
        if (size) {
            int c = stream->readBytes(char_buff, ((size > sizeof(char_buff)) ? sizeof(char_buff) : size));
            if (found) {
                if (seek && char_buff[0] != ':') { // Skip until we find a colon
                    continue;
                } else if (char_buff[0] != '\n') { // If we haven't reached the end of the line
                    if (seek && char_buff[0] == ':') { // If we found a colon
                        seek = false;
                        int c = stream->readBytes(char_buff, 1);
                    } else {
                        ret_str += char_buff[0]; // Add the character to the return string
                    }
                } else {
                    break; // We've reached the end of the line, break the loop
                }
            } else if ((!look) && (char_buff[0] ==
                                   key[0])) { // If we're not currently looking and we found the first character of the key
                look = true;
                ind = 1;
            } else if (look &&
                       (char_buff[0] == key[ind])) { // If we're looking and we found the next character of the key
                ind++;
                if (ind == key.length()) found = true; // If we've found the entire key, set found to true
            } else if (look && (char_buff[0] != key[ind])) { // If we're looking and the character doesn't match the key
                ind = 0;
                look = false;
            }
        }
    }

    if (*(ret_str.end() - 1) == ',') { // If the last character of the return string is a comma
        ret_str = ret_str.substring(0, ret_str.length() - 1); // Remove the last character
    }
    return ret_str;
}

//http response struct
struct httpResponse {
    int responseCode;
    String responseMessage;
};

struct songDetails {
    int durationMs;
    String album;
    String artist;
    String song;
    String Id;
};


/**
 * @class SpotifyConnection
 *
 * This class is responsible for managing the connection to the Spotify API.
 * It handles tasks such as getting the user code, refreshing the authorization, getting track information,
 * adjusting the volume, skipping forward and backward, and drawing the screen.
 *
 * @property accessTokenSet A boolean indicating whether the access token is set.
 * @property tokenStartTime The start time of the token.
 * @property tokenExpireTime The expiration time of the token.
 * @property currentSong A struct holding the details of the current song.
 * @property currentSongPositionMs The current position of the song in milliseconds.
 * @property lastSongPositionMs The last position of the song in milliseconds.
 * @property currVol The current volume.
 * @property client A unique pointer to a WiFiClientSecure object.
 * @property https An HTTPClient object.
 * @property isPlaying A boolean indicating whether a song is currently playing.
 * @property accessToken The access token for the Spotify API.
 * @property refreshToken The refresh token for the Spotify API.
 *
 * @method getUserCode This method is used to get the user code from the Spotify API.
 * @method refreshAuth This method is used to refresh the authorization with the Spotify API.
 * @method getTrackInfo This method is used to get the track information from the Spotify API.
 * @method togglePlay This method is used to toggle the play state of a song.
 * @method adjustVolume This method is used to adjust the volume.
 * @method skipForward This method is used to skip forward in a song.
 * @method skipBack This method is used to skip backward in a song.
 */
class SpotifyConnection {
public:
    /**
 * @brief Default constructor for the SpotifyConnection class.
 *
 * This constructor initializes the SpotifyConnection object. It creates a new instance of the WiFiClientSecure class
 * and sets it to be insecure. This is necessary for the HTTPS connections that the SpotifyConnection class makes to the Spotify API.
 *
 */
    SpotifyConnection() {
        client = std::make_unique<BearSSL::WiFiClientSecure>();
        client->setInsecure();
    }

/**
 * @brief This method is used to get the user code from the Spotify API.
 *
 * @param serverCode A String that represents the server code.
 *
 * @return A boolean value indicating whether the access token is set.
 *
 * The function sends a POST request to the Spotify API to get the user code.
 * It sets the Authorization and Content-Type headers of the request, and constructs the request body using the server code and the redirect URI.
 * If the HTTP response code is 200 (OK), the function parses the response to get the access token, refresh token, and token expiration time.
 * It then sets the access token, refresh token, and token expiration time of the SpotifyConnection object, and sets accessTokenSet to true.
 * If the HTTP response code is not 200, the function prints the HTTP response code and the response body to the serial monitor for debugging purposes.
 */
    bool getUserCode(String serverCode) {
        https.begin(*client, "https://accounts.spotify.com/api/token");
        String auth = "Basic " + base64::encode(String(CLIENT_ID) + ":" + String(CLIENT_SECRET));
        https.addHeader("Authorization", auth);
        https.addHeader("Content-Type", "application/x-www-form-urlencoded");
        String requestBody =
                "grant_type=authorization_code&code=" + serverCode + "&redirect_uri=" + String(REDIRECT_URI);
        Serial.println("Request Body: " + requestBody);
        Serial.println("Authentication: " + auth);
        int httpResponseCode = https.POST(requestBody);
        if (httpResponseCode == HTTP_CODE_OK) {
            String response = https.getString();
            DynamicJsonDocument doc(1024); // Used to store JSON
            deserializeJson(doc, response); // Parse the response body
            accessToken = String((const char *) doc["access_token"]); // Get the access token from the response
            refreshToken = String((const char *) doc["refresh_token"]); // Get the refresh token from the response
            tokenExpireTime = doc["expires_in"]; // Get the token expiration time from the response
            tokenStartTime = millis();
            accessTokenSet = true;
            Serial.println("Access Token: " + accessToken);
            Serial.println("Refresh Token: " + refreshToken);
        } else {
            Serial.println("Error in Response: " + https.getString()); // Debug if not OK
        }
        https.end();
        return accessTokenSet;
    }

/**
 * @brief This method is used to refresh the authorization with the Spotify API.
 *
 * @return A boolean value indicating whether the access token is set.
 *
 * The function sends a POST request to the Spotify API to refresh the authorization.
 * It sets the Authorization and Content-Type headers of the request, and constructs the request body using the refresh token.
 * If the HTTP response code is 200 (OK), the function parses the response to get the access token and token expiration time.
 * It then sets the access token and token expiration time of the SpotifyConnection object, and sets accessTokenSet to true.
 * If the HTTP response code is not 200, the function prints the HTTP response code and the response body to the serial monitor for debugging purposes.
 */
    bool refreshAuth() {
        https.begin(*client, "https://accounts.spotify.com/api/token");
        String auth = "Basic " + base64::encode(String(CLIENT_ID) + ":" + String(CLIENT_SECRET));
        https.addHeader("Authorization", auth);
        https.addHeader("Content-Type", "application/x-www-form-urlencoded");
        String requestBody = "grant_type=refresh_token&refresh_token=" + String(refreshToken);
        // Send the POST request to the Spotify API
        int httpResponseCode = https.POST(requestBody);
        accessTokenSet = false;
        // Check if the request was successful
        if (httpResponseCode == HTTP_CODE_OK) {
            String response = https.getString();
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, response);
            accessToken = String((const char *) doc["access_token"]);
            tokenExpireTime = doc["expires_in"];
            tokenStartTime = millis();
            accessTokenSet = true;
            Serial.println("Access Token: " + accessToken);
            Serial.println("Refresh Token: " + refreshToken);
        } else {
            Serial.println("Error in Response: " + https.getString()); // Debug if not OK
        }
        // Disconnect from the Spotify API
        https.end();
        return accessTokenSet;
    }

/**
 * @brief This method is used to get the track information from the Spotify API.
 *
 * @return A boolean value indicating whether the track information was successfully retrieved.
 *
 * The function sends a GET request to the Spotify API to get the currently playing track information.
 * It sets the Authorization header of the request using the access token.
 * If the HTTP response code is 200 (OK), the function parses the response to get the track details such as song name, artist name, album name, song duration, and song ID.
 * It also checks if the song is currently playing and updates the `isPlaying` property accordingly.
 * If the song ID is different from the current song ID.
 * It then updates the `currentSong` property with the new track details and calls the `printToDisplay` method to update the display.
 * If the HTTP response code is not 200, the function prints the HTTP response code to the serial monitor for debugging purposes.
 * The function returns true if the track information was successfully retrieved, and false otherwise.
 */
    bool getTrackInfo() {
        String url = "https://api.spotify.com/v1/me/player/currently-playing";
        https.useHTTP10(true);
        https.begin(*client, url);
        String auth = "Bearer " + String(accessToken);
        https.addHeader("Authorization", auth);
        int httpResponseCode = https.GET();
        bool success = false;
        String songId = "";
        // Check if the request was successful
        if (httpResponseCode == 200) {
            String albumName = getValue(https, "name");
            String artistName = getValue(https, "name");
            String songDuration = getValue(https, "duration_ms");
            currentSong.durationMs = songDuration.toInt();
            String songName = getValue(https, "name");
            songId = getValue(https, "uri");
            String isPlay = getValue(https, "is_playing");
            isPlaying = isPlay == "true";
            songId = songId.substring(15, songId.length() - 1);

            https.end();

            currentSong.artist = artistName.substring(1, artistName.length() - 1);
            currentSong.song = songName.substring(1, songName.length() - 1);
            currentSong.Id = songId;

            printToDisplay(currentSong.song + "  -  " + currentSong.artist);

            success = true;
        } else {
            Serial.print("Error getting track info: ");
            Serial.println("Error in Response: " + https.getString()); // Debug if not OK
            https.end();
        }

       return success;
    }




/**
 * @brief This method is used to toggle the play state of a song.
 *
 * @return A boolean value indicating whether the operation was successful.
 *
 * The function constructs a URL for the Spotify API endpoint to either pause or play the current song, based on the current play state.
 * It then sends a PUT request to the constructed URL.
 * The Authorization header of the request is set using the access token.
 * If the HTTP response code is 204 (No Content), which indicates that the request has succeeded, the function toggles the `isPlaying` property and prints a message to the serial monitor.
 * The function then calls the `getTrackInfo` method to update the track information.
 * If the HTTP response code is not 204, the function prints the HTTP response code and the response body to the serial monitor for debugging purposes.
 * The function returns true if the operation was successful, and false otherwise.
 */
    bool togglePlay() {
        String url = "https://api.spotify.com/v1/me/player/" + String(isPlaying ? "pause" : "play");
        isPlaying = !isPlaying;
        https.begin(*client, url);
        String auth = "Bearer " + String(accessToken);
        https.addHeader("Authorization", auth);
        int httpResponseCode = https.PUT("");
        bool success = false;
        // Check if the request was successful
        if (httpResponseCode == 204) {
            Serial.println((isPlaying ? "Playing" : "Pausing"));
            success = true;
        } else {
            Serial.print("Error pausing or playing: ");
            Serial.println("Error Code: " + httpResponseCode);
            String response = https.getString();
            Serial.println("Error in Response: " + https.getString()); // Debug if not OK
        }
        // Disconnect from the Spotify API
        https.end();
        getTrackInfo();
        return success;
    }

/**
 * @brief This method is used to adjust the volume of the Spotify player.
 *
 * @param vol An integer that represents the desired volume level.
 *
 * @return A boolean value indicating whether the operation was successful.
 *
 * The function constructs a URL for the Spotify API endpoint to set the volume to the specified level.
 * It then sends a PUT request to the constructed URL.
 * The Authorization header of the request is set using the access token.
 * If the HTTP response code is 204 (No Content), which indicates that the request has succeeded, the function updates the `currVol` property and returns true.
 * If the HTTP response code is 403, the function updates the `currVol` property and returns false.
 * If the HTTP response code is neither 204 nor 403, the function prints the HTTP response code and the response body to the serial monitor for debugging purposes, and returns false.
 */
    bool adjustVolume(int vol) {
        String url = "https://api.spotify.com/v1/me/player/volume?volume_percent=" + String(vol);
        https.begin(*client, url);
        String auth = "Bearer " + String(accessToken);
        https.addHeader("Authorization", auth);
        int httpResponseCode = https.PUT("");
        bool success = false;
        // Check if the request was successful
        if (httpResponseCode == 204) {
            currVol = vol;
            success = true;
        } else if (httpResponseCode == 403) {
            currVol = vol;
            success = false;
            Serial.print("Error setting volume: ");
            Serial.println("Error Code: " + httpResponseCode);
            String response = https.getString();
            Serial.println("Error in Response: " + https.getString()); // Debug if not OK
        } else {
            Serial.print("Error setting volume: ");
            Serial.println("Error Code: " + httpResponseCode);
            String response = https.getString();
            Serial.println("Error in Response: " + https.getString()); // Debug if not OK
        }

        // Disconnect from the Spotify API
        https.end();
        return success;
    }

/**
 * @brief This method is used to skip forward in the Spotify playlist.
 *
 * @return A boolean value indicating whether the operation was successful.
 *
 * The function constructs a URL for the Spotify API endpoint to skip to the next song in the playlist.
 * It then sends a POST request to the constructed URL.
 * The Authorization header of the request is set using the access token.
 * If the HTTP response code is 204 (No Content), which indicates that the request has succeeded, the function prints a message to the serial monitor and returns true.
 * If the HTTP response code is not 204, the function prints the HTTP response code and the response body to the serial monitor for debugging purposes.
 * The function then calls the `getTrackInfo` method to update the track information.
 * The function returns true if the operation was successful, and false otherwise.
 */
    bool skipForward() {
        String url = "https://api.spotify.com/v1/me/player/next";
        https.begin(*client, url);
        String auth = "Bearer " + String(accessToken);
        https.addHeader("Authorization", auth);
        int httpResponseCode = https.POST("");
        bool success = false;
        // Check if the request was successful
        if (httpResponseCode == 204) {
            Serial.println("skipping forward");
            success = true;
        } else {
            Serial.print("Error skipping forward: ");
            Serial.println("Error Code: " + httpResponseCode);
            String response = https.getString();
            Serial.println("Error in Response: " + https.getString()); // Debug if not OK
        }

        // Disconnect from the Spotify API
        https.end();
        getTrackInfo();
        return success;
    }

/**
 * @brief This method is used to skip backward in the Spotify playlist.
 *
 * @return A boolean value indicating whether the operation was successful.
 *
 * The function constructs a URL for the Spotify API endpoint to skip to the previous song in the playlist.
 * It then sends a POST request to the constructed URL.
 * The Authorization header of the request is set using the access token.
 * If the HTTP response code is 204 (No Content), which indicates that the request has succeeded, the function prints a message to the serial monitor and returns true.
 * If the HTTP response code is not 204, the function prints the HTTP response code and the response body to the serial monitor for debugging purposes.
 * The function then calls the `getTrackInfo` method to update the track information.
 * The function returns true if the operation was successful, and false otherwise.
 */
    bool skipBack() {
        String url = "https://api.spotify.com/v1/me/player/previous";
        https.begin(*client, url);
        String auth = "Bearer " + String(accessToken);
        https.addHeader("Authorization", auth);
        int httpResponseCode = https.POST("");
        bool success = false;
        // Check if the request was successful
        if (httpResponseCode == 204) {
            Serial.println("skipping backward");
            success = true;
        } else {
            Serial.print("Error skipping backward: ");
            Serial.println("Error Code: " + httpResponseCode);
            String response = https.getString();
            Serial.println("Error in Response: " + https.getString()); // Debug if not OK
        }

        // Disconnect from the Spotify API
        https.end();
        getTrackInfo();
        return success;
    }


    bool accessTokenSet = false;
    long tokenStartTime;
    int tokenExpireTime;
    songDetails currentSong;
    float currentSongPositionMs;
    float lastSongPositionMs;
    int currVol;
private:
    std::unique_ptr <BearSSL::WiFiClientSecure> client;
    HTTPClient https;
    bool isPlaying = false;
    String accessToken;
    String refreshToken;
};

//Vars for keys, play state, last song, etc.
bool buttonStates[] = {1, 1, 1, 1};
int debounceDelay = 50;
unsigned long debounceTimes[] = {0, 0, 0, 0};
int buttonPins[] = {4, 5, 6, 7}; //TODO: Anpassen

ESP8266WebServer server(80);
SpotifyConnection spotifyConnection;

U8G2_SSD1306_128X64_ALT0_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // SSD1306 and SSD1308Z are compatible TODO: Pins? https://wiki.seeedstudio.com/Grove-OLED_Display_0.96inch/

/**
 * @brief This method handles the root ("/") path of the web server.
 *
 * When a client sends a GET request to the root path of the server, this method is called.
 * It constructs a web page using the mainPage template and the Spotify client ID and redirect URI.
 * It then sends the constructed web page as the HTTP response to the client.
 * The HTTP response code is 200 (OK), and the Content-Type header is set to "text/html".
 */
void handleRoot() {
    Serial.println("handling root");
    char page[500];
    sprintf(page, mainPage, CLIENT_ID, REDIRECT_URI);
    server.send(200, "text/html", String(page) + "\r\n"); //Send web page
}

/**
 * @brief This method handles the "/callback" path of the web server.
 *
 * When a client sends a GET request to the "/callback" path of the server, this method is called.
 * It checks if the access token is set. If not, it checks if the "code" parameter is present in the request.
 * If the "code" parameter is not present, it constructs an error page using the errorPage template and the Spotify client ID and redirect URI, and sends the constructed page as the HTTP response to the client.
 * If the "code" parameter is present, it calls the `getUserCode` method of the `spotifyConnection` object with the "code" parameter.
 * If the `getUserCode` method returns true, it sends a success message as the HTTP response to the client.
 * If the `getUserCode` method returns false, it constructs an error page using the errorPage template and the Spotify client ID and redirect URI, and sends the constructed page as the HTTP response to the client.
 * If the access token is set, it sends a success message as the HTTP response to the client.
 */
void handleCallbackPage() {
    if (!spotifyConnection.accessTokenSet) {
        if (server.arg("code") == "") {     //Parameter not found
            char page[500];
            sprintf(page, page, CLIENT_ID, REDIRECT_URI);
            server.send(200, "text/html", String(page)); //Send web page
        } else {     //Parameter found
            if (spotifyConnection.getUserCode(server.arg("code"))) {
                server.send(200, "text/html",
                            "Spotify setup complete Auth refresh in :" + String(spotifyConnection.tokenExpireTime));
            } else {
                char page[500];
                sprintf(page, page, CLIENT_ID, REDIRECT_URI);
                server.send(200, "text/html", String(page)); //Send web page
            }
        }
    } else {
        server.send(200, "text/html", "Spotify setup complete");
    }
}

/**
 * @brief This function prints a given message to the display.
 *
 * This function takes a string as a parameter and displays it on the screen.
 * It first clears the internal memory of the display, then sets the font.
 * After that, it writes the given message to the internal memory of the display.
 * The message is then transferred from the internal memory to the display.
 *
 * @param message The message to be displayed on the screen.
 */
void printToDisplay(String scrollText) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  const char* charArray = scrollText.c_str();
  int textWidth = u8g2.getStrWidth(charArray);
  int scrollSpeed = 16;
  static int xPos = 5;
  u8g2.drawStr(xPos, 24, charArray);
  xPos -= scrollSpeed;
  if (xPos < -textWidth) {
    xPos = 128;
  }
  u8g2.sendBuffer();
}

long timeLoop;
long refreshLoop;
bool serverOn = true;
static int loopCounter; // Add a counter for loop iterations

/**
 * @brief This method is the setup function for the Arduino sketch.
 *
 * This function is called once when the sketch starts. It is used to initialize variables, input and output pin modes, and start using libraries.
 * The function does the following:
 * - Starts the serial communication with a baud rate of 115200.
 * - Connects to the WiFi network using the specified SSID and password.
 * - Starts the HTTP server and sets up the server routes.
 * - Sets up the buttons for the Spotify player.
 */
void setup() {
    Serial.begin(115200);

    // Initialise the display
    u8g2.begin();

    WiFi.begin(WIFI_SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi\n Ip is: ");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/callback", handleCallbackPage);
    server.begin(); //Start server
    Serial.println("HTTP server started");

    pinMode(PLAY_BUTTON, INPUT);
    pinMode(FORWARD_BUTTON, INPUT);
    pinMode(BACK_BUTTON, INPUT);

    printToDisplay(WiFi.localIP().toString());
    loopCounter = 0; // Add a counter for loop iterations
}

/**
 * @brief This is the main loop function for the Arduino sketch.
 *
 * This function is called repeatedly in the main program. It is used to update the state of the Spotify player.
 * The function does the following:
 * - Checks if the access token is set. If not, it handles the HTTP server client.
 * - If the access token is set, it checks if the token has expired. If so, it refreshes the authorization with the Spotify API.
 * - Every 10000 iterations of the loop, it gets the track information from the Spotify API, otherwise interruption is to long.
 * - It reads the state of the buttons. If a button is pressed, it performs the corresponding action (toggle play state,  skip forward, skip backward).
 * - It reads the value of the analog input A0, maps it to a volume level, and adjusts the volume if the requested volume level is different from the current volume level.
 */
void loop() {
    loopCounter++;
    if (spotifyConnection.accessTokenSet) {
        if (serverOn) {
            server.close();
            serverOn = false;
        }
        if ((millis() - spotifyConnection.tokenStartTime) / 1000 > spotifyConnection.tokenExpireTime) {
            Serial.println("refreshing token");
            if (spotifyConnection.refreshAuth()) {
                Serial.println("refreshed token");
            }
        }
        if (loopCounter >= 10000) {
            spotifyConnection.getTrackInfo();
            refreshLoop = millis();
            loopCounter = 0;
        }
        int pinstatePlay = digitalRead(PLAY_BUTTON);
        int pinstateForward = digitalRead(FORWARD_BUTTON);
        int pinstateBack = digitalRead(BACK_BUTTON);

        if(pinstatePlay == HIGH){
          Serial.println("Play button pressed");
          spotifyConnection.togglePlay();
          delay(100);
        } 
        if(pinstateForward == LOW){
          Serial.println("Skip Forward button pressed");
          spotifyConnection.skipForward();
          delay(100);
        } 
        if(pinstateBack == LOW){
          Serial.println("Skip back button pressed");
          spotifyConnection.skipBack();
          delay(100);
        } 

        int currentVolRead = analogRead(A0);
        if(abs(currentVolRead - lastVolRead) > 15){
          Serial.println("adjusting volume");
          spotifyConnection.adjustVolume(map(analogRead(A0), 0, 1023, 100, 0));
          lastVolRead = currentVolRead;
        }
        

        timeLoop = millis();
    } else {
        server.handleClient();
    }
}
