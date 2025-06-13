#include "AppLogPresenter.h"

AppLogPresenter::AppLogPresenter(IAppLogModel *model, IAppLogWidget *view,
                                 QObject *parent)
    : IAppLogPresenter(parent), m_model(model), m_view(view) {
  // Connects view/model signals to presenter slots
}

// Implements presenter slots
