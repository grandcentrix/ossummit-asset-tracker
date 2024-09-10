#include "payload.h"

#include <zephyr/data/json.h>
#include <zephyr/logging/log.h>

#include <stdio.h>

LOG_MODULE_REGISTER(thingsboard_data, CONFIG_TBAT_LOG_LEVEL);

// Encoding a struct with floating point values is complicated with Zephyr's json library.
// Instead of struct gps_fix we use this struct for encoding which contains latitude and longitude
// as strings.
struct json_gps_fix {
	char *latitude;
	char *longitude;
	const char *gps_timestamp;
};

struct json_obj_descr json_gps_fix_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct json_gps_fix, latitude, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_gps_fix, longitude, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct json_gps_fix, gps_timestamp, JSON_TOK_STRING),
};

int payload_encode_gps_fix(const struct gps_fix *gps_fix, char *json_buf, size_t json_buf_len)
{
	int err;
	struct json_gps_fix json_gps_fix;
	char buf_latitude[11] = {0};
	char buf_longitude[11] = {0};

	json_gps_fix.latitude = buf_latitude;
	json_gps_fix.longitude = buf_longitude;
	json_gps_fix.gps_timestamp = gps_fix->timestamp;

	sprintf(json_gps_fix.latitude, "%3.07f", gps_fix->latitude);
	sprintf(json_gps_fix.longitude, "%3.07f", gps_fix->longitude);

	err = json_obj_encode_buf(json_gps_fix_descr, ARRAY_SIZE(json_gps_fix_descr), &json_gps_fix,
				  json_buf, json_buf_len);
	if (err) {
		LOG_ERR("Failed to encode gps fix as JSON, error: %s (%d)", strerror(-err), err);
		return err;
	}

	return 0;
}
