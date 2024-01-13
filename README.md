# Spotify Remote Control
This project is part of the [IoT Engineering](https://github.com/tamberg/fhnw-iot) course.

## Introduction
The software allows remote control of a Spotify session. With buttons to skip, go to previous, pause/play songs. Also displays song title and artist on Grove display.

### Team members
* @ProgrammiererEFZ, Luca Plozner
* @xenoisenegger, Xeno Isenegger
* @stefansimic, Stefan Simic

## Deliverables
The following deliverables are mandatory.

### Source code
[IOT-Spotify.ino](IOT-Spotify.ino)


### Setup
1. Install the **U8g2** and **ArduinoJson** library under **Manage Libraries**.
2. Set WiFi credentials in [IOT-Spotify.ino](IOT-Spotify.ino)
3. Retrieve local IP by loading [IOT-Spotify.ino](IOT-Spotify.ino) onto ESP8266 and running it.
4. [Create Spotify App](https://developer.spotify.com/dashboard/create), set callback to `http://local_IP/callback/' on API, give all permissions.
5. Copy credentials and callback address into [IOT-Spotify.ino](IOT-Spotify.ino)
6. Load script onto ESP8266, go to IP and log into Spotify.
7. Start a song.

### Presentation
4-slide presentation, PDF format, committed to (this) project repo.

[MY_TEAM_PROJECT_PRESENTATION.pdf](MY_TEAM_PROJECT_PRESENTATION.pdf)

1) Use-case of your project.
2) Reference model of your project.
3) Single slide interface documentation.
4) Issues you faced, how you solved them.

### Live demo
Working end-to-end prototype, "device to cloud", part of your 10' presentation.

[http://172.20.10.14/](http:/172.20.10.14/)

1) Sensor input on a IoT device triggers an event.
2) The event or measurement shows up online, in an app or Web client.
3) The event triggers actuator output on the same or on a separate IoT device.

