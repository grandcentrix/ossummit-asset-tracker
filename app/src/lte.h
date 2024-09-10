#ifndef TBAT_LTE_H
#define TBAT_LTE_H

typedef void (*lte_status_callback_t)(void);

void lte_set_connected_callback(lte_status_callback_t cb);
int lte_connect(void);

#endif // TBAT_LTE_H
