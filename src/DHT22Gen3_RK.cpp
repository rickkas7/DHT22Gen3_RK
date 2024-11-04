#include "DHT22Gen3_RK.h"

// Repository: https://github.com/rickkas7/DHT22Gen3_RK
// License: MIT

// Must undefine this or the direct nRF52 libraries won't compile as there is a struct member SCK
#undef SCK

#include "nrf_gpio.h"
#include "nrfx_i2s.h"

// Needed for log() to calculate dewpoint.
#include <math.h>

static const size_t NUM_SAMPLES = 180;
static volatile int buffersRequested = 0;
static uint16_t sampleBuffer[NUM_SAMPLES];
static nrfx_i2s_buffers_t i2sBuffer = {
		.p_rx_buffer = (uint32_t *)sampleBuffer,
		.p_tx_buffer = 0
};

DHTSensorTypeDHT11 DHT22Gen3::sensorTypeDHT11;
DHTSensorTypeDHT22 DHT22Gen3::sensorTypeDHT22;


static float combineBytes(uint8_t highByte, uint8_t lowByte) {
	return (float) ((((uint16_t)highByte) << 8) | lowByte);
}

static void dataHandler(nrfx_i2s_buffers_t const *p_released, uint32_t status) {
	if (status == NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED) {
		buffersRequested++;
		if (buffersRequested >= 2) {
			nrfx_i2s_stop();
		}
	}
}

//
// Sensor type decoders
//
DHTSensorTypeDHT11::DHTSensorTypeDHT11() : DHTSensorType("DHT11", 1000, 25) {

};

float DHTSensorTypeDHT11::getTempC(const DHTSample &sample) const {
	return (float) ((int8_t) sample[2]);
}

float DHTSensorTypeDHT11::getHumidity(const DHTSample &sample) const {
	return (float) ((int8_t) sample[0]);
}


DHTSensorTypeDHT22::DHTSensorTypeDHT22() : DHTSensorType("DHT22", 2000, 25) {

};

float DHTSensorTypeDHT22::getTempC(const DHTSample &sample) const {
	return (sample[2] & 0x80 ? -combineBytes(sample[2] & 0x7F, sample[3]) : combineBytes(sample[2], sample[3])) * 0.1;

}

float DHTSensorTypeDHT22::getHumidity(const DHTSample &sample) const {
	return combineBytes(sample[0], sample[1]) * 0.1;
}

//
// Sample result container
//
DHTSample::DHTSample() {

}

DHTSample::~DHTSample() {

}

void DHTSample::clear() {
	sampleResult = SampleResult::ERROR;
	bytes[5] = {0};
	tries = 0;
}


bool DHTSample::isValidChecksum() const {
	uint8_t sum = bytes[0] + bytes[1] + bytes[2] + bytes[3];
	return (sum == bytes[4]);
}


float DHTSample::getTempC() const {
	return sensorType->getTempC(*this);
}

float DHTSample::getTempF() const {

	return (getTempC() * 9) / 5 + 32.0;
}


float DHTSample::getHumidity() const {
	return sensorType->getHumidity(*this);
}

float DHTSample::getDewPointC() const {
	// http://en.wikipedia.org/wiki/Dew_point

	double temp = (double) getTempC();

	double a = 17.271;
	double b = 237.7;
	double adjTemp = (a * temp) / (b + temp) + log(getHumidity() / 100.0);
	double Td = (b * adjTemp) / (a - adjTemp);

	return Td;
}


float DHTSample::getDewPointF() const {
	return (getDewPointC() * 9) / 5 + 32.0;
}

//
// Main class
//

DHT22Gen3::DHT22Gen3(pin_t unusedPin1, pin_t unusedPin2) : unusedPin1(unusedPin1), unusedPin2(unusedPin2) {

}

DHT22Gen3::~DHT22Gen3() {
}

void DHT22Gen3::setup() {
	attachInterruptDirect(I2S_IRQn, nrfx_i2s_irq_handler, false);
}

void DHT22Gen3::loop() {
	switch(state) {
	case State::IDLE_STATE:
		break;

	case State::START_STATE:
		if (lastRequestTime != 0 && millis() - lastRequestTime < sensorType->minSamplePeriodMs) {
			// Not time to check yet, wait a bit
			break;
		}

		result.clear();

		// Can sample now
		pinMode(unusedPin1, OUTPUT); // SCK
		pinMode(unusedPin2, OUTPUT); // LRCK

		// Because it was in INPUT mode before and there is an external pull-up it was already high
		pinMode(dhtPin, OUTPUT);

		// Low for 18 ms
		digitalWrite(dhtPin, LOW);
		stateTime = millis();
		state = State::SEND_START_STATE;
		break;

	case State::SEND_START_STATE:
		if (millis() - stateTime < 18) {
			// Stay in SEND_START_STATE for 18 milliseconds
			break;
		}

		{
			// Go into input mode; the pull-up will keep it high
			pinMode(dhtPin, INPUT);

			// We let the pull-up pull the pin high again, and it should stay that way for 20-40 us then the device takes over
			Hal_Pin_Info *pinMap =
#if (SYSTEM_VERSION < SYSTEM_VERSION_ALPHA(5, 0, 0, 1))
        		HAL_Pin_Map();
#elif (SYSTEM_VERSION >= SYSTEM_VERSION_ALPHA(6, 2, 0, 0))
        		hal_pin_map();
#else // SYSTEM_VERSION
        		Hal_Pin_Map();
#endif // SYSTEM_VERSION			

			nrfx_i2s_config_t config = NRFX_I2S_DEFAULT_CONFIG;

			// Log.info("Got %02x Expected %02x", NRF_GPIO_PIN_MAP(pinMap[dhtPin].gpio_port, pinMap[dhtPin].gpio_pin), NRF_GPIO_PIN_MAP(0, 29));

			config.sdin_pin = (uint8_t)NRF_GPIO_PIN_MAP(pinMap[dhtPin].gpio_port, pinMap[dhtPin].gpio_pin);
			config.sck_pin = (uint8_t)NRF_GPIO_PIN_MAP(pinMap[unusedPin1].gpio_port, pinMap[unusedPin1].gpio_pin);
			config.lrck_pin = (uint8_t)NRF_GPIO_PIN_MAP(pinMap[unusedPin2].gpio_port, pinMap[unusedPin2].gpio_pin);
			config.mck_pin = NRFX_I2S_PIN_NOT_USED;
			config.sdout_pin = NRFX_I2S_PIN_NOT_USED;

			config.mode = NRF_I2S_MODE_MASTER;
			config.format = NRF_I2S_FORMAT_I2S;
			config.alignment = NRF_I2S_ALIGN_LEFT;
			config.sample_width = NRF_I2S_SWIDTH_16BIT;
			config.channels = NRF_I2S_CHANNELS_STEREO;
			config.mck_setup = NRF_I2S_MCK_32MDIV63;
			config.ratio = NRF_I2S_RATIO_32X;

			// These settings are for 16,000 samples per second, 16-bit, stereo
			// That means that we have 16,000 32-bit words per second or 512,000 bits per second

			nrfx_err_t err = nrfx_i2s_init(&config, dataHandler);
			if (err != NRFX_SUCCESS) {
				Log.info("nrfx_i2s_init error=%lu", err);
				callCompletion(DHTSample::SampleResult::ERROR);
				return;
			}

			buffersRequested = 0;

			// Sample data. The / 2 factor because the parameter is the number of 32-bit words, not number of 16-bit samples!
			err = nrfx_i2s_start(&i2sBuffer, NUM_SAMPLES / 2, 0);
			if (err != NRFX_SUCCESS) {
				Log.info("nrfx_i2s_start error=%lu", err);
				callCompletion(DHTSample::SampleResult::ERROR);
				return;
			}
		}

		result.tries++;
		stateTime = millis();
		state = State::SAMPLING_STATE;
		break;

	case State::SAMPLING_STATE:
		if (buffersRequested < 2 && millis() - stateTime < 15) {
			// Wait for samples to complete
			break;
		}

		// uninitialize the I2S peripheral
		nrfx_i2s_uninit();

		if (buffersRequested < 2) {
			// This means the I2S peripheral is in a weird and unknown state (not related to the sensor)
			callCompletion(DHTSample::SampleResult::ERROR);
			return;
		}

		lastRequestTime = millis();


		// Decode samples
		bool prev = true;
		int count = 0;
		int pair = -2; // This is for the start sequence
		memset(result.bytes, 0, sizeof(result.bytes));

		for(size_t ii = 0; ii < NUM_SAMPLES; ii++) {
			uint16_t value = sampleBuffer[ii];
			for(int bit = 15; bit >= 0; bit--) {
				bool bitValue = ((value & (1 << bit)) != 0);
				if (bitValue == prev) {
					count++;
				}
				else {
					// Log.info("%d bit count=%d", prev, count);
					if (prev == 1) {
						if (pair >= 0 && pair < 40) {
							// Normal 0 bit is count = 13
							// Normal 1 bit is count = 37
							bool b = (count > sensorType->oneBitThreshold);

							if (b) {
								int byteIndex = pair / 8;
								int bitIndex = pair % 8;
								result.bytes[byteIndex] |= 1 << (7 - bitIndex);
							}
						}
						pair++;
					}

					count = 1;
					prev = bitValue;
				}
			}
		}

		if (pair == 40) {
			// Log.info("result.bytes = %02x %02x %02x %02x %02x", result.bytes[0], result.bytes[1], result.bytes[2], result.bytes[3], result.bytes[4]);

			if (result.isValidChecksum()) {
				callCompletion(DHTSample::SampleResult::SUCCESS);
				return;
			}
			else {
				// Bad checksum
				Log.info("bad checksum");
			}
		}
		else {
			Log.info("pairs=%d expected 40", pair);
		}

		if (result.tries >= maxTries) {
			callCompletion(DHTSample::SampleResult::TOO_MANY_RETRIES);
			return;
		}

		// Corrupted data, retry
		Log.info("retrying");
		stateTime = millis();
		state = State::START_STATE;
		break;
	}
}


void DHT22Gen3::getSample(pin_t dhtPin, std::function<void(DHTSample)> completion, DHTSensorType *sensorType) {
	if (state != State::IDLE_STATE) {
		DHTSample tempResult;
		tempResult.withBusy();
		if (completion) {
			completion(tempResult);
		}
		return;
	}

	this->dhtPin = dhtPin;
	this->completion = completion;
	this->sensorType = sensorType;
	result.tries = 0;
	result.sensorType = sensorType;
	state = State::START_STATE;

}

void DHT22Gen3::callCompletion(DHTSample::SampleResult sampleResult) {
	result.sampleResult = sampleResult;
	state = State::IDLE_STATE;
	if (completion) {
		completion(result);
	}
}





