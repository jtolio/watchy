#include "Calendar.h"
#include "Layout.h"
#include "Watchy.h"
#include <Fonts/Picopixel.h>
#include <Fonts/FreeMono9pt7b.h>

const GFXfont *SMALL_FONT = &Picopixel;
const int16_t CALENDAR_PAST_MINUTES = 30;
const int16_t CALENDAR_FUTURE_HOURS = 7;

time_t unixEpochTime(tmElements_t tm) {
  // the system clock is stored in the local timezone and not UTC, like most
  // unix systems.
  // the unix timestamp calculation is therefore off by the local timezone
  // offset from UTC, so to fix it we need to subtract the gmtOffset.
  // note that time_t may be 32 bits, and may have a Y2038 problem. it may
  // only make sense to use time_t for time deltas.
  return makeTime(tm) - gmtOffset;
}

void fromUnixEpochTime(time_t ts, tmElements_t *tm) {
  // see comment in unixEpochTime
  ts += gmtOffset;
  breakTime(ts, *tm);
}

void reset(dayEventsData *data) {
  data->eventCount = 0;
}

void addEvent(dayEventsData *data, String summary, String start, String end) {
  if (data->eventCount >= MAX_DAY_EVENTS) { return; }
  if (data->eventCount == MAX_DAY_EVENTS - 1) { summary = "TOO MANY EVENTS"; }
  start.toCharArray(data->events[data->eventCount].start, 11);
  end.toCharArray(data->events[data->eventCount].end, 11);
  summary.toCharArray(data->events[data->eventCount].summary, MAX_EVENT_NAME_LEN);
  data->eventCount++;
}

void reset(eventsData *data) {
  data->eventCount = 0;
}

void addEvent(eventsData *data, String summary, time_t start, time_t end) {
  if (data->eventCount >= MAX_EVENTS_PER_COLUMN) { return; }
  if (data->eventCount == MAX_EVENTS_PER_COLUMN - 1) { summary = "TOO MANY EVENTS"; }
  data->events[data->eventCount].start = start;
  data->events[data->eventCount].end = end;
  summary.toCharArray(data->events[data->eventCount].summary, MAX_EVENT_NAME_LEN);
  data->eventCount++;
}

void reset(alarmsData *data) {
  data->alarmCount = 0;
}

void addAlarm(alarmsData *data, String summary, time_t start) {
  if (data->alarmCount >= MAX_ALARMS) { return; }
  if (data->alarmCount == MAX_ALARMS - 1) { summary = "TOO MANY ALARMS"; }
  data->alarms[data->alarmCount].start = start;
  summary.toCharArray(data->alarms[data->alarmCount].summary, MAX_EVENT_NAME_LEN);
  data->alarmCount++;
}

void CalendarDayEvents::maybeDraw(
    int16_t x0, int16_t y0, uint16_t targetWidth, uint16_t targetHeight,
    uint16_t *width, uint16_t *height, bool noop) {
  *width = targetWidth;
  *height = 0;

  Watchy::Watchy::display.setFont(SMALL_FONT);
  Watchy::Watchy::display.setTextColor(color_);

  String today = String(tmYearToCalendar(currentTime_.Year));
  today += (currentTime_.Month < 10) ? "-0" : "-";
  today += String(currentTime_.Month);
  today += (currentTime_.Day < 10) ? "-0" : "-";
  today += String(currentTime_.Day);

  for (int i = 0; i < data_->eventCount; i++) {
    String text = data_->events[0].summary;
    if (String(data_->events[i].start) > today ||
        String(data_->events[i].end) <= today) {
      continue;
    }

    uint16_t textWidth, textHeight;
    int16_t x1, y1;
    Watchy::Watchy::display.getTextBounds(
      text, 0, 0, &x1, &y1, &textWidth, &textHeight);
    textWidth += 4;
    textHeight += 2;
    x1 -= 2;
    y1 -= 2;
    if (textWidth > *width) {
      *width = textWidth;
    }
    if (!noop) {
      Watchy::Watchy::display.setCursor(x0 - x1, y0 - y1 + *height);
      Watchy::Watchy::display.print(text);
    }
    *height += textHeight;
  }

  if (*height > 0) {
    *height += 2;
    if (!noop) {
      Watchy::Watchy::display.drawFastHLine(x0, y0 + *height, *width, color_);
    }
  }
}

void CalendarColumn::draw(int16_t x0, int16_t y0,
                          uint16_t targetWidth, uint16_t targetHeight,
                          uint16_t *width, uint16_t *height) {
  if (targetHeight <= 0) { targetHeight = 1; }

  *width = targetWidth;
  *height = targetHeight;

  Watchy::Watchy::display.setFont(SMALL_FONT);
  Watchy::Watchy::display.setTextColor(color_);

  time_t now = unixEpochTime(currentTime_);
  time_t windowStart = now - (CALENDAR_PAST_MINUTES * 60);
  time_t windowEnd = now + (CALENDAR_FUTURE_HOURS * 60 * 60);
  time_t secondsPerPixel = (windowEnd - windowStart) / targetHeight;
  if (secondsPerPixel <= 0) { secondsPerPixel = 1; }

  for (int i = 0; i < data_->eventCount; i++) {
    eventData *event = &(data_->events[i]);
    time_t eventStart = event->start;
    time_t eventEnd = event->end;
    if (eventEnd <= windowStart) { continue; }
    if (eventStart >= windowEnd) { continue; }

    tmElements_t eventStarttm;
    fromUnixEpochTime(eventStart, &eventStarttm);
    if (eventStarttm.Hour == currentTime_.Hour && eventStarttm.Minute == currentTime_.Minute) {
      watchy_->vibMotor(75, 5);
    }

    if (eventStart < windowStart) {
      eventStart = windowStart;
    } else {
      Watchy::Watchy::display.drawFastHLine(
        x0, y0 + ((eventStart - windowStart) / secondsPerPixel),
        targetWidth, color_);
    }
    if (eventEnd > windowEnd) {
      eventEnd = windowEnd;
    } else {
      Watchy::Watchy::display.drawFastHLine(
        x0, y0 + ((eventEnd - windowStart) / secondsPerPixel),
        targetWidth, color_);
    }
    int16_t eventOffset = (eventStart - windowStart) / secondsPerPixel;
    int16_t eventSize = (eventEnd - eventStart) / secondsPerPixel;
    Watchy::Watchy::display.drawFastVLine(
      x0, y0 + eventOffset, eventSize, color_);
    Watchy::Watchy::display.drawFastVLine(
      x0 + targetWidth - 1, y0 + eventOffset, eventSize, color_);
    if (targetWidth <= 4 || eventSize <= 4) { continue; }
    int16_t x1, y1;
    uint16_t tw, th;
    Watchy::Watchy::display.getTextBounds(event->summary, 0, 0, &x1, &y1, &tw, &th);
    if (tw + 4 > targetWidth || th + 4 > eventSize) {
      resizeText(event->summary, MAX_EVENT_NAME_LEN, targetWidth - 4, eventSize - 4, &x1, &y1, &tw, &th);
    }

    Watchy::Watchy::display.setCursor(x0 - x1 + 2, y0 - y1 + eventOffset + 2) ;
    Watchy::Watchy::display.print(event->summary);
  }
}

void CalendarColumn::resizeText(char *text, uint8_t buflen, uint16_t width,
    uint16_t height, int16_t *x1, int16_t *y1, uint16_t *tw, uint16_t *th) {
  String original = text;
  String result;
  int i = 0;
  while (true) {
    if (i >= original.length()) {
      break;
    }
    Watchy::Watchy::display.getTextBounds(
      result + original[i], 0, 0, x1, y1, tw, th);
    if (*th > height) {
      break;
    }
    if (*tw > width) {
      if (result.length() > 0 && result[result.length() - 1] == '\n') {
        break;
      }
      result += "\n";
      continue;
    }
    result += original[i];
    i++;
  }
  result.toCharArray(text, buflen);
  Watchy::Watchy::display.getTextBounds(
    result, 0, 0, x1, y1, tw, th);
}

void CalendarHourBar::maybeDraw(int16_t x0, int16_t y0,
                                uint16_t targetWidth, uint16_t targetHeight,
                                uint16_t *width, uint16_t *height, bool noop) {
  if (targetHeight <= 0) { targetHeight = 1; }
  *width = 0;
  *height = targetHeight;

  Watchy::Watchy::display.setFont(SMALL_FONT);
  Watchy::Watchy::display.setTextColor(color_);

  time_t now = unixEpochTime(currentTime_);
  time_t windowStart = now - (CALENDAR_PAST_MINUTES * 60);
  time_t windowEnd = now + (CALENDAR_FUTURE_HOURS * 60 * 60);
  time_t secondsPerPixel = (windowEnd - windowStart) / targetHeight;
  if (secondsPerPixel <= 0) { secondsPerPixel = 1; }

  tmElements_t currentHour = currentTime_;
  currentHour.Minute = 0;
  currentHour.Second = 0;
  time_t currentHourUnix = unixEpochTime(currentHour);
  int currentHourNum = currentHour.Hour;

  for (int i = -(CALENDAR_PAST_MINUTES / 60) - 1; i <= CALENDAR_FUTURE_HOURS; i++) {
    time_t hourTime = currentHourUnix + (i * 60 * 60);
    if (hourTime < windowStart) { continue; }
    if (hourTime >= windowEnd) { continue; }

    String text(((currentHourNum + i + 11) % 12) + 1);
    int16_t x1, y1;
    uint16_t tw, th;
    Watchy::Watchy::display.getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);
    if (th * secondsPerPixel + 3 + hourTime > windowEnd) { continue; }
    if (tw + 4 > *width) { *width = tw + 4; }

    if (!noop) {
      int16_t yoffset = y0 + ((hourTime - windowStart) / secondsPerPixel);
      Watchy::Watchy::display.setCursor(x0 - x1 + 2, yoffset - y1 + 3);
      Watchy::Watchy::display.print(text);
    }
  }

  if (!noop) {
    Watchy::Watchy::display.drawFastHLine(
      x0,
      y0 + ((now - windowStart) / secondsPerPixel),
      *width, color_);

    for (int i = -(CALENDAR_PAST_MINUTES / 60) - 1; i <= CALENDAR_FUTURE_HOURS; i++) {
      time_t hourTime = currentHourUnix + (i * 60 * 60);
      if (hourTime < windowStart) { continue; }
      if (hourTime >= windowEnd) { continue; }

      int16_t yoffset = y0 + ((hourTime - windowStart) / secondsPerPixel);
      Watchy::Watchy::display.drawFastHLine(x0, yoffset, *width / 2, color_);
    }
  }
}


void CalendarAlarms::maybeDraw(int16_t x0, int16_t y0,
                               uint16_t targetWidth, uint16_t targetHeight,
                               uint16_t *width, uint16_t *height, bool noop) {
  *width = targetWidth;
  *height = 0;

  Watchy::Watchy::display.setFont(&FreeMono9pt7b);
  Watchy::Watchy::display.setTextColor(color_);

  time_t now = unixEpochTime(watchy_->currentTime);
  time_t windowStart = now - (2*60);
  time_t windowEnd = now + (60*60);
  for (int i = 0; i < data_->alarmCount; i++) {
    alarmData *alarm = &(data_->alarms[i]);
    if (alarm->start < windowStart) { continue; }
    if (alarm->start > windowEnd) { continue; }
    tmElements_t alarmtm;
    fromUnixEpochTime(alarm->start, &alarmtm);
    if (!noop &&
        alarmtm.Minute == watchy_->currentTime.Minute &&
        alarmtm.Hour == watchy_->currentTime.Hour) {
      watchy_->vibMotor(100, 10);
    }
    String text;
    int hourNum = ((alarmtm.Hour + 11) % 12) + 1;
    if (hourNum < 10) {
      text += "0";
    }
    text += hourNum;
    text += (alarmtm.Minute < 10 ? ":0" : ":");
    text += alarmtm.Minute;
    text += ": ";
    text += alarm->summary;

    int16_t x1, y1;
    uint16_t tw, th;
    Watchy::Watchy::display.getTextBounds(text, 0, 0, &x1, &y1, &tw, &th);

    if (!noop) {
      Watchy::Watchy::display.setCursor(x0 - x1 + 2, y0 + *height - y1 + 2);
      Watchy::Watchy::display.print(text);
    }

    *height += th + 2;
  }
  if (*height > 0) { *height += 2; }
}
