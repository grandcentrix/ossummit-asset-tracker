#include "gps.h"
#include "lte.h"
#include "thingsboard.h"
#include "payload.h"

#include <dk_buttons_and_leds.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <string.h>

LOG_MODULE_REGISTER(main, CONFIG_TBAT_LOG_LEVEL);

K_SEM_DEFINE(lte_connected, 0, 1);

struct gps_work_info {
	struct k_work work;
	struct gps_fix gps_fix;
} gps_work_info;

static char payload_buf[1024];

static void lte_connected_cb()
{
	k_sem_give(&lte_connected);
}

void gps_worker(struct k_work *item)
{
	struct gps_work_info *work_info = CONTAINER_OF(item, struct gps_work_info, work);
	int err;

	err = payload_encode_gps_fix(&work_info->gps_fix, payload_buf, sizeof(payload_buf));
	if (err) {
		return;
	}

	LOG_INF("Sending GPS telemetry: %s", payload_buf);

	err = tb_send_telemetry(payload_buf, CONFIG_TBAT_THINGSBOARD_ACCESS_TOKEN);
	if (err) {
		LOG_ERR("Failed to send GPS telemetry, error %d", err);
	}
}

static void gps_fix_cb(const struct gps_fix *gps_fix)
{
	gps_work_info.gps_fix = *gps_fix;
	k_work_submit(&gps_work_info.work);
}

int main(void)
{
	int err;

	if (strlen(CONFIG_TBAT_THINGSBOARD_ACCESS_TOKEN) == 0) {
		LOG_ERR("No access token configured");
		return -1;
	}

	err = dk_leds_init();
	if (err) {
		LOG_ERR("Could not initialize DK LEDs, error (%d): %s", err, strerror(-err));
		return err;
	}

	k_work_init(&gps_work_info.work, gps_worker);

	LOG_INF("Initializing modem library");
	err = nrf_modem_lib_init();
	if (err) {
		LOG_ERR("Failed to initialize the modem library, error (%d): %s", err,
			strerror(-err));
		return err;
	}

	lte_set_connected_callback(&lte_connected_cb);

	err = lte_connect();
	if (err) {
		return err;
	}

	// Wait until modem is connected
	err = k_sem_take(&lte_connected, K_FOREVER);
	if (err) {
		LOG_ERR("Could not wait for LTE connection, error (%d): %s", err, strerror(-err));
		return err;
	}

	err = tb_init(CONFIG_TBAT_THINGSBOARD_HOSTNAME, CONFIG_TBAT_THINGSBOARD_PORT);
	if (err) {
		return err;
	}

	err = gps_start(&gps_fix_cb, CONFIG_TBAT_GPS_PERIODIC_INTERVAL,
			CONFIG_TBAT_GPS_PERIODIC_TIMEOUT);
	if (err) {
		return err;
	}

	LOG_DBG("Initialization complete. Exiting main thread");

	return err;
}
