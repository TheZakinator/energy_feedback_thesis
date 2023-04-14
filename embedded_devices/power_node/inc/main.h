#ifndef MAIN_H
#define MAIN_H

#include <zephyr.h>
#include <stdint.h>

typedef struct {
    uint8_t applianceOn;
    uint8_t hysterisisLevel;
    uint8_t nodeNum;
    uint32_t voltageVal;
} PowerNodeData_t;


void get_dummy_data(const char* deviceName, PowerNodeData_t* powerNodeData, int powerState);

void main(void);

#endif