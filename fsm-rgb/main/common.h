#include "sdkconfig.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

#define STACK_SIZE 3584

struct gpio_led {
	gpio_num_t pin;
	char *name;
	int state;
};

typedef enum {
	INIT_STATE,
        RED_STATE,
        GREEN_STATE,
        GREEN_FLASH_STATE,
        YELLOW_STATE,
} tl_state_t;

typedef enum {
	OFF,
	RED_COLOR,
        GREEN_COLOR,
        BLUE_COLOR,
        YELLOW_COLOR,
} color_t;


