#ifndef CALENDAR_H
#define CALENDAR_H

#include "Watchy.h"
#include "Layout.h"

const uint8_t MAX_EVENT_NAME_LEN    = 24;
const uint8_t MAX_EVENTS_PER_COLUMN = 16;
const uint8_t MAX_ALARMS            = 24;
const uint8_t MAX_DAY_EVENTS        = 5;

typedef struct dayEventData {
  char summary[MAX_EVENT_NAME_LEN];
  char start[11];
  char end[11];
} dayEventData;

typedef struct dayEventsData {
  dayEventData events[MAX_DAY_EVENTS];
  uint8_t eventCount;
} dayEventsData;

typedef struct eventData {
  char summary[MAX_EVENT_NAME_LEN];
  time_t start;
  time_t end;
} eventData;

typedef struct eventsData {
  eventData events[MAX_EVENTS_PER_COLUMN];
  uint8_t eventCount;
} eventsData;

typedef struct alarmData {
  char summary[MAX_EVENT_NAME_LEN];
  time_t start;
} alarmData;

typedef struct alarmsData {
  alarmData alarms[MAX_ALARMS];
  uint8_t alarmCount;
} alarmsData;

time_t unixEpochTime(tmElements_t tm);
void fromUnixEpochTime(time_t ts, tmElements_t *tm);

void reset(dayEventsData *data);
void reset(eventsData *data);
void reset(alarmsData *data);
void addEvent(dayEventsData *data, String summary, String start, String end);
void addEvent(eventsData *data, String summary, time_t start, time_t end);
void addAlarm(alarmsData *data, String summary, time_t start);

class CalendarDayEvents : public LayoutElement {
public:
  CalendarDayEvents(dayEventsData *data, tmElements_t currentTime,
                    uint16_t color)
      : data_(data), currentTime_(currentTime), color_(color) {}

  void size(uint16_t targetWidth, uint16_t targetHeight, uint16_t *width,
            uint16_t *height) override {
    maybeDraw(0, 0, targetWidth, targetHeight, width, height, true);
  }

  void draw(int16_t x0, int16_t y0, uint16_t targetWidth, uint16_t targetHeight,
            uint16_t *width, uint16_t *height) override {
    maybeDraw(x0, y0, targetWidth, targetHeight, width, height, false);
  }

private:
  void maybeDraw(int16_t x0, int16_t y0, uint16_t targetWidth,
                 uint16_t targetHeight, uint16_t *width, uint16_t *height,
                 bool noop);

private:
  dayEventsData *data_;
  tmElements_t currentTime_;
  uint16_t color_;
};

class CalendarColumn : public LayoutElement {
public:
  CalendarColumn() : data_(NULL), watchy_(NULL), color_(0) {
    breakTime(0, currentTime_);
  }

  CalendarColumn(eventsData *data, Watchy *watchy, uint16_t color)
      : data_(data), watchy_(watchy), currentTime_(watchy->currentTime),
        color_(color) {}

  void size(uint16_t targetWidth, uint16_t targetHeight, uint16_t *width,
            uint16_t *height) override {
    *width  = targetWidth;
    *height = targetHeight;
  }

  void draw(int16_t x0, int16_t y0, uint16_t targetWidth, uint16_t targetHeight,
            uint16_t *width, uint16_t *height) override;

private:
  void resizeText(char *text, uint8_t buflen, uint16_t width, uint16_t height,
                  int16_t *x1, int16_t *y1, uint16_t *tw, uint16_t *th);

private:
  eventsData *data_;
  Watchy *watchy_;
  tmElements_t currentTime_;
  uint16_t color_;
};

class CalendarHourBar : public LayoutElement {
public:
  CalendarHourBar(tmElements_t currentTime, uint16_t color)
      : currentTime_(currentTime), color_(color) {}

  void size(uint16_t targetWidth, uint16_t targetHeight, uint16_t *width,
            uint16_t *height) override {
    maybeDraw(0, 0, targetWidth, targetHeight, width, height, true);
  }
  void draw(int16_t x0, int16_t y0, uint16_t targetWidth, uint16_t targetHeight,
            uint16_t *width, uint16_t *height) override {
    maybeDraw(x0, y0, targetWidth, targetHeight, width, height, false);
  }

private:
  void maybeDraw(int16_t x0, int16_t y0, uint16_t targetWidth,
                 uint16_t targetHeight, uint16_t *width, uint16_t *height,
                 bool noop);

private:
  tmElements_t currentTime_;
  uint16_t color_;
};

class CalendarAlarms : public LayoutElement {
public:
  CalendarAlarms(alarmsData *data, Watchy *watchy, uint16_t color)
      : data_(data), watchy_(watchy), color_(color) {}

  void size(uint16_t targetWidth, uint16_t targetHeight, uint16_t *width,
            uint16_t *height) override {
    maybeDraw(0, 0, targetWidth, targetHeight, width, height, true);
  }
  void draw(int16_t x0, int16_t y0, uint16_t targetWidth, uint16_t targetHeight,
            uint16_t *width, uint16_t *height) override {
    maybeDraw(x0, y0, targetWidth, targetHeight, width, height, false);
  }

private:
  void maybeDraw(int16_t x0, int16_t y0, uint16_t targetWidth,
                 uint16_t targetHeight, uint16_t *width, uint16_t *height,
                 bool noop);

private:
  alarmsData *data_;
  Watchy *watchy_;
  uint16_t color_;
};

#endif
