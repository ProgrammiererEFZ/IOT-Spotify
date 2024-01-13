const char mainPage[] PROGMEM = R"=====(
<HTML>
    <HEAD>
        <TITLE>Spotify Controller</TITLE>
    </HEAD>
    <BODY>
        <CENTER>
            <a href="https://accounts.spotify.com/authorize?response_type=code&client_id=%s&redirect_uri=%s&scope=user-modify-playback-state user-read-currently-playing user-read-playback-state user-library-modify user-library-read">Spotify Login</a>
        </CENTER>
    </BODY>
</HTML>
)=====";