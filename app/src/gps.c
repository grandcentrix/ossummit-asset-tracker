#include "gps.h"

#include <modem/lte_lc.h>
#include <nrf_modem_gnss.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(gps, CONFIG_TBAT_LOG_LEVEL);

static struct nrf_modem_gnss_pvt_data_frame pvt_data;
static struct gps_fix gps_fix;
static gps_fix_callback_t on_gps_fix;

static int gps_fix_new(struct gps_fix *gps_fix, struct nrf_modem_gnss_pvt_data_frame *pvt_data)
{
	int ret;

	gps_fix->timestamp[0] = '\0';
	gps_fix->latitude = 0;
	gps_fix->longitude = 0;

	ret = snprintf(gps_fix->timestamp, sizeof(gps_fix->timestamp),
		       "%04u-%02u-%02uT%02u:%02u:%02uZ", pvt_data->datetime.year,
		       pvt_data->datetime.month, pvt_data->datetime.day, pvt_data->datetime.hour,
		       pvt_data->datetime.minute, pvt_data->datetime.seconds);
	if (ret < 0 || ret == sizeof(gps_fix->timestamp)) {
		return -EINVAL;
	}
	gps_fix->latitude = pvt_data->latitude;
	gps_fix->longitude = pvt_data->longitude;
	return 0;
}

static void handle_gps_event(int event)
{
	int err;

	switch (event) {
	// PVT data available
	case NRF_MODEM_GNSS_EVT_PVT:
		// Count number of satellites
		size_t num_satellites = 0;
		for (size_t i = 0; i < ARRAY_SIZE(pvt_data.sv); i++) {
			if (pvt_data.sv[i].signal != 0) {
				num_satellites++;
			}
		}
		LOG_DBG("Number of current satellites: %d", num_satellites);

		// Read PVT data
		err = nrf_modem_gnss_read(&pvt_data, sizeof(pvt_data), NRF_MODEM_GNSS_DATA_PVT);
		if (err) {
			LOG_ERR("reading PVT data failed, error (%d): %s", err, strerror(-err));
			break;
		}

		if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_DEADLINE_MISSED) {
			LOG_DBG("GPS blocked by LTE activity");
		} else if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME) {
			LOG_DBG("Insufficient GPS time windows");
		} else if (pvt_data.flags & NRF_MODEM_GNSS_PVT_FLAG_FIX_VALID) {
			err = gps_fix_new(&gps_fix, &pvt_data);
			if (err) {
				LOG_ERR("Couldn't save GPS fix, error (%d): %s", err,
					strerror(-err));
			}
			if (on_gps_fix) {
				on_gps_fix(&gps_fix);
			}
		}

		break;
	case NRF_MODEM_GNSS_EVT_PERIODIC_WAKEUP:
		LOG_DBG("GPS has woken up");
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_TIMEOUT:
		LOG_DBG("GPS enters sleep after fix timeout");
		break;
	case NRF_MODEM_GNSS_EVT_SLEEP_AFTER_FIX:
		LOG_DBG("GPS enters sleep after achieving fix");
		break;
	case NRF_MODEM_GNSS_EVT_BLOCKED:
		LOG_DBG("GPS is blocked by LTE activity");
		break;
	default:
		break;
	}
}

int gps_start(gps_fix_callback_t cb, uint16_t fix_interval, uint16_t fix_timeout)
{
	int err;

	on_gps_fix = cb;

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_GNSS);
	if (err) {
		LOG_ERR("Failed to activate GPS, error (%d): %s", err, strerror(-err));
		return err;
	}

	err = nrf_modem_gnss_event_handler_set(handle_gps_event);
	if (err) {
		LOG_ERR("Failed to set GPS event handler, error (%d): %s", err, strerror(-err));
		return err;
	}

	err = nrf_modem_gnss_fix_interval_set(fix_interval);
	if (err) {
		LOG_ERR("Failed to set GPS fix interval, error (%d): %s", err, strerror(-err));
		return err;
	}

	err = nrf_modem_gnss_fix_retry_set(fix_timeout);
	if (err) {
		LOG_ERR("Failed to set GPS fix retry, error (%d): %s", err, strerror(-err));
		return err;
	}

	LOG_INF("Starting GPS");
	err = nrf_modem_gnss_start();
	if (err) {
		LOG_ERR("Failed to start GPS, error (%d): %s", err, strerror(-err));
		return err;
	}

	return 0;
}

#ifdef CONFIG_SHELL

static struct gps_fix fake_gps_fix;

static int cmd_gps(const struct shell *shell, size_t argc, char **argv)
{
	double latitude = atof(argv[1]);
	double longitude = atof(argv[2]);

	if (latitude == 0.0 || longitude == 0.0) {
		shell_print(shell, "Entered GPS coordinates not valid.");
		return -1;
	}

	fake_gps_fix.latitude = latitude;
	fake_gps_fix.longitude = longitude;

	if (strlen(argv[3]) != sizeof(fake_gps_fix.timestamp) - 1) {
		shell_print(
			shell,
			"Timestamp has incorrect length. Expects an ISO8061 time stamp in UTC.");
		shell_print(shell, "For example: 2024-02-12T11:10:37Z");
		return -1;
	}

	strcpy(fake_gps_fix.timestamp, argv[3]);

	if (on_gps_fix) {
		on_gps_fix(&fake_gps_fix);
	}

	return 0;
}

SHELL_CMD_ARG_REGISTER(gps, NULL,
		       "Send a fake GPS position to thingsboard (usage: gps LAT LONG ISO8061_TIME)",
		       cmd_gps, 4, 0);

#endif // CONFIG_SHELL
