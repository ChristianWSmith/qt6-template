#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

FEATURE_NAME_TITLE="${1:-}"

usage() {
    echo "$(basename ${0}) <FeatureName>"
    echo "  - feature name should be a TitleCase valid file name"
    exit 1
}

if [[ "${FEATURE_NAME_TITLE}" == "" ]] || \
    [[ ! "${FEATURE_NAME_TITLE}" =~ ^[A-Z] ]] || \
    [[ ! "${FEATURE_NAME_TITLE}" =~ [A-Za-z]? ]]; then 
    usage
fi

FEATURE_NAME_UPPER="$(echo "${FEATURE_NAME_TITLE}" | tr '[:lower:]' '[:upper:]')"
FEATURE_NAME_LOWER="$(echo "${FEATURE_NAME_TITLE}" | tr '[:upper:]' '[:lower:]')"

FEATURE_DIR="${FEATURES_DIR}/${FEATURE_NAME_LOWER}"

MODEL_DIR="${FEATURE_DIR}/model"
PRESENTER_DIR="${FEATURE_DIR}/presenter"
WIDGET_DIR="${FEATURE_DIR}/widget"

# COMMON
mkdir -p "${FEATURE_DIR}"
cat > "${FEATURE_DIR}/${FEATURE_NAME_LOWER}.h" <<EOF
#define ${FEATURE_NAME_UPPER}_FEATURE_ID APP_ID ".${FEATURE_NAME_TITLE}"
EOF

# MODEL
mkdir -p "${MODEL_DIR}"
cat > "${MODEL_DIR}/I${FEATURE_NAME_TITLE}Model.h" <<EOF
#ifndef I${FEATURE_NAME_UPPER}MODEL_H
#define I${FEATURE_NAME_UPPER}MODEL_H
#include "../../featurescommon.h"
#include "../${FEATURE_NAME_LOWER}common.h"
#include <QMetaMethod>
#include <QObject>
#include <QtPlugin>

class I${FEATURE_NAME_TITLE}Model : public QObject {
  Q_OBJECT

public:
  explicit I${FEATURE_NAME_TITLE}Model(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~I${FEATURE_NAME_TITLE}Model() = default;
  // methods and connections
};

Q_DECLARE_INTERFACE(I${FEATURE_NAME_TITLE}Model, ${FEATURE_NAME_UPPER}_FEATURE_ID FEATURE_MODEL_SUFFIX)

#endif

EOF
cat > "${MODEL_DIR}/${FEATURE_NAME_TITLE}Model.h" <<EOF
#ifndef ${FEATURE_NAME_UPPER}MODEL_H
#define ${FEATURE_NAME_UPPER}MODEL_H
#include "I${FEATURE_NAME_TITLE}Model.h"

class ${FEATURE_NAME_TITLE}Model : public I${FEATURE_NAME_TITLE}Model {
  Q_OBJECT
  Q_INTERFACES(I${FEATURE_NAME_TITLE}Model)

public:
  explicit ${FEATURE_NAME_TITLE}Model(QObject *parent = nullptr);
  // methods and connections

signals:
  // signals

private:
  // data
};

#endif

EOF
cat > "${MODEL_DIR}/${FEATURE_NAME_TITLE}Model.cpp" <<EOF
#include "${FEATURE_NAME_TITLE}Model.h"

// method and connections impl

EOF

# PRESENTER
mkdir -p "${PRESENTER_DIR}"
cat > "${PRESENTER_DIR}/I${FEATURE_NAME_TITLE}Presenter.h" <<EOF
#ifndef I${FEATURE_NAME_UPPER}PRESENTER_H
#define I${FEATURE_NAME_UPPER}PRESENTER_H
#include "../../featurescommon.h"
#include "../${FEATURE_NAME_LOWER}common.h"
#include <QObject>
#include <QtPlugin>

class I${FEATURE_NAME_TITLE}Presenter : public QObject {
  Q_OBJECT

public:
  explicit I${FEATURE_NAME_TITLE}Presenter(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~I${FEATURE_NAME_TITLE}Presenter() = default;
};

Q_DECLARE_INTERFACE(I${FEATURE_NAME_TITLE}Presenter,
                    ${FEATURE_NAME_UPPER}_FEATURE_ID FEATURE_PRESENTER_SUFFIX)

#endif

EOF
cat > "${PRESENTER_DIR}/${FEATURE_NAME_TITLE}Presenter.h" <<EOF
#ifndef ${FEATURE_NAME_UPPER}PRESENTER_H
#define ${FEATURE_NAME_UPPER}PRESENTER_H
#include "../model/I${FEATURE_NAME_TITLE}Model.h"
#include "../widget/I${FEATURE_NAME_TITLE}Widget.h"
#include "I${FEATURE_NAME_TITLE}Presenter.h"
#include <QObject>

class ${FEATURE_NAME_TITLE}Presenter : public I${FEATURE_NAME_TITLE}Presenter {
  Q_OBJECT
  Q_INTERFACES(I${FEATURE_NAME_TITLE}Presenter)

public:
  explicit ${FEATURE_NAME_TITLE}Presenter(I${FEATURE_NAME_TITLE}Model *model, I${FEATURE_NAME_TITLE}Widget *view,
                            QObject *parent = nullptr);

private slots:
  // slots

private:
  I${FEATURE_NAME_TITLE}Model *m_model;
  I${FEATURE_NAME_TITLE}Widget *m_view;
};

#endif

EOF
cat > "${PRESENTER_DIR}/${FEATURE_NAME_TITLE}Presenter.cpp" <<EOF
#include "${FEATURE_NAME_TITLE}Presenter.h"

${FEATURE_NAME_TITLE}Presenter::${FEATURE_NAME_TITLE}Presenter(I${FEATURE_NAME_TITLE}Model *model, I${FEATURE_NAME_TITLE}Widget *view,
                                   QObject *parent)
    : I${FEATURE_NAME_TITLE}Presenter(parent), m_model(model), m_view(view) {
  // connect signals to presenter slots
}

// implement presenter slots

EOF

# WIDGET
mkdir -p "${WIDGET_DIR}"
cat > "${WIDGET_DIR}/I${FEATURE_NAME_TITLE}Widget.h" <<EOF
#ifndef I${FEATURE_NAME_UPPER}WIDGET_H
#define I${FEATURE_NAME_UPPER}WIDGET_H
#include "../../featurescommon.h"
#include "../${FEATURE_NAME_LOWER}common.h"
#include <QWidget>
#include <QtPlugin>

class I${FEATURE_NAME_TITLE}Widget : public QWidget {
  Q_OBJECT

public:
  explicit I${FEATURE_NAME_TITLE}Widget(QWidget *parent = nullptr) : QWidget(parent) {}
  virtual ~I${FEATURE_NAME_TITLE}Widget() = default;
  // methods and connections
};

Q_DECLARE_INTERFACE(I${FEATURE_NAME_TITLE}Widget, ${FEATURE_NAME_UPPER}_FEATURE_ID FEATURE_WIDGET_SUFFIX)

#endif

EOF
cat > "${WIDGET_DIR}/${FEATURE_NAME_TITLE}Widget.h" <<EOF
#ifndef ${FEATURE_NAME_UPPER}WIDGET_H
#define ${FEATURE_NAME_UPPER}WIDGET_H
#include "I${FEATURE_NAME_TITLE}Widget.h"
#include "ui_${FEATURE_NAME_TITLE}Widget.h"

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class ${FEATURE_NAME_TITLE}Widget : public I${FEATURE_NAME_TITLE}Widget {
  Q_OBJECT
  Q_INTERFACES(I${FEATURE_NAME_TITLE}Widget)

public:
  explicit ${FEATURE_NAME_TITLE}Widget(QWidget *parent = nullptr);
  ~${FEATURE_NAME_TITLE}Widget();
  // methods and connections

signals:
  // signals

private slots:
  // slots (names must align with ui on_<elementName>_<eventType>)

private:
  Ui::${FEATURE_NAME_TITLE}Widget *ui;
};

#endif

EOF
cat > "${WIDGET_DIR}/${FEATURE_NAME_TITLE}Widget.cpp" <<EOF
#include "${FEATURE_NAME_TITLE}Widget.h"

${FEATURE_NAME_TITLE}Widget::${FEATURE_NAME_TITLE}Widget(QWidget *parent)
    : I${FEATURE_NAME_TITLE}Widget(parent), ui(new Ui::${FEATURE_NAME_TITLE}Widget) {
  ui->setupUi(this);
}

${FEATURE_NAME_TITLE}Widget::~${FEATURE_NAME_TITLE}Widget() { delete ui; }

// implement methods and connections

// implement slots (emit signal)

EOF
cat > "${WIDGET_DIR}/${FEATURE_NAME_TITLE}Widget.ui" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <class>${FEATURE_NAME_TITLE}Widget</class>
 <widget class="QWidget" name="${FEATURE_NAME_TITLE}Widget">
  <property name="geometry">
   <rect>
    <x>0</x>
    <y>0</y>
    <width>12</width>
    <height>12</height>
   </rect>
  </property>
  <layout class="QVBoxLayout" name="verticalLayout"/>
 </widget>
 <resources/>
 <connections/>
</ui>

EOF
