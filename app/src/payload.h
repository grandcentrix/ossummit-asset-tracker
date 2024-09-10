#ifndef TBAT_PAYLOAD_H
#define TBAT_PAYLOAD_H

#include <stddef.h>

#include "gps.h"

int payload_encode_gps_fix(const struct gps_fix *gps_fix, char *json_buf, size_t json_buf_len);

#endif // TBAT_PAYLOAD_H
