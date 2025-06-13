#ifndef APPLOGPRESENTER_H
#define APPLOGPRESENTER_H
#include "../model/IAppLogModel.h"
#include "../widget/IAppLogWidget.h"
#include "IAppLogPresenter.h"
#include <QObject>

class AppLogPresenter : public IAppLogPresenter {
  Q_OBJECT
  Q_INTERFACES(IAppLogPresenter)

public:
  explicit AppLogPresenter(IAppLogModel *model, IAppLogWidget *view,
                           QObject *parent = nullptr);

private slots:
  // Concrete slots for handling events

private:
  IAppLogModel *m_model;
  IAppLogWidget *m_view;
};

#endif
