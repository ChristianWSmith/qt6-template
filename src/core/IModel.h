#pragma once

class IModel {
public:
  IModel() = default;
  virtual ~IModel() = default;

  IModel(const IModel &) = delete;
  IModel &operator=(const IModel &) = delete;
  IModel(IModel &&) = delete;
  IModel &operator=(IModel &&) = delete;

  virtual void shutdown() = 0;

  virtual void loadState() = 0;
  virtual void saveState() const = 0;
};
