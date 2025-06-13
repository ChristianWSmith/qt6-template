#ifndef IAPPLOGPRESENTER_H
#define IAPPLOGPRESENTER_H
#include "../../featurescommon.h"
#include "../applogcommon.h"
#include <QObject>
#include <QtPlugin>

class IAppLogPresenter : public QObject {
  Q_OBJECT

public:
  explicit IAppLogPresenter(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~IAppLogPresenter() = default;
};

Q_DECLARE_INTERFACE(IAppLogPresenter,
                    APPLOG_FEATURE_ID FEATURE_PRESENTER_SUFFIX)

#endif
