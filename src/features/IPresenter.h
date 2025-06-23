#pragma once

class IPresenter {
public:
  IPresenter() = default;
  virtual ~IPresenter() = default;

  IPresenter(const IPresenter &) = delete;
  IPresenter &operator=(const IPresenter &) = delete;
  IPresenter(IPresenter &&) = delete;
  IPresenter &operator=(IPresenter &&) = delete;

  virtual void shutdown() = 0;
};
