// Copyright 2024 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "ch559.h"
#include "gpio.h"
#include "io.h"
#include "led.h"
#include "serial.h"
#include "timer3.h"
#include "usb/hid/hid.h"
#include "usb/usb_host.h"

static int16_t x = 0;
static int16_t y = 0;
static bool buttons[3] = {false, false, false};

static bool button_check(uint16_t index, const uint8_t* data) {
  if (index == 0xffff) {
    return false;
  }
  uint8_t byte = index >> 3;
  uint8_t bit = index & 7;
  return data[byte] & (1 << bit);
}

static uint16_t analog_check(const struct hid_info* info,
                             const uint8_t* data,
                             uint8_t index) {
  if (info->axis[index] == 0xffff) {
    // return 0x8000;
  } else if (info->axis_size[index] == 8) {
    uint8_t v = data[info->axis[index] >> 3];
    v <<= info->axis_shift[index];
    if (info->axis_sign[index]) {
      v += 0x80;
    }
    if (info->axis_polarity[index]) {
      v = 0xff - v;
    }
    return v << 8;
  } else if (info->axis_size[index] == 10 || info->axis_size[index] == 12) {
    uint8_t byte_index = info->axis[index] >> 3;
    uint16_t l = data[byte_index + 0];
    uint16_t h = data[byte_index + 1];
    uint16_t v = (((h << 8) | l) >> (info->axis[index] & 7))
                 << (16 - info->axis_size[index]);
    v <<= info->axis_shift[index];
    if (info->axis_sign[index]) {
      v += 0x8000;
    }
    if (info->axis_polarity[index]) {
      v = 0xffff - v;
    }
    return v;
  } else if (info->axis_size[index] == 16) {
    uint8_t byte = info->axis[index] >> 3;
    uint16_t v = data[byte] | ((uint16_t)data[byte + 1] << 8);
    v <<= info->axis_shift[index];
    if (info->axis_sign[index]) {
      v += 0x8000;
    }
    if (info->axis_polarity[index]) {
      v = 0xffff - v;
    }
    return v;
  }
  return 0x8000;
}

void update(const uint8_t hub,
            const struct hid_info* info,
            const uint8_t* data,
            uint16_t size) {
  hub;
  if (info->report_id) {
    if (info->report_id != data[0]) {
      return;
    }
    data++;
    size--;
  }
  if (info->state != HID_STATE_READY || info->type != HID_TYPE_MOUSE) {
    x = y = 0;
    buttons[0] = buttons[1] = buttons[2] = false;
    return;
  }
  x += (int16_t)analog_check(info, data, 0);
  y += (int16_t)analog_check(info, data, 1);
  for (int i = 0; i < 3; ++i) {
    buttons[i] = button_check(info->button[i], data);
  }
}

static void detected(void) {
  led_oneshot(L_PULSE_ONCE);
}

static uint8_t get_flags(void) {
  return USE_HUB0 | USE_HUB1;
}

void main(void) {
  initialize();

  P3_DIR = 0xff;  // Output
  P3 = 0xff;      // High

  // For local tentative repair.
  P4_DIR |= (1 << 3);
  P4_OUT |= (1 << 3);

  timer3_tick_init();

  Serial.println("Moonphase");

  led_init(1, 5, LOW);
  led_mode(L_ON);

  struct hid hid;
  hid.report = update;
  hid.detected = detected;
  hid.get_flags = get_flags;
  hid_init(&hid);
  usb_host_reset();

  static uint8_t phase_a[4] = {0, 1, 1, 0};
  static uint8_t phase_b[4] = {0, 0, 1, 1};
  static uint16_t phase_x = 0;
  static uint16_t phase_y = 0;

  uint16_t tick = timer3_tick_raw();
  uint16_t ms = timer3_tick_msec();
  int16_t vx = 0;
  int16_t vy = 0;
  for (;;) {
    hid_poll();
    led_poll();
    if (!timer3_tick_msec_between(ms, ms + 10)) {
      ms = timer3_tick_msec();
      vx = x;
      vy = y;
      x = 0;
      y = 0;
    }
    uint16_t now = timer3_tick_raw();
    if (tick == now) {
      continue;
    }
    tick = now;
    phase_x += vx;
    phase_y += vy;
    P3_1 = phase_a[(phase_x >> 8) & 3];
    P3_2 = phase_b[(phase_x >> 8) & 3];
    P3_3 = phase_a[(phase_y >> 8) & 3];
    P3_4 = phase_b[(phase_y >> 8) & 3];
    P3_5 = buttons[0] ? LOW : HIGH;
    P3_6 = buttons[2] ? LOW : HIGH;
    P3_7 = buttons[1] ? LOW : HIGH;

    // For local tantative repair.
    P4_3 = buttons[1] ? LOW : HIGH;
  }
}
