//============================================================================
// Name        : OOK_tx.cpp
// Author      : Jonathan C. Brandenburg
// Version     :
// Copyright   : (C) Copyright 2019, Jonathan C. Brandenburg
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <blocks/StreamSinkBlock.h>
#include <iostream>
#include <complex>

#include "blocks/ConstantSourceBlock.h"
#include "blocks/RepeatBlock.h"
#include "blocks/VectorSourceBlock.h"
#include "blocks/SignalSourceBlock.h"
#include "blocks/ToComplexBlock.h"
#include "blocks/MultiplyBlock.h"
#include "blocks/ThrottleBlock.h"

using namespace std;

const unsigned RUNTIME_SECONDS = 10;           /**< The number of seconds the application will run before automatically terminating */
const long double SAMPLE_RATE = 8000000;       /**< The theoretical sampling rate of the application */
const long double CARRIER_FREQUENCY = 2000000; /**< The frequency of the carrier wave */
const long double CARRIER_AMPLITUDE = 1000;    /**< The maximum positive and negative range of the generated carrier wave */
const unsigned REPEAT_COUNT = 800;             /**< The number of teams each value in the input vector will be repeated */

float data[] = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; /**< The values used by the input source */

/**
 * \brief A sample application demonstrating on-off keying.
 */
int main() {
	sdr::blocks::VectorSourceBlock<float> vectorSource;

	vectorSource.setData(data, sizeof(data) / sizeof(data[0]));
	vectorSource.setRepeat(true);

	sdr::blocks::RepeatBlock<float> repeatBlock;

	repeatBlock.setRepeatCount(REPEAT_COUNT);

	sdr::blocks::ConstantSourceBlock<float> constantSource;
	float constantSourceValue = 0;
	constantSource.setValue(&constantSourceValue);

	sdr::blocks::ToComplexBlock<float> toComplexBlock;

	sdr::blocks::SignalSourceBlock<complex<float>> signalSourceBlock;
	signalSourceBlock.setSamplingFrequency(SAMPLE_RATE);
	signalSourceBlock.setSignalFrequency(CARRIER_FREQUENCY);
	signalSourceBlock.setAmplitude(CARRIER_AMPLITUDE);
	signalSourceBlock.setWaveform(sdr::radio::Waveform::WAVEFORM_COS);
	signalSourceBlock.setOffset(complex<float>(0.0, 0.0));

	sdr::blocks::MultiplyBlock<complex<float>> multiplyBlock;

	sdr::blocks::ThrottleBlock<complex<float>> throttleBlock;
	throttleBlock.setSamplingFrequency(SAMPLE_RATE);

	sdr::blocks::StreamSinkBlock<complex<float>> fileSinkBlock;
	ofstream output_file;
	output_file.open("rfdata.out", ios::binary | ios::trunc);
	fileSinkBlock.setStream(&output_file);



	unsigned long iterationCount = 0;

	time_t startTime;
	startTime = time(NULL);

	time_t endTime = startTime + RUNTIME_SECONDS;

	float *vectorValue, *repeatValue, *constantValue;
	complex<float> *floatToComplexValue, *signalSourceValue, *throttleValue, *multiplyValue;

	for(;;) {
		iterationCount++;
		if (time(NULL) >= endTime) {
			break;
		}

		if (!repeatBlock.isOutputAvailable()) {
			vectorValue = vectorSource.process();
			if (vectorValue == nullptr) {
				cout << " [Reached end of source data]" << endl;
				break;
			}
			repeatBlock.setValue(vectorValue);
		}

		repeatValue = repeatBlock.process();

		constantValue = constantSource.process();

		toComplexBlock.setReal(repeatValue);
		toComplexBlock.setImag(constantValue);
		floatToComplexValue = toComplexBlock.process();
		
		signalSourceValue = signalSourceBlock.process();

		throttleBlock.setValue(signalSourceValue);
		throttleValue = throttleBlock.process();

		multiplyBlock.setLeft(floatToComplexValue);
		multiplyBlock.setRight(throttleValue);
		multiplyValue = multiplyBlock.process();

		fileSinkBlock.setValue(multiplyValue);
		fileSinkBlock.process();
	}

	cout << "Executed " << iterationCount << " iterations in " << RUNTIME_SECONDS << " seconds." << endl;

	return EXIT_SUCCESS;
}
