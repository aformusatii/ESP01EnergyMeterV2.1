#ifndef _AppConfig_H_
#define _AppConfig_H_

// MQTT broker (local Mosquitto, anonymous). For a broker with authentication, define
// MQTT_USER and MQTT_PASSWORD in include/WiFiConfig.h (git-ignored), not here.
#define MQTT_HOST        "192.168.100.3"
#define MQTT_PORT        1883
#define MQTT_CLIENT_ID   "esp01-energy-meter"
#define MQTT_BASE_TOPIC  "energy-meter"

// Local time defines the hour/day/week/month boundaries.
// Europe/Chisinau as a POSIX TZ string (EET, EEST from last Sunday of March 02:00
// to last Sunday of October 03:00).
#define TIME_ZONE        "EET-2EEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER_1     "pool.ntp.org"
#define NTP_SERVER_2     "time.cloudflare.com"

#endif /* _AppConfig_H_ */
