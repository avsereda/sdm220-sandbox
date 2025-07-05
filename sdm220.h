/**
 * @file input-stream.h
 * @author Sereda Anton <sereda-anton@yandex.ru>
 *
 * Copyright (c) 2017 Sereda Anton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef SDM220_H
#define SDM220_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "input-stream.h"

typedef struct _Sdm220Meter Sdm220Meter;
typedef struct _Sdm220MeterError Sdm220MeterError;
typedef enum _Sdm220MeterErrorCode Sdm220MeterErrorCode;

typedef void (*Sdm220MeterErrorCallback)(Sdm220Meter *, Sdm220MeterError *, void *);
typedef void (*Sdm220MeterReadyCallback)(Sdm220Meter *, void *);

enum _Sdm220MeterErrorCode {
	SDM220_METER_ERROR_CODE_TIMEOUT = 1,
	SDM220_METER_ERROR_CODE_BAD_RESPONSE,
	SDM220_METER_ERROR_CODE_BUFFER_OVERFLOW
};

#define SDM220_BUFFER_SIZE 	16
#define SDM220_VALUE_TABLE_SIZE	14

struct _Sdm220MeterError {
	Sdm220MeterErrorCode code;
};

struct _Sdm220Meter {
	uint8_t slave_address;
	InputStream istream;
	uint8_t buffer[SDM220_BUFFER_SIZE];
	double value_table[SDM220_VALUE_TABLE_SIZE];
	size_t buffer_size;
	size_t data_size;
	int state;
	int next_input_register;
	bool error_flag;
	unsigned timeout;
	Sdm220MeterErrorCallback error_callback;
	Sdm220MeterReadyCallback ready_callback;
	void *user_data;
};

void sdm220_meter_init(Sdm220Meter *self, uint8_t addr);
void sdm220_meter_iterate(Sdm220Meter *self);
bool sdm220_meter_async_poll_pending(Sdm220Meter *self);

bool sdm220_meter_poll_async(Sdm220Meter *self,
			     unsigned timeout,
			     Sdm220MeterErrorCallback error_callback,
			     Sdm220MeterReadyCallback ready_callback,
			     void *user_data);

double sdm220_meter_get_voltage(Sdm220Meter *self);
double sdm220_meter_get_current(Sdm220Meter *self);
double sdm220_meter_get_active_power(Sdm220Meter *self);
double sdm220_meter_get_apparent_power(Sdm220Meter *self);
double sdm220_meter_get_reactive_power(Sdm220Meter *self);
double sdm220_meter_get_power_factor(Sdm220Meter *self);
double sdm220_meter_get_phase_angle(Sdm220Meter *self);
double sdm220_meter_get_frequency(Sdm220Meter *self);
double sdm220_meter_get_import_active_energy(Sdm220Meter *self);
double sdm220_meter_get_export_active_energy(Sdm220Meter *self);
double sdm220_meter_get_import_reactive_energy(Sdm220Meter *self);
double sdm220_meter_get_export_reactive_energy(Sdm220Meter *self);
double sdm220_meter_get_total_active_energy(Sdm220Meter *self);
double sdm220_meter_get_total_reactive_energy(Sdm220Meter *self);

#endif /* SDM220_H */
