#ifndef _ODBCCUSB_INCLUDE_H
#define _ODBCCUSB_INCLUDE_H

typedef struct {
  INT   buffer_size;
  INT   lam_timeout;
  INT   trig_delay;
  INT   delay;
  INT   width;
} CCUSB_SETTINGS;

#define CCUSB_SETTINGS_STR(_name) const char *_name[] = {\
    "[.]",\
    "Buffer size mode = INT 0",\
    "LAM timeout (us)= INT  1",\
    "Trigger delay (us) = INT 1",\
    "Pulse Delay (ns) = INT 1000000",\
    "Pulse Width (ns) = INT 50",\
    "",\
    NULL }

#endif
