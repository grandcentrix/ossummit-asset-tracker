#ifndef TBAT_GPS_H
#define TBAT_GPS_H

#include <stdint.h>

struct gps_fix {
	double latitude;
	double longitude;
	// ISO8061 formatted string
	char timestamp[21];
};

typedef void (*gps_fix_callback_t)(const struct gps_fix *);

int gps_start(gps_fix_callback_t cb, uint16_t fix_interval, uint16_t fix_timeout);

#endif // TBAT_GPS_H
