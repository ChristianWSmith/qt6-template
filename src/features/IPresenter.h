#pragma once

class IPresenter {
public:
  virtual ~IPresenter() = default;

  virtual void shutdown() = 0;
};
