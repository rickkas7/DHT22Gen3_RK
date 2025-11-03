# DHT22Gen3 Library

There's been a problem with DHT22 libraries on Gen 3 devices (Argon, Boron, Xenon). Interrupt latency causes it to misinterpret bits for interrupt-based libraries (like PietteTech_DHT) and bit-banging isn't much better. The narrowest pulse is 26-28 us. 

I created a new library, DHT22Gen3_RK, that takes an entirely different approach. It uses the nRF52 I2S peripheral not to capture sound data, but as a DMA-based digital signal sampler. It can grab a full DHT22 sample into a 360 byte buffer at a 512 kHz sampling rate. It doesn't require any interrupts, no timers are required, and it's completely non-blocking. It doesn't have to disable interrupts or thread swapping. 

I ran a test overnight and captured 14,703 samples every 2.5 seconds with no checksum errors. The library will retry on error, but didn't have to at all.

It can sample multiple sensors (sequentially, not at the same time), one per GPIO pin. There one annoying caveat: You need to allocate two spare GPIO pins. The I2S peripheral requires that you expose SCK and LRCK on physical GPIO pins or it doesn't work. These signals are of no use for capturing DHT22 samples, but they still need to be allocated. Most people have a few GPIO left, so hopefully that's OK.

The API is callback/lambda based, as it takes about 24 milliseconds to query a DHT22. The callback allows the loop to flow freely during this time, but the callback is dispatched from the loop context so you can still do things like Particle.publish from the callback. The callback is particularly helpful if a retry is required because of a bad checksum. Since the DHT22 can only be queried every 2 seconds, retries take a long time.

- Repository: https://github.com/rickkas7/DHT22Gen3_RK
- License: MIT
- [Full browsable API documentation](https://rickkas7.github.io/DHT22Gen3_RK/)

As of version 0.0.2, it now supports:

- DHT22 (and AM2302)
- DHT11

This library cannot the used on the P2, Photon 2, or M-SoM (RTL872x).

## Example Usage

```
#include "DHT22Gen3_RK.h"

SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);

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

		dht.getSample(A3, sampleCallback);
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

```

The `getSample()` method can also take a C++11 lambda, which is handy for calling member functions or implementing the method inline:

```
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

```

When using a DHT11:

```
		dht.getSample(A3, sampleCallback, &DHT22Gen3::sensorTypeDHT11);
```

## More details

Include the `DHT22Gen3_RK` library and include its header file:

```
#include "DHT22Gen3_RK.h"
```

Add a global variable. There should only be one instance of this object, and it should be in a global scope.

```
DHT22Gen3 dht(A4, A5);
```

The two parameters are unused GPIO pins. These can be A pins, D pins, even unused special port pins that you are not otherwise using (TX, RX, SCK, MISO, MOSI). They do need to be valid pins, however, and not the same pin. The output will be 512 kHz signal on the first pin and a 32 kHz signal on the second. You shouldn't connect anything to these pins. They're necessary because the I2S peripheral that's used to decode the DHT22 signal requires the pins.

From `setup()` call `dht.setup()`:

```
void setup() {
	dht.setup();
}
```

From `loop()` call `dht.loop()`:

```
void loop() {
	dht.loop();
	
	// Rest of your code
}
```

You should call dht.loop() on every loop call even if you are not currently sampling data. It uses very little processor time when not active.

Use the `dht.getSample()` method above to query the sensor. It will take 24 milliseconds normally, but could take up to 9 seconds to get a result.


## Version History

#### 0.0.4 (2025-10-03)

- Fixed a linker error for nrfx_i2s_init, nrfx_i2s_stop, and others with Device OS 6.3.3 and later.

#### 0.0.3 (2024-11-04)

- Fix compile error on newer versions of Device OS.

#### 0.0.2 (2019-12-17)

- Support for DHT11. Actually it worked before, but it wasn't tested and there wasn't an example for it.

#### 0.0.1 (2019-12-13)

- Initial version! There may be bugs still.

