#include "lte.h"

#include <dk_buttons_and_leds.h>
#include <modem/lte_lc.h>
#include <zephyr/logging/log.h>

#include <string.h>

LOG_MODULE_REGISTER(lte, CONFIG_TBAT_LOG_LEVEL);

#define LED_LTE DK_LED1

static lte_status_callback_t on_lte_connected;

static void handle_event(const struct lte_lc_evt *const evt);
static void registration_status_changed(enum lte_lc_nw_reg_status status);

void lte_set_connected_callback(lte_status_callback_t cb)
{
	on_lte_connected = cb;
}

int lte_connect(void)
{
	int err;

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_LTE);
	if (err) {
		LOG_ERR("Failed to activate LTE, error (%d): %s", err, strerror(-err));
		return err;
	}

	lte_lc_register_handler(handle_event);

	LOG_INF("Connecting to LTE network");
	err = lte_lc_connect_async(NULL);
	if (err) {
		LOG_ERR("Error in lte_lc_connect_async, error (%d): %s", err, strerror(-err));
		return err;
	}

	return 0;
}

static void handle_event(const struct lte_lc_evt *const evt)
{
	if (evt->type == LTE_LC_EVT_NW_REG_STATUS) {
		registration_status_changed(evt->nw_reg_status);
	}
}

static void log_registration_status(enum lte_lc_nw_reg_status status)
{
	char *status_str;

	switch (status) {
	case LTE_LC_NW_REG_REGISTERED_HOME:
		status_str = "registered - home network";
		break;
	case LTE_LC_NW_REG_REGISTERED_ROAMING:
		status_str = "registered - roaming";
		break;
	case LTE_LC_NW_REG_SEARCHING:
		status_str = "not registered - searching";
		break;
	case LTE_LC_NW_REG_NOT_REGISTERED:
		status_str = "not registered - not searching";
		break;
	case LTE_LC_NW_REG_REGISTRATION_DENIED:
		status_str = "not registered - denied";
		break;
	case LTE_LC_NW_REG_UICC_FAIL:
		status_str = "not registered - UICC failure";
		break;
	case LTE_LC_NW_REG_UNKNOWN:
		status_str = "unknown";
		break;
	default:
		status_str = "-";
		break;
	}
	LOG_INF("Network registration status: %s", status_str);
}

static void registration_status_changed(enum lte_lc_nw_reg_status status)
{
	log_registration_status(status);

	if (status == LTE_LC_NW_REG_REGISTERED_HOME || status == LTE_LC_NW_REG_REGISTERED_ROAMING) {
		dk_set_led_on(LED_LTE);
		if (on_lte_connected) {
			on_lte_connected();
		}
	} else {
		dk_set_led_off(LED_LTE);
	}
}
