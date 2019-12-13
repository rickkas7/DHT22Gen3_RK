// Example code for stress-testing the sampling code
//
// Sample output on USB serial:
// 0036755525 [app] INFO: sampleResult=0 tempF=67.1 tempC=19.5 humidity=14.5 tries=1 elapsed=24
// 0036755528 [app] INFO: success=14701 attempts=14701 successPct=100 checksumRetries=0
// 0036758025 [app] INFO: sampleResult=0 tempF=67.1 tempC=19.5 humidity=14.5 tries=1 elapsed=24
// 0036758029 [app] INFO: success=14702 attempts=14702 successPct=100 checksumRetries=0
// 0036760525 [app] INFO: sampleResult=0 tempF=67.1 tempC=19.5 humidity=14.3 tries=1 elapsed=24
// 0036760528 [app] INFO: success=14703 attempts=14703 successPct=100 checksumRetries=0

#include "DHT22Gen3_RK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

const unsigned long CHECK_INTERVAL = 2500;
unsigned long lastCheck = 0;

// The two parameters are any available GPIO pins. They will be used as output but the signals aren't
// particularly important for DHT11 and DHT22 sensors. They do need to be valid pins, however.
DHT22Gen3 dht(A4, A5);

int attempts = 0;
int success = 0;
int checksumRetries = 0;

void setup() {
	// Wait for a USB serial connection for up to 15 seconds. Useful for the tester, less so for
	// normal firmware.
	waitFor(Serial.isConnected, 15000);

	dht.setup();
}

void loop() {
	dht.loop();

	if (millis() - lastCheck >= CHECK_INTERVAL) {
		lastCheck = millis();

		unsigned long start = millis();
		attempts++;
		dht.getSample(A3, [start](DHTSample sample) {
			if (sample.isSuccess()) {
				unsigned long elapsed = millis() - start;

				Log.info("sampleResult=%d tempF=%.1f tempC=%.1f humidity=%.1f tries=%d elapsed=%lu",
						(int) sample.getSampleResult(), sample.getTempF(), sample.getTempC(), sample.getHumidity(), sample.getTries(), elapsed);

				success++;
			}
			else {
				Log.info("sample is not valid sampleResult=%d", sample.getSampleResult());
			}
			checksumRetries += sample.getTries() - 1;

			int successPct = success * 100 / attempts;
			Log.info("success=%d attempts=%d successPct=%d checksumRetries=%d", success, attempts, successPct, checksumRetries);
		});
	}

}
