# Spotify Remote Control
This project is part of the [IoT Engineering](https://github.com/tamberg/fhnw-iot) course.

## Introduction
The software allows remote control of a Spotify session. With buttons to skip, go to previous, pause/play songs. Also displays song title and artist on Grove display.

### Team members
* @ProgrammiererEFZ, Luca Plozner
* @xenoisenegger, Xeno Isenegger
* , Stefan Simic

## Deliverables
The following deliverables are mandatory.

### Source code
Source code, Arduino C, JS or Python, committed to (this) project repo.

[Arduino/MY_TEAM_PROJECT/MY_TEAM_PROJECT.ino](Arduino/MY_TEAM_PROJECT_FILE.ino)

[Nodejs/MY_TEAM_PROJECT.js](Nodejs/MY_TEAM_PROJECT_FILE.js)

[Python/MY_TEAM_PROJECT.py](Nodejs/MY_TEAM_PROJECT_FILE.py)

### Setup
1. Install the **U8g2** and **ArduinoJson** library under **Manage Libraries**.
2. Set WiFi credentials in TODO.ino
3. Retrieve local IP by loading TODO.ino onto ESP8266 and running it.
4. [Create Spotify App](https://developer.spotify.com/dashboard/create), set callback to `http://local_IP/callback/' on API, give all permissions.
5. Copy credentials and callback address into TODO.ino
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

