#ifndef ICOUNTERPRESENTER_H
#define ICOUNTERPRESENTER_H
#include "../../features_common.h"
#include "../counter_common.h"
#include <QObject>
#include <QtPlugin>

class ICounterPresenter : public QObject {
  Q_OBJECT
public:
  explicit ICounterPresenter(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~ICounterPresenter() = default;
};

Q_DECLARE_INTERFACE(ICounterPresenter,
                    COUNTER_FEATURE_ID FEATURE_PRESENTER_SUFFIX)

#endif
