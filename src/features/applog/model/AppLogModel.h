#ifndef APPLOGMODEL_H
#define APPLOGMODEL_H
#include "IAppLogModel.h"

class AppLogModel : public IAppLogModel {
  Q_OBJECT
  Q_INTERFACES(IAppLogModel)

public:
  explicit AppLogModel(QObject *parent = nullptr);
  // Implements IAppLogModel methods

signals:
  // Signals emitted by this concrete Model

private:
  // Private data members holding the Model's state
};

#endif
