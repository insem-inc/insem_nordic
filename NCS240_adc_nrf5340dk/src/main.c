/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/drivers/adc.h>
#include <hal/nrf_saadc.h>

static const struct device *adc_dev;

#define ADC_DEVICE_NAME DT_ADC_0_NAME
#define ADC_RESOLUTION 10
#define ADC_GAIN ADC_GAIN_1_6
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_1ST_CHANNEL_ID 0
#define ADC_1ST_CHANNEL_INPUT NRF_SAADC_INPUT_AIN0

static const struct adc_channel_cfg m_1st_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_1ST_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_1ST_CHANNEL_INPUT,
#endif
};

#define BUFFER_SIZE 1
static int16_t m_sample_buffer[BUFFER_SIZE];

static int adc_sample(void)
{
	int ret;
	float adc_voltage = 0;

	const struct adc_sequence sequence = {
		.channels = BIT(ADC_1ST_CHANNEL_ID),
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution = ADC_RESOLUTION,
	};

	if (!adc_dev) {
		return -1;
	}

	ret = adc_read(adc_dev, &sequence);
	printk("ADC read err: %d\n", ret);

	/* Print the AIN0 values */
	for (int i = 0; i < BUFFER_SIZE; i++) {
		adc_voltage = (float)(((float)m_sample_buffer[i] / 1023.0f) * 3600.0f);
		printk("ADC raw value: %d\n", m_sample_buffer[i]);
		printf("Measured voltage: %f mV\n", adc_voltage);
	}

	return ret;
}

int main(void)
{
	int err;

	printk("nRF5340 saadc sampling AIN0 (P0.04)\n");

	adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));
	if (!adc_dev) {
		printk("device_get_binding ADC_0 failed\n");
	}

	err = adc_channel_setup(adc_dev, &m_1st_channel_cfg);
	if (err) {
		printk("Error in adc setup: %d\n", err);
	}

	/* Trigger offset calibration
	 * As this generates a _DONE and _RESULT event
	 * the first result will be incorrect.
	 */
#if (CONFIG_BUILD_WITH_TFM)
	NRF_SAADC_NS->TASKS_CALIBRATEOFFSET = 1;
#else
	NRF_SAADC_S->TASKS_CALIBRATEOFFSET = 1; 
#endif
	while (1) {
		err = adc_sample();
		if (err) {
			printk("Error in adc sampling: %d\n", err);
		}
		k_sleep(K_MSEC(1000));
	}
}