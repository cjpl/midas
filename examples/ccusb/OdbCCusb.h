#ifndef _ODBCCUSB_INCLUDE_H
#define _ODBCCUSB_INCLUDE_H

typedef struct {
  INT   buffer_size;
  INT   lam_timeout;
  INT   trig_delay;
  INT   frequency;
  INT   width;
} CCUSB_SETTINGS;

#define CCUSB_SETTINGS_STR(_name) const char *_name[] = {\
    "[.]",\
    "Buffer size = INT 7",\
    "LAM timeout = INT 0x840",\
    "Trigger delay (us) = INT 300",\
    "Frequency (Hz) = INT 5000",\
    "Pulse Width (ns) = INT 500",\
    "",\
    NULL }

#endif
