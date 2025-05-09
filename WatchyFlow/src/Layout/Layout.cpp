#include "Layout.h"

static void *LayoutElement::operator new(size_t size) {
  return globalArena.allocate(size, alignof(LayoutElement));
}
static void *LayoutElement::operator new[](size_t size) {
  return globalArena.allocate(size, alignof(LayoutElement));
}
static void LayoutElement::operator delete(void *ptr, size_t size) noexcept {
  globalArena.deallocate(ptr, size);
}
static void LayoutElement::operator delete(void *ptr) noexcept {
  globalArena.deallocate(ptr, -1);
}
static void LayoutElement::operator delete[](void *ptr, size_t size) noexcept {
  globalArena.deallocate(ptr, size);
}
static void LayoutElement::operator delete[](void *ptr) noexcept {
  globalArena.deallocate(ptr, -1);
}

void LayoutText::size(Display *display, uint16_t targetWidth,
                      uint16_t targetHeight, uint16_t *width,
                      uint16_t *height) {
  if (text_.length() <= 0) {
    *width  = 0;
    *height = 0;
    return;
  }
  int16_t x1, y1;
  display->setFont(font_);
  display->getTextBounds(text_, 0, 0, &x1, &y1, width, height);
}

void LayoutText::draw(Display *display, int16_t x0, int16_t y0,
                      uint16_t targetWidth, uint16_t targetHeight,
                      uint16_t *width, uint16_t *height) {
  if (text_.length() <= 0) {
    *width  = 0;
    *height = 0;
    return;
  }
  int16_t x1, y1;
  display->setFont(font_);
  display->getTextBounds(text_, 0, 0, &x1, &y1, width, height);
  display->setTextColor(color_);
  display->setCursor(x0 - x1, y0 - y1);
  display->print(text_);
}

void LayoutRotate::size(Display *display, uint16_t targetWidth,
                        uint16_t targetHeight, uint16_t *width,
                        uint16_t *height) {
  uint16_t swap;
  if (rotate_ == 1 || rotate_ == 3) {
    swap         = targetWidth;
    targetWidth  = targetHeight;
    targetHeight = swap;
  }
  child_->size(display, targetWidth, targetHeight, width, height);
  if (rotate_ == 1 || rotate_ == 3) {
    swap    = *width;
    *width  = *height;
    *height = swap;
  }
}

void LayoutRotate::draw(Display *display, int16_t x0, int16_t y0,
                        uint16_t targetWidth, uint16_t targetHeight,
                        uint16_t *width, uint16_t *height) {
  uint16_t swap;
  if (rotate_ == 1 || rotate_ == 3) {
    swap         = targetWidth;
    targetWidth  = targetHeight;
    targetHeight = swap;
  }
  child_->size(display, targetWidth, targetHeight, width, height);

  switch (rotate_) {
  default:
    break;
  case 1:
    swap    = *width;
    *width  = *height;
    *height = swap;
    swap    = x0;
    x0      = y0;
    y0      = display->width() - swap - *width;
    break;
  case 2:
    x0 = display->width() - x0 - *height;
    y0 = display->height() - y0 - *width;
    break;
  case 3:
    swap    = *width;
    *width  = *height;
    *height = swap;
    swap    = y0;
    y0      = x0;
    x0      = display->height() - swap - *height;
    break;
  }

  uint8_t currentRotation = display->getRotation();
  display->setRotation((currentRotation + rotate_) % 4);
  child_->draw(display, x0, y0, targetWidth, targetHeight, width, height);
  display->setRotation(currentRotation);
  if (rotate_ == 1 || rotate_ == 3) {
    swap    = *width;
    *width  = *height;
    *height = swap;
  }
}

MemArenaAllocator<LayoutEntry> allocatorLayoutEntry(globalArena);

LayoutColumns::LayoutColumns(std::initializer_list<LayoutEntry> elems)
    : elems_(allocatorLayoutEntry) {
  elems_.reserve(elems.size());
  for (const LayoutEntry &info : elems) {
    elems_.push_back(info);
  }
}

void LayoutColumns::size(Display *display, uint16_t targetWidth,
                         uint16_t targetHeight, uint16_t *width,
                         uint16_t *height) {
  *height         = targetHeight;
  *width          = 0;
  bool canStretch = false;

  for (uint16_t i = 0; i < elems_.size(); i++) {
    if (elems_[i].stretch_) {
      canStretch = true;
    }
    uint16_t columnWidth, columnHeight;
    elems_[i].elem_->size(display, 0, targetHeight, &columnWidth,
                          &columnHeight);
    if (columnHeight > *height) {
      *height = columnHeight;
    }
    *width += columnWidth;
  }

  if (*width < targetWidth && canStretch) {
    *width = targetWidth;
  }
}

void LayoutColumns::draw(Display *display, int16_t x0, int16_t y0,
                         uint16_t targetWidth, uint16_t targetHeight,
                         uint16_t *width, uint16_t *height) {
  uint16_t fixedWidth = 0;
  uint16_t splits     = 0;

  for (uint16_t i = 0; i < elems_.size(); i++) {
    uint16_t subwidth, subheight;
    elems_[i].elem_->size(display, 0, targetHeight, &subwidth, &subheight);
    if (subheight > targetHeight) {
      targetHeight = subheight;
    }
    if (elems_[i].stretch_) {
      splits++;
      continue;
    }
    fixedWidth += subwidth;
  }

  uint16_t remainingWidth = 0;
  if (targetWidth > fixedWidth) {
    remainingWidth = targetWidth - fixedWidth;
  }

  *width  = 0;
  *height = 0;
  for (uint16_t i = 0; i < elems_.size(); i++) {
    uint16_t subTargetWidth = 0;
    if (elems_[i].stretch_) {
      subTargetWidth = remainingWidth / splits;
      remainingWidth -= subTargetWidth;
      splits--;
    }
    uint16_t subwidth, subheight;
    elems_[i].elem_->draw(display, x0 + *width, y0, subTargetWidth,
                          targetHeight, &subwidth, &subheight);
    *width += subwidth;
    if (subheight > *height) {
      *height = subheight;
    }
  }
}

LayoutRows::LayoutRows(std::initializer_list<LayoutEntry> elems)
    : elems_(allocatorLayoutEntry) {
  elems_.reserve(elems.size());
  for (const LayoutEntry &info : elems) {
    elems_.push_back(info);
  }
}

void LayoutRows::size(Display *display, uint16_t targetWidth,
                      uint16_t targetHeight, uint16_t *width,
                      uint16_t *height) {
  *width          = targetWidth;
  *height         = 0;
  bool canStretch = false;

  for (uint16_t i = 0; i < elems_.size(); i++) {
    if (elems_[i].stretch_) {
      canStretch = true;
    }
    uint16_t rowWidth, rowHeight;
    elems_[i].elem_->size(display, targetWidth, 0, &rowWidth, &rowHeight);
    if (rowWidth > *width) {
      *width = rowWidth;
    }
    *height += rowHeight;
  }

  if (*height < targetHeight && canStretch) {
    *height = targetHeight;
  }
}

void LayoutRows::draw(Display *display, int16_t x0, int16_t y0,
                      uint16_t targetWidth, uint16_t targetHeight,
                      uint16_t *width, uint16_t *height) {
  uint16_t fixedHeight = 0;
  uint16_t splits      = 0;

  for (uint16_t i = 0; i < elems_.size(); i++) {
    uint16_t subwidth, subheight;
    elems_[i].elem_->size(display, targetWidth, 0, &subwidth, &subheight);
    if (subwidth > targetWidth) {
      targetWidth = subwidth;
    }
    if (elems_[i].stretch_) {
      splits++;
      continue;
    }
    fixedHeight += subheight;
  }

  uint16_t remainingHeight = 0;
  if (targetHeight > fixedHeight) {
    remainingHeight = targetHeight - fixedHeight;
  }

  *width  = 0;
  *height = 0;
  for (uint16_t i = 0; i < elems_.size(); i++) {
    uint16_t subTargetHeight = 0;
    if (elems_[i].stretch_) {
      subTargetHeight = remainingHeight / splits;
      remainingHeight -= subTargetHeight;
      splits--;
    }
    uint16_t subwidth, subheight;
    elems_[i].elem_->draw(display, x0, y0 + *height, targetWidth,
                          subTargetHeight, &subwidth, &subheight);
    *height += subheight;
    if (subwidth > *width) {
      *width = subwidth;
    }
  }
}

void LayoutCenter::size(Display *display, uint16_t targetWidth,
                        uint16_t targetHeight, uint16_t *width,
                        uint16_t *height) {
  child_->size(display, targetWidth, targetHeight, width, height);
  if (*width < targetWidth) {
    *width = targetWidth;
  }
  if (*height < targetHeight) {
    *height = targetHeight;
  }
}

void LayoutCenter::draw(Display *display, int16_t x0, int16_t y0,
                        uint16_t targetWidth, uint16_t targetHeight,
                        uint16_t *width, uint16_t *height) {
  int16_t x0_offset = 0, y0_offset = 0;

  child_->size(display, targetWidth, targetHeight, width, height);
  if (*width < targetWidth) {
    x0_offset = (targetWidth - *width) / 2;
  }
  if (*height < targetHeight) {
    y0_offset = (targetHeight - *height) / 2;
  }

  child_->draw(display, x0 + x0_offset, y0 + y0_offset, *width, *height, width,
               height);

  *width += x0_offset;
  *height += y0_offset;
  if (*width < targetWidth) {
    *width = targetWidth;
  }
  if (*height < targetHeight) {
    *height = targetHeight;
  }
}

void LayoutHCenter::size(Display *display, uint16_t targetWidth,
                         uint16_t targetHeight, uint16_t *width,
                         uint16_t *height) {
  child_->size(display, targetWidth, targetHeight, width, height);
  if (*width < targetWidth) {
    *width = targetWidth;
  }
}

void LayoutHCenter::draw(Display *display, int16_t x0, int16_t y0,
                         uint16_t targetWidth, uint16_t targetHeight,
                         uint16_t *width, uint16_t *height) {
  int16_t x0_offset = 0;

  child_->size(display, targetWidth, targetHeight, width, height);
  if (*width < targetWidth) {
    x0_offset = (targetWidth - *width) / 2;
  }

  child_->draw(display, x0 + x0_offset, y0, *width, *height, width, height);

  *width += x0_offset;
  if (*width < targetWidth) {
    *width = targetWidth;
  }
}

void LayoutVCenter::size(Display *display, uint16_t targetWidth,
                         uint16_t targetHeight, uint16_t *width,
                         uint16_t *height) {
  child_->size(display, targetWidth, targetHeight, width, height);
  if (*height < targetHeight) {
    *height = targetHeight;
  }
}

void LayoutVCenter::draw(Display *display, int16_t x0, int16_t y0,
                         uint16_t targetWidth, uint16_t targetHeight,
                         uint16_t *width, uint16_t *height) {
  int16_t y0_offset = 0;

  child_->size(display, targetWidth, targetHeight, width, height);
  if (*height < targetHeight) {
    y0_offset = (targetHeight - *height) / 2;
  }

  child_->draw(display, x0, y0 + y0_offset, *width, *height, width, height);

  *height += y0_offset;
  if (*height < targetHeight) {
    *height = targetHeight;
  }
}

LayoutPad::LayoutPad(const LayoutElement &child, int16_t padTop,
                     int16_t padRight, int16_t padBottom, int16_t padLeft)
    : child_(child.clone()), padTop_(padTop), padRight_(padRight),
      padBottom_(padBottom), padLeft_(padLeft) {}

void LayoutPad::size(Display *display, uint16_t targetWidth,
                     uint16_t targetHeight, uint16_t *width, uint16_t *height) {
  int16_t signedTargetWidth  = (int16_t)targetWidth;
  int16_t signedTargetHeight = (int16_t)targetHeight;
  signedTargetWidth -= (padLeft_ + padRight_);
  signedTargetHeight -= (padTop_ + padBottom_);
  targetWidth  = (signedTargetWidth < 0) ? 0 : (uint16_t)signedTargetWidth;
  targetHeight = (signedTargetHeight < 0) ? 0 : (uint16_t)signedTargetHeight;
  child_->size(display, targetWidth, targetHeight, width, height);
  *width += padLeft_ + padRight_;
  *height += padTop_ + padBottom_;
}

void LayoutPad::draw(Display *display, int16_t x0, int16_t y0,
                     uint16_t targetWidth, uint16_t targetHeight,
                     uint16_t *width, uint16_t *height) {
  int16_t signedTargetWidth  = (int16_t)targetWidth;
  int16_t signedTargetHeight = (int16_t)targetHeight;
  signedTargetWidth -= (padLeft_ + padRight_);
  signedTargetHeight -= (padTop_ + padBottom_);
  targetWidth  = (signedTargetWidth < 0) ? 0 : (uint16_t)signedTargetWidth;
  targetHeight = (signedTargetHeight < 0) ? 0 : (uint16_t)signedTargetHeight;
  child_->draw(display, x0 + padLeft_, y0 + padTop_, targetWidth, targetHeight,
               width, height);
  *width += padLeft_ + padRight_;
  *height += padTop_ + padBottom_;
}

LayoutBorder::LayoutBorder(const LayoutElement &child, bool top, bool right,
                           bool bottom, bool left, uint16_t color)
    : pad_(child, top ? 1 : 0, right ? 1 : 0, bottom ? 1 : 0, left ? 1 : 0),
      color_(color) {}

void LayoutBorder::size(Display *display, uint16_t targetWidth,
                        uint16_t targetHeight, uint16_t *width,
                        uint16_t *height) {
  pad_.size(display, targetWidth, targetHeight, width, height);
}

void LayoutBorder::draw(Display *display, int16_t x0, int16_t y0,
                        uint16_t targetWidth, uint16_t targetHeight,
                        uint16_t *width, uint16_t *height) {
  pad_.draw(display, x0, y0, targetWidth, targetHeight, width, height);
  if (pad_.padTop() > 0) {
    display->drawFastHLine(x0, y0, *width, color_);
  }
  if (pad_.padBottom() > 0) {
    display->drawFastHLine(x0, y0 + *height - 1, *width, color_);
  }
  if (pad_.padLeft() > 0) {
    display->drawFastVLine(x0, y0, *height, color_);
  }
  if (pad_.padRight() > 0) {
    display->drawFastVLine(x0 + *width - 1, y0, *height, color_);
  }
}

void LayoutBackground::size(Display *display, uint16_t targetWidth,
                            uint16_t targetHeight, uint16_t *width,
                            uint16_t *height) {
  child_->size(display, targetWidth, targetHeight, width, height);
}

void LayoutBackground::draw(Display *display, int16_t x0, int16_t y0,
                            uint16_t targetWidth, uint16_t targetHeight,
                            uint16_t *width, uint16_t *height) {
  child_->size(display, targetWidth, targetHeight, width, height);
  display->fillRect(x0, y0, *width, *height, color_);
  child_->draw(display, x0, y0, targetWidth, targetHeight, width, height);
}

void LayoutOverlay::size(Display *display, uint16_t targetWidth,
                         uint16_t targetHeight, uint16_t *width,
                         uint16_t *height) {
  uint16_t w, h;
  background_->size(display, targetWidth, targetHeight, &w, &h);
  if (targetWidth < w) {
    targetWidth = w;
  }
  if (targetHeight < h) {
    targetHeight = h;
  }
  foreground_->size(display, targetWidth, targetHeight, width, height);
  if (w > *width) {
    *width = w;
  }
  if (h > *height) {
    *height = h;
  }
}

void LayoutOverlay::draw(Display *display, int16_t x0, int16_t y0,
                         uint16_t targetWidth, uint16_t targetHeight,
                         uint16_t *width, uint16_t *height) {
  background_->draw(display, x0, y0, targetWidth, targetHeight, width, height);
  if (targetWidth < *width) {
    targetWidth = *width;
  }
  if (targetHeight < *height) {
    targetHeight = *height;
  }
  foreground_->draw(display, x0, y0, targetWidth, targetHeight, width, height);
}
