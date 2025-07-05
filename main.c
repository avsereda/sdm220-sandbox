#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "timer.h"
#include "rs485.h"
#include "sdm220.h"

#define POLL_TIMEOUT    3000
#define SDM220_ADDRESS 	1

static void on_pwr_meter_error(Sdm220Meter *meter, Sdm220MeterError *error,
                               void *user_data)
{
    fprintf(stderr, "Ooops!!! Something went wrong... Code: %d\n", error->code);
    exit(EXIT_FAILURE);
}

static void on_pwr_meter_ready(Sdm220Meter *meter, void *user_data)
{
    printf("\nSDM220 Data:\n\n");

    printf("%-40s %.2f\n", "Line to neutral volts (V):", sdm220_meter_get_voltage(meter));
    printf("%-40s %.2f\n", "Current (A):", sdm220_meter_get_current(meter));
    printf("%-40s %.2f\n", "Active power (W):", sdm220_meter_get_active_power(meter));
    printf("%-40s %.2f\n", "Apparent power (VA):", sdm220_meter_get_apparent_power(meter));
    printf("%-40s %.2f\n", "Reactive power (VAr):", sdm220_meter_get_reactive_power(meter));
    printf("%-40s %.2f\n", "Power factor (None):", sdm220_meter_get_power_factor(meter));
    printf("%-40s %.2f\n", "Phase angle (Degree):", sdm220_meter_get_phase_angle(meter));
    printf("%-40s %.2f\n", "Frequency (Hz):", sdm220_meter_get_frequency(meter));
    printf("%-40s %.2f\n", "Import active energy (kWh):", sdm220_meter_get_import_active_energy(meter));
    printf("%-40s %.2f\n", "Export active energy (kWh):", sdm220_meter_get_export_active_energy(meter));
    printf("%-40s %.2f\n", "Import reactive energy (kvarh):", sdm220_meter_get_import_reactive_energy(meter));
    printf("%-40s %.2f\n", "Export reactive energy (kvarh):", sdm220_meter_get_export_reactive_energy(meter));
    printf("%-40s %.2f\n", "Total active energy (kWh):", sdm220_meter_get_total_active_energy(meter));
    printf("%-40s %.2f\n", "Total reactive energy (kvarh):", sdm220_meter_get_total_reactive_energy(meter));

    printf("\nAll done.\n");
}

int main(int argc, char **argv)
{
    Sdm220Meter pwr_meter = {0, };

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <tty device>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    rs485_init(argv[1]);
    sdm220_meter_init(&pwr_meter, SDM220_ADDRESS);
    if (sdm220_meter_poll_async(&pwr_meter, POLL_TIMEOUT,
                                on_pwr_meter_error, on_pwr_meter_ready, NULL)) {
        while (sdm220_meter_async_poll_pending(&pwr_meter)) {
            sdm220_meter_iterate(&pwr_meter);
        }
    }

    return 0;
}
