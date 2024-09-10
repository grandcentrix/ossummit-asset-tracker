#ifndef TBAT_THINGSBOARD_H
#define TBAT_THINGSBOARD_H

#include <stdint.h>

int tb_init(char *hostname, uint16_t port);
int tb_send_telemetry(char *payload, char *access_token);
void tb_close(void);

#endif // TBAT_THINGSBOARD_H
