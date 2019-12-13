// Example code that publishes the temperature and humidity values once per minute

#include "DHT22Gen3_RK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

// How often to check the temperature in humidity in milliseconds. Default is once per minute.
const unsigned long CHECK_INTERVAL = 60000;
unsigned long lastCheck = 0;

// The two parameters are any available GPIO pins. They will be used as output but the signals aren't
// particularly important for DHT11 and DHT22 sensors. They do need to be valid pins, however.
DHT22Gen3 dht(A4, A5);

void setup() {
	dht.setup();
}

void loop() {
	dht.loop();

	if (millis() - lastCheck >= CHECK_INTERVAL) {
		lastCheck = millis();

		dht.getSample(A3, [](DHTSample sample) {
			if (sample.isSuccess()) {
				char buf[128];
				snprintf(buf, sizeof(buf), "{\"temp\":%.1f,\"hum\":%1.f}", sample.getTempC(), sample.getHumidity());
				if (Particle.connected()) {
					Particle.publish("temperatureTest", buf, PRIVATE);
					Log.info("published: %s", buf);
				}
				else {
					Log.info("not published: %s", buf);
				}
			}
			else {
				Log.info("sample is not valid sampleResult=%d", (int) sample.getSampleResult());
			}
		});
	}

}
