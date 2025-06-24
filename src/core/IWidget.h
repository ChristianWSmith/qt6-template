#pragma once

class IWidget {
public:
  IWidget() = default;
  virtual ~IWidget() = default;

  IWidget(const IWidget &) = delete;
  IWidget &operator=(const IWidget &) = delete;
  IWidget(IWidget &&) = delete;
  IWidget &operator=(IWidget &&) = delete;

  virtual void shutdown() = 0;
};
