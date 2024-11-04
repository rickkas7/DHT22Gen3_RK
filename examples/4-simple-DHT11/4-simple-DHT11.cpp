#include "DHT22Gen3_RK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);
// SYSTEM_MODE(MANUAL);

// How often to check the temperature in humidity in milliseconds
const unsigned long CHECK_INTERVAL = 5000;
unsigned long lastCheck = 0;

// The two parameters are any available GPIO pins. They will be used as output but the signals aren't
// particularly important for DHT11 and DHT22 sensors. They do need to be valid pins, however.
DHT22Gen3 dht(A4, A5);

void sampleCallback(DHTSample sample);

void setup() {
	dht.setup();
}

void loop() {
	dht.loop();

	if (millis() - lastCheck >= CHECK_INTERVAL) {
		lastCheck = millis();

		dht.getSample(A2, sampleCallback, &DHT22Gen3::sensorTypeDHT11);
	}

}

void sampleCallback(DHTSample sample) {
	// The callback is called at loop() (from dht.loop()) so it's safe to do anything you would normally
	// do at loop time (like publish) without having to worry about thread concurrency.

	if (sample.isSuccess()) {
		Log.info("sampleResult=%d tempF=%.1f tempC=%.1f humidity=%.1f tries=%d",
				(int) sample.getSampleResult(), sample.getTempF(), sample.getTempC(), sample.getHumidity(), sample.getTries() );
		Log.info("dewPointF=%.1f dewPointC=%.1f",
				sample.getDewPointF(), sample.getDewPointC() );
	}
	else {
		Log.info("sample is not valid sampleResult=%d", (int) sample.getSampleResult());
	}
}
