#ifndef _DHT22GEN3_RK
#define _DHT22GEN3_RK

#include "Particle.h"

// Repository: https://github.com/rickkas7/DHT22Gen3_RK
// License: MIT

class DHTSample; // Forward declaration

/**
 * @brief Abstract base class for all types of sensor
 */
class DHTSensorType {
public:
	/**
	 * @brief Constructor for a sensor type
	 *
	 * @param name Short descriptive name (DHT11, DHT22)
	 *
	 * @param minSamplePeriodMs Minimum number of milliseconds between queries to the sensor
	 *
	 * @param oneBitThreshold Number of counts for the bit in the I2S buffer to be considered a 1 bit
	 */
	DHTSensorType(const char *name, unsigned long minSamplePeriodMs, unsigned long oneBitThreshold) :
			name(name), minSamplePeriodMs(minSamplePeriodMs), oneBitThreshold(oneBitThreshold) {};

	/**
	 * @brief Destructor
	 */
	virtual ~DHTSensorType() {};

	/**
	 * @brief For the sample, convert it into degrees C based on the sensor type
	 *
	 * @param sample The sample data to convert
	 */
	virtual float getTempC(const DHTSample &sample) const = 0;

	/**
	 * @brief For the sample, convert it into percent humidity (0-100) based on the sensor type
	 *
	 * @param sample The sample data to convert
	 */
	virtual float getHumidity(const DHTSample &sample) const = 0;


	const char *name;					//!< Short name of sensor
	unsigned long minSamplePeriodMs; 	//!< Minimum period between samples.
	int oneBitThreshold;				//!< Threshold in number of I2S bits for a 1-bit
};

/**
 * @brief DHTSensorType object to decode DHT11 sensor data
 */
class DHTSensorTypeDHT11 : public DHTSensorType {
public:
	/**
	 * @brief Constructor for DHT11 sensor type.
	 *
	 * You normally don't need to instantiate one of these; there's one pre-allocated in
	 * DHT22Gen3::sensorTypeDHT11.
	 */
	DHTSensorTypeDHT11();

	/**
	 * @brief For the sample, convert it into degrees C based on the sensor type
	 *
	 * @param sample The sample data to convert
	 */
	virtual float getTempC(const DHTSample &sample) const;

	/**
	 * @brief For the sample, convert it into percent humidity (0-100) based on the sensor type
	 *
	 * @param sample The sample data to convert
	 */
	virtual float getHumidity(const DHTSample &sample) const;
};


/**
 * @brief DHTSensorType object to decode DHT22 sensor data
 */
class DHTSensorTypeDHT22 : public DHTSensorType {
public:
	/**
	 * @brief Constructor for DHT22 sensor type.
	 *
	 * You normally don't need to instantiate one of these; there's one pre-allocated in
	 * DHT22Gen3::sensorTypeDHT22.
	 */
	DHTSensorTypeDHT22();

	/**
	 * @brief For the sample, convert it into degrees C based on the sensor type
	 *
	 * @param sample The sample data to convert
	 */
	virtual float getTempC(const DHTSample &sample) const;

	/**
	 * @brief For the sample, convert it into percent humidity (0-100) based on the sensor type
	 *
	 * @param sample The sample data to convert
	 */
	virtual float getHumidity(const DHTSample &sample) const;
};


/**
 * @brief Class for encapsulating the results to a call to getSample()
 */
class DHTSample {
public:
	/**
	 * @brief Enumeration for result codes from the getSample() request.
	 *
	 * Success is 0. All other codes are errors
	 */
	enum class SampleResult {
		SUCCESS = 0,		//!< Success (including valid checksum)
		ERROR,				//!< An internal error (problem with the I2S peripheral, etc.)
		TOO_MANY_RETRIES,	//!< After the specified number of retries, could not get a valid result
		BUSY				//!< Called getSample() before the previous call completed
	};

	/**
	 * @brief Constructor
	 *
	 * You normally don't need to construct one of these; it's filled in for you by getSample()
	 */
	DHTSample();


	virtual ~DHTSample(); //!< Destructor

	/**
	 * @brief Clears the results
	 *
	 * Sets the result to ERROR, zeros the bytes, sets tries to 0
	 */
	void clear();

	/**
	 * @brief Returns true if the checksum in bytes is valid
	 *
	 * You normally should check isSuccess() since that will only be set if the checksum is valid
	 * and normally don't need to call this directly.
	 */
	bool isValidChecksum() const;

	/**
	 * @brief Gets the temperature in degrees Celsius
	 *
	 * The value is undefined is isSuccess() is not true. You you check that before relying on this value.
	 */
	float getTempC() const;

	/**
	 * @brief Gets the temperature in degrees Fahrenheit
	 */
	float getTempF() const;

	/**
	 * @brief Gets the humidity in percent RH (0-100) as a floating point number
	 *
	 * The value is undefined is isSuccess() is not true. You you check that before relying on this value.
	 */
	float getHumidity() const;

	/**
	 * @brief Gets the dew point in degrees Celsius
	 *
	 * The value is undefined is isSuccess() is not true. You you check that before relying on this value.
	 */
	float getDewPointC() const;

	/**
	 * @brief Gets the dew point in degrees Fahrenheit
	 *
	 * The value is undefined is isSuccess() is not true. You you check that before relying on this value.
	 */
	float getDewPointF() const;

	/**
	 * @brief Gets the sample result value (0 = success, non-zero = error)
	 *
	 * You may want to use isSuccess(), isBusy(), isError(), or isTooManyRetries() instead.
	 */
	SampleResult getSampleResult() const { return sampleResult; };

	/**
	 * @brief Gets the number of retries
	 *
	 * @return 1 in the normal case. If a checksum error occurred, numbers > 1 indicate retries, up to
	 * the configured number of maxRetries
	 */
	int getTries() const { return tries; };

	/**
	 * @brief Sets the sample result to SUCCESS
	 */
	DHTSample &withSuccess() { sampleResult = SampleResult::SUCCESS; return *this; };

	/**
	 * @brief Returns true if getSample() was successful.
	 *
	 * The call completed successfully and the checksum passed. The temperature and humidity
	 * values should be valid.
	 */
	bool isSuccess() const { return sampleResult == SampleResult::SUCCESS; };

	/**
	 * @brief Sets the sample result to BUSY
	 */
	DHTSample &withBusy() { sampleResult = SampleResult::BUSY; return *this; };

	/**
	 * @brief Returns true if getSample() failed because another call was in progress
	 */
	bool isBusy() const { return sampleResult == SampleResult::BUSY; };

	/**
	 * @brief Sets the sample result to ERROR
	 */
	DHTSample &withError() { sampleResult = SampleResult::ERROR; return *this; };

	/**
	 * @brief Returns true if getSample() failed because of an internal error
	 */
	bool isError() const { return sampleResult == SampleResult::ERROR; };

	/**
	 * @brief Sets the sample result to TOO_MANY_RETRIES
	 */
	DHTSample &withTooManyRetries() { sampleResult = SampleResult::TOO_MANY_RETRIES; return *this; };

	/**
	 * @brief Returns true if getSample() failed because a valid result was not obtained in
	 * the maximum number of retries.
	 */
	bool isTooManyRetries() const { return sampleResult == SampleResult::TOO_MANY_RETRIES; };

	/**
	 * @brief Sets the data format of bytes
	 */
	DHTSample &withSensorType(DHTSensorType *sensorType) { this->sensorType = sensorType; return *this; };

	/**
	 * @brief Get a byte from the bytes array
	 */
	uint8_t operator[](size_t index) const { return bytes[index]; };

protected:
	SampleResult sampleResult = SampleResult::ERROR;	//!< Result code. 0 is success, error are non-zero
	DHTSensorType *sensorType = 0; //!< Sensor type for this sample
	uint8_t bytes[5] = {0};	//!< Raw bytes of data from DHT22
	int tries = 0;	//!< Number of retries. Normally 1 for the initial try, will be greater for retries.
	friend class DHT22Gen3;
};


/**
 * @brief Class for interfacing with one or more DHT22 sensors on a Gen3 Particle device
 *
 * Only allocate one of these, typically as a global variable, per device.
 *
 * Be sure to call the setup() and loop() methods from the actual setup and loop.
 */
class DHT22Gen3 {
public:
	/**
	 * @brief Enumeration for possible states for the Finite State Machine
	 */
	enum class State {
		IDLE_STATE,			//!< Idle, can call getSample()
		START_STATE,		//!< Getting ready to get a sample
		SEND_START_STATE,	//!< Sending the start bit and starting the I2S peripheral
		SAMPLING_STATE		//!< Capturing samples
	};

	/**
	 * @brief Initialize the DHT22Gen3 driver
	 *
	 * @param unusedPin1 An unused GPIO pin to use for SCK. It's unfortunate, but for the I2S
	 * peripheral to work, SCK must be mapped to a valid pin. It will be set to output and
	 * there will be 512 KHz signal output on this pin.
	 *
	 * @param unusedPin2 An unused GPIO pin to use for LRCK. It's unfortunate, but for the I2S
	 * peripheral to work, LRCK must be mapped to a valid pin. It will be set to output and
	 * there will be 32 KHz signal output on this pin. It must be a different pin than unusedPin1.
	 *
	 * - You will often allocate this object as a global variable.
	 * - There can only be one per device.
	 * - Make sure you call setup() and loop() methods from your actual setup() and loop()!
	 */
	DHT22Gen3(pin_t unusedPin1, pin_t unusedPin2);
	virtual ~DHT22Gen3();

	/**
	 * @brief Call from setup() to initialize the driver
	 */
	void setup();

	/**
	 * @brief Call from loop(), preferably every call to loop.
	 *
	 * This allows the completion function to be dispatched from normal loop execution time,
	 * which allows normal calls like Particle.publish to be called safely.
	 */
	void loop();

	/**
	 * @brief Get a sample on the specified pin
	 *
	 * @param dhtPin The pin the sensor is connected to (A0, A1, ..., D0, D1, ...) can also
	 * use special pins when the port is not being use, for example, TX, RX, MISO, MOSI, SCK.
	 *
	 * @param completion A function or C++ lambda to call when the operation completes.
	 *
	 * @param sensorType Optional. Default to &sensorTypeDHT22. Can also be &sensorTypeDHT11.
	 *
	 * This function is asynchronous and the completion function is called when done.
	 *
	 * Normal operation takes 24 milliseconds. If a checksum failure occurs, each retry takes
	 * 2 seconds because the DHT22 cannot get new samples faster than that. So with 4 retries,
	 * it could take about 9 seconds.
	 */
	void getSample(pin_t dhtPin, std::function<void(DHTSample)> completion, DHTSensorType *sensorType = &sensorTypeDHT22);

	/**
	 * @brief Returns true if you can call getSample(). Returns false if another call is still in progress.
	 */
	bool canGetSample() const { return state == State::IDLE_STATE; };

	/**
	 * @brief Gets the last result if you want to poll instead of use the completion function.
	 */
	DHTSample getLastResult() const { return result; };

	/**
	 * @brief Maximum number of attempts to get a valid result (passes checksum). Default is 4.
	 *
	 * Note that each retry takes 2.25 seconds, so you may not want to set the value too high.
	 */
	DHT22Gen3 &withMaxTries(int tries) { this->maxTries = tries; return *this; };

	/**
	 * @brief Pass a pointer to sensorTypeDHT11 to getSamples() for DHT11 sensors
	 */
	static DHTSensorTypeDHT11 sensorTypeDHT11;

	/**
	 * @brief Pass a pointer to sensorTypeDHT22 to getSamples() for DHT22 sensors. This is the default.
	 */
	static DHTSensorTypeDHT22 sensorTypeDHT22;

protected:
	/**
	 * @brief Used internally to call the completion handler
	 *
	 * Goes into IDLE state after calling the completion
	 */
	void callCompletion(DHTSample::SampleResult sampleResult);

	pin_t unusedPin1; //!< Pin to output SCK (not used by DHT22, but unfortunately required by I2S)
	pin_t unusedPin2; //!< Pin to output LRCK (not used by DHT22, but unfortunately required by I2S)

	pin_t dhtPin = 0; //!< Pin to communicate with DHT22. Set by getSample()
	DHTSensorType *sensorType = &sensorTypeDHT22; //!< Sensor type, optional parameter to getSample();

	unsigned long lastRequestTime = 0; //!< millis() value at last request. Used to prevent querying more often than minSamplePeriodMs (2 seconds)
	unsigned long stateTime = 0; //!< millis() value used with state transitions
	int 	maxTries = 4; //!< Maximum number of retries on checksum values. Default is 4. Each retry takes 2.5 seconds.
	State state = State::IDLE_STATE; //!< State of the finite state machine.
	DHTSample result; //!< Result that will be passed to the callback (by value)
	std::function<void(DHTSample)> completion = 0; //!< Completion handler function or lambda. Set by getSample(). May be 0.
};



#endif /* _DHT22GEN3_RK */
