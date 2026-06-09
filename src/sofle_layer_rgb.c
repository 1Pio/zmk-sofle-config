/*
 * Layer-aware RGB profiles for the Sofle Choc Pro.
 *
 * Keep layer switching in the keymap and react to the settled layer state here.
 * This avoids press/release RGB flicker from hold-tap macros while still invoking
 * ZMK's global RGB behavior so split underglow locality is preserved.
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <dt-bindings/zmk/rgb.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/rgb_underglow.h>

#define BASE 0
#define LOWER 1
#define RAISE 2
#define ADJUST 3

#define RGB_PROFILE_SETTLE_MS 20

struct layer_rgb_profile {
    uint16_t hue;
    uint8_t saturation;
    uint8_t brightness;
    uint8_t effect;
};

static const struct layer_rgb_profile layer_rgb_profiles[] = {
    [BASE] = {.hue = 3, .saturation = 100, .brightness = 2, .effect = 0},
    [LOWER] = {.hue = 3, .saturation = 100, .brightness = 25, .effect = 0},
    [RAISE] = {.hue = 6, .saturation = 100, .brightness = 10, .effect = 0},
    [ADJUST] = {.hue = 30, .saturation = 95, .brightness = 7, .effect = 0},
};

static struct k_work_delayable layer_rgb_work;
static uint8_t last_applied_layer = UINT8_MAX;

static int invoke_rgb_behavior(uint32_t command, uint32_t value) {
    struct zmk_behavior_binding binding = {
        .behavior_dev = "rgb_ug",
        .param1 = command,
        .param2 = value,
    };
    struct zmk_behavior_binding_event event = {
        .layer = zmk_keymap_highest_layer_active(),
        .position = 0,
        .timestamp = k_uptime_get(),
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
#endif
    };

    return zmk_behavior_invoke_binding(&binding, event, true);
}

static void apply_current_layer_rgb(struct k_work *work) {
    bool rgb_on;

    if (zmk_rgb_underglow_get_state(&rgb_on) < 0 || !rgb_on) {
        return;
    }

    uint8_t layer = zmk_keymap_highest_layer_active();
    if (layer >= ARRAY_SIZE(layer_rgb_profiles)) {
        layer = BASE;
    }

    if (layer == last_applied_layer) {
        return;
    }

    const struct layer_rgb_profile *profile = &layer_rgb_profiles[layer];
    int err = invoke_rgb_behavior(
        RGB_COLOR_HSB_CMD,
        RGB_COLOR_HSB_VAL(profile->hue, profile->saturation, profile->brightness));
    if (err < 0) {
        return;
    }

    err = invoke_rgb_behavior(RGB_EFS_CMD, profile->effect);
    if (err >= 0) {
        last_applied_layer = layer;
    }
}

static int layer_rgb_listener(const zmk_event_t *eh) {
    if (as_zmk_layer_state_changed(eh) != NULL) {
        k_work_reschedule(&layer_rgb_work, K_MSEC(RGB_PROFILE_SETTLE_MS));
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static int layer_rgb_init(void) {
    k_work_init_delayable(&layer_rgb_work, apply_current_layer_rgb);
    k_work_reschedule(&layer_rgb_work, K_MSEC(100));
    return 0;
}

ZMK_LISTENER(layer_rgb, layer_rgb_listener);
ZMK_SUBSCRIPTION(layer_rgb, zmk_layer_state_changed);

SYS_INIT(layer_rgb_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
