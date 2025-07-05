#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "rs485.h"
#include "sdm220.h"

#define READ_INPUT_REGISTERS 4

typedef struct {
    uint8_t hi_byte;
    uint8_t low_byte;
} InputRegisterAddress;

typedef struct __attribute__((aligned (1))) {
    uint8_t slave_address;
    uint8_t function;
    uint8_t start_address_hi;
    uint8_t start_address_low;
    uint8_t quantity_hi;
    uint8_t quantity_low;
    uint8_t crc_low;
    uint8_t crc_hi;
} QueryReadInputRegisters;

typedef enum {
    INPUT_REG_VOLTAGE = 0,
    INPUT_REG_CURRENT,
    INPUT_REG_ACTIVE_POWER,
    INPUT_REG_APPARENT_POWER,
    INPUT_REG_REACTIVE_POWER,
    INPUT_REG_POWER_FACTOR,
    INPUT_REG_PHASE_ANGLE,
    INPUT_REG_FREQUENCY,
    INPUT_REG_IMPORT_ACTIVE_ENERGY,
    INPUT_REG_EXPORT_ACTIVE_ENERGY,
    INPUT_REG_IMPORT_REACTIVE_ENERGY,
    INPUT_REG_EXPORT_REACTIVE_ENERGY,
    INPUT_REG_TOTAL_ACTIVE_ENERGY,
    INPUT_REG_TOTAL_REACTIVE_ENERGY,

    N_INPUT_REGISTERS
} InputRegister;

enum {
    STATE_INVALID = -1,
    STATE_BEGIN_QUERY = 0,
    STATE_READ_MODBUS_HEADER,
    STATE_READ_MODBUS_BODY
};

static InputRegisterAddress input_registers[N_INPUT_REGISTERS] = {
    [INPUT_REG_VOLTAGE]                 = {0,   0},
    [INPUT_REG_CURRENT]                 = {0,   0x6},
    [INPUT_REG_ACTIVE_POWER]            = {0,   0xc},
    [INPUT_REG_APPARENT_POWER]          = {0,   0x12},
    [INPUT_REG_REACTIVE_POWER]          = {0,   0x18},
    [INPUT_REG_POWER_FACTOR]            = {0,   0x1e},
    [INPUT_REG_PHASE_ANGLE]             = {0,   0x24},
    [INPUT_REG_FREQUENCY]               = {0,   0x46},
    [INPUT_REG_IMPORT_ACTIVE_ENERGY]    = {0,   0x48},
    [INPUT_REG_EXPORT_ACTIVE_ENERGY]    = {0,   0x4a},
    [INPUT_REG_IMPORT_REACTIVE_ENERGY]  = {0,   0x4c},
    [INPUT_REG_EXPORT_REACTIVE_ENERGY]  = {0,   0x4e},
    [INPUT_REG_TOTAL_ACTIVE_ENERGY]     = {1,   0x56},
    [INPUT_REG_TOTAL_REACTIVE_ENERGY]   = {1,   0x58}
};

uint16_t crc16(uint8_t *data, size_t data_size)
{
    size_t i = 0u;
    size_t j = 0u;
    uint16_t crc = 0xffffu;

    for (; i < data_size; ++i) {
        crc ^= data[i];
        for (j = 0u; j < 8u; ++j) {
            if ((crc & 1u) != 0u) {
                crc >>= 1;
                crc ^= 0xa001u;
            } else
                crc >>= 1;
        }
    }

    return crc;
}

static inline double parse_ieee754_be(uint8_t *bytes)
{
    union {
        uint32_t uint_value;
        float float_value;
    } ieee754_repr = {0, };

    ieee754_repr.uint_value |= bytes[0] << 24;
    ieee754_repr.uint_value |= bytes[1] << 16;
    ieee754_repr.uint_value |= bytes[2] << 8;
    ieee754_repr.uint_value |= bytes[3];

    return (double) ieee754_repr.float_value;
}

static bool read_byte_impl(InputStream *istream, uint8_t *byte)
{
    return rs485_read_byte_nonblocking(byte);
}

static bool poll_impl(InputStream *istream)
{
    return rs485_available();
}

void sdm220_meter_init(Sdm220Meter *self, uint8_t addr)
{
    input_stream_init(&self->istream, read_byte_impl, poll_impl);

    memset(self->buffer, 0, SDM220_BUFFER_SIZE);
    memset(self->value_table, 0, SDM220_VALUE_TABLE_SIZE * sizeof(double));

    self->slave_address = addr;
    self->buffer_size = 0u;
    self->data_size = 0u;
    self->state = STATE_INVALID;
    self->next_input_register = -1;
    self->error_flag = false;
    self->timeout = 0u;
    self->error_callback = NULL;
    self->ready_callback = NULL;
    self->user_data = NULL;
}

static inline void read_input_register_begin(Sdm220Meter *self, InputRegister reg)
{
    uint16_t crc = 0u;
    uint8_t *query_buf = NULL;
    QueryReadInputRegisters query = {0, };

    query_buf = (uint8_t *) &query;

    query.slave_address = self->slave_address;
    query.function = READ_INPUT_REGISTERS;
    query.start_address_hi = input_registers[reg].hi_byte;
    query.start_address_low = input_registers[reg].low_byte;
    query.quantity_hi = 0;
    query.quantity_low = 2;

    crc = crc16(query_buf, 6);

    query.crc_low = (uint8_t) (crc & 0xff);
    query.crc_hi = (uint8_t) ((crc >> 8) & 0xff);

    rs485_write(query_buf, sizeof(query) / sizeof(uint8_t));
}

static void on_data_ready(InputStream *istream, RuntimeError *error,
                          uint8_t *buf, size_t size, void *user_data)
{
    uint16_t crc = 0u;
    size_t resp_size = 0u;
    Sdm220Meter *self = NULL;

    self = user_data;

    switch (self->state) {
    case STATE_READ_MODBUS_HEADER:
        if (size == 3 && self->buffer[2] != 0) {
            self->data_size = (size_t) (self->buffer[2] + 2u);
            self->state = STATE_READ_MODBUS_BODY;
        } else {
            /*
             * TODO: Handle Error: Bad response
             */
        }
        break;

    case STATE_READ_MODBUS_BODY:
        if (size == self->data_size) {
            resp_size = self->data_size + 3u;

            crc = crc16(self->buffer, resp_size - 2u);
            if (self->buffer[resp_size-2] == (uint8_t) (crc & 0xff)
                && self->buffer[resp_size-1] == (uint8_t) ((crc >> 8) & 0xff)) {

                self->value_table[self->next_input_register] = parse_ieee754_be(buf);
            } else {
                /*
                 * TODO: Handle error: Bad checksum
                 */
            }

            self->next_input_register++;
            self->state = STATE_BEGIN_QUERY;

            if (self->next_input_register == N_INPUT_REGISTERS) {
                if (!self->error_flag) {
                    self->ready_callback(self, self->user_data);
                }

                self->error_callback = NULL;
                self->ready_callback = NULL;
            }
        } else {
            /*
             * TODO: Handle Error: Bad response
             */
        }
        break;

    default:
        break;
    }
}

void sdm220_meter_iterate(Sdm220Meter *self)
{
    InputStream *istream = NULL;

    if (!sdm220_meter_async_poll_pending(self))
        return;

    istream = &self->istream;

    if (input_stream_pending(istream)) {
        input_stream_run(istream);
        return;
    }

    switch (self->state) {
    case STATE_BEGIN_QUERY:
        read_input_register_begin(self, self->next_input_register);
        self->state = STATE_READ_MODBUS_HEADER;
        break;

    case STATE_READ_MODBUS_HEADER:
        input_stream_read_async(istream, self->timeout,
                                self->buffer,
                                3,
                                on_data_ready, self);
        break;

    case STATE_READ_MODBUS_BODY:
        input_stream_read_async(istream, self->timeout,
                                self->buffer + 3,
                                self->data_size,
                                on_data_ready, self);
        break;

    default:
        break;
    }
}

bool sdm220_meter_async_poll_pending(Sdm220Meter *self)
{
    return self->ready_callback != NULL;
}

bool sdm220_meter_poll_async(Sdm220Meter *self,
			                 unsigned timeout,
			                 Sdm220MeterErrorCallback error_callback,
			                 Sdm220MeterReadyCallback ready_callback,
			                 void *user_data)
{
    if (sdm220_meter_async_poll_pending(self))
        return false;

    self->next_input_register = INPUT_REG_VOLTAGE;
    self->state = STATE_BEGIN_QUERY;

    self->timeout = timeout;
    self->error_callback = error_callback;
    self->ready_callback = ready_callback;
    self->user_data = user_data;

    return true;
}

double sdm220_meter_get_voltage(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_VOLTAGE];
}

double sdm220_meter_get_current(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_CURRENT];
}

double sdm220_meter_get_active_power(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_ACTIVE_POWER];
}

double sdm220_meter_get_apparent_power(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_APPARENT_POWER];
}

double sdm220_meter_get_reactive_power(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_REACTIVE_POWER];
}

double sdm220_meter_get_power_factor(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_POWER_FACTOR];
}

double sdm220_meter_get_phase_angle(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_PHASE_ANGLE];
}

double sdm220_meter_get_frequency(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_FREQUENCY];
}

double sdm220_meter_get_import_active_energy(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_IMPORT_ACTIVE_ENERGY];
}

double sdm220_meter_get_export_active_energy(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_EXPORT_ACTIVE_ENERGY];
}

double sdm220_meter_get_import_reactive_energy(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_IMPORT_REACTIVE_ENERGY];
}

double sdm220_meter_get_export_reactive_energy(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_EXPORT_REACTIVE_ENERGY];
}

double sdm220_meter_get_total_active_energy(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_TOTAL_ACTIVE_ENERGY];
}

double sdm220_meter_get_total_reactive_energy(Sdm220Meter *self)
{
    return self->value_table[INPUT_REG_TOTAL_REACTIVE_ENERGY];
}

