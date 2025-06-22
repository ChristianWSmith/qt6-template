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

format() {
    if [[ ! -z "$(which clang-format)" ]]; then
        clang-format -i "${1}"
    fi
}

if [[ "${FEATURE_NAME_TITLE}" == "" ]] || \
    [[ ! "${FEATURE_NAME_TITLE}" =~ ^[A-Z] ]] || \
    [[ ! "${FEATURE_NAME_TITLE}" =~ [A-Za-z]? ]]; then 
    usage
fi

FEATURE_NAME_UPPER="$(echo "${FEATURE_NAME_TITLE}" | tr '[:lower:]' '[:upper:]')"
FEATURE_NAME_LOWER="$(echo "${FEATURE_NAME_TITLE}" | tr '[:upper:]' '[:lower:]')"

FEATURE_DIR="${FEATURES_DIR}/${FEATURE_NAME_LOWER}"

if [[ -e "${FEATURE_DIR}" ]]; then
  echo "Feature already exists."
  exit 1
fi

MODEL_DIR="${FEATURE_DIR}/model"
PRESENTER_DIR="${FEATURE_DIR}/presenter"
WIDGET_DIR="${FEATURE_DIR}/widget"

TEST_FEATURE_DIR="${TESTS_FEATURES_DIR}/${FEATURE_NAME_LOWER}"
TEST_MODEL_DIR="${TEST_FEATURE_DIR}/model"
TEST_PRESENTER_DIR="${TEST_FEATURE_DIR}/presenter"
TEST_WIDGET_DIR="${TEST_FEATURE_DIR}/widget"

MODEL_DIR_RELATIVE=${MODEL_DIR#"${SRC_DIR}/"}
PRESENTER_DIR_RELATIVE=${PRESENTER_DIR#"${SRC_DIR}/"}
WIDGET_DIR_RELATIVE=${WIDGET_DIR#"${SRC_DIR}/"}

# COMMON
mkdir -p "${FEATURE_DIR}"
cat > "${FEATURE_DIR}/${FEATURE_NAME_LOWER}common.h" <<EOF
#pragma once

#define ${FEATURE_NAME_UPPER}_FEATURE_ID APP_ID ".${FEATURE_NAME_TITLE}"

EOF
format "${FEATURE_DIR}/${FEATURE_NAME_LOWER}common.h"

# MODEL INTERFACE
mkdir -p "${MODEL_DIR}"
cat > "${MODEL_DIR}/I${FEATURE_NAME_TITLE}Model.h" <<EOF
#pragma once
#include "../../IModel.h"
#include "../../featurescommon.h"
#include "../${FEATURE_NAME_LOWER}common.h"
#include <QMetaMethod>
#include <QObject>
#include <QtPlugin>

class I${FEATURE_NAME_TITLE}Model : public QObject, public IModel {
  Q_OBJECT

public:
  explicit I${FEATURE_NAME_TITLE}Model(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~I${FEATURE_NAME_TITLE}Model() = default;

  I${FEATURE_NAME_TITLE}Model(const I${FEATURE_NAME_TITLE}Model &) = delete;
  I${FEATURE_NAME_TITLE}Model &operator=(const I${FEATURE_NAME_TITLE}Model &) = delete;
  I${FEATURE_NAME_TITLE}Model(I${FEATURE_NAME_TITLE}Model &&) = delete;
  I${FEATURE_NAME_TITLE}Model &operator=(I${FEATURE_NAME_TITLE}Model &&) = delete;

  // Model API methods and connections
};

Q_DECLARE_INTERFACE(I${FEATURE_NAME_TITLE}Model,
                    ${FEATURE_NAME_UPPER}_FEATURE_ID FEATURE_MODEL_SUFFIX)

EOF
format "${MODEL_DIR}/I${FEATURE_NAME_TITLE}Model.h"

# MODEL HEADER
cat > "${MODEL_DIR}/${FEATURE_NAME_TITLE}Model.h" <<EOF
#pragma once
#include "I${FEATURE_NAME_TITLE}Model.h"

class ${FEATURE_NAME_TITLE}Model : public I${FEATURE_NAME_TITLE}Model {
  Q_OBJECT
  Q_INTERFACES(I${FEATURE_NAME_TITLE}Model)

public:
  explicit ${FEATURE_NAME_TITLE}Model(QObject *parent = nullptr);
  void shutdown() override;
  // Implements I${FEATURE_NAME_TITLE}Model methods

signals:
  // Signals emitted by this concrete Model

private:
  // Private data members holding the Model's state
};

EOF
format "${MODEL_DIR}/${FEATURE_NAME_TITLE}Model.h"

# MODEL CPP
cat > "${MODEL_DIR}/${FEATURE_NAME_TITLE}Model.cpp" <<EOF
#include "${FEATURE_NAME_TITLE}Model.h"

${FEATURE_NAME_TITLE}Model::${FEATURE_NAME_TITLE}Model(QObject *parent)
    : I${FEATURE_NAME_TITLE}Model(parent) {}

// Implementation of I${FEATURE_NAME_TITLE}Model methods and internal connections

void ${FEATURE_NAME_TITLE}Model::shutdown() {
  // Perform model shutdown actions
}

EOF
format "${MODEL_DIR}/${FEATURE_NAME_TITLE}Model.cpp"

# PRESENTER INTERFACE
mkdir -p "${PRESENTER_DIR}"
cat > "${PRESENTER_DIR}/I${FEATURE_NAME_TITLE}Presenter.h" <<EOF
#pragma once
#include "../../IPresenter.h"
#include "../../featurescommon.h"
#include "../${FEATURE_NAME_LOWER}common.h"
#include <QObject>
#include <QtPlugin>

class I${FEATURE_NAME_TITLE}Presenter : public QObject, public IPresenter {
  Q_OBJECT

public:
  explicit I${FEATURE_NAME_TITLE}Presenter(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~I${FEATURE_NAME_TITLE}Presenter() = default;

  I${FEATURE_NAME_TITLE}Presenter(const I${FEATURE_NAME_TITLE}Presenter &) = delete;
  I${FEATURE_NAME_TITLE}Presenter &operator=(const I${FEATURE_NAME_TITLE}Presenter &) = delete;
  I${FEATURE_NAME_TITLE}Presenter(I${FEATURE_NAME_TITLE}Presenter &&) = delete;
  I${FEATURE_NAME_TITLE}Presenter &operator=(I${FEATURE_NAME_TITLE}Presenter &&) = delete;
};

Q_DECLARE_INTERFACE(I${FEATURE_NAME_TITLE}Presenter,
                    ${FEATURE_NAME_UPPER}_FEATURE_ID FEATURE_PRESENTER_SUFFIX)

EOF
format "${PRESENTER_DIR}/I${FEATURE_NAME_TITLE}Presenter.h"

# PRESENTER HEADER
cat > "${PRESENTER_DIR}/${FEATURE_NAME_TITLE}Presenter.h" <<EOF
#pragma once
#include "../model/I${FEATURE_NAME_TITLE}Model.h"
#include "../widget/I${FEATURE_NAME_TITLE}Widget.h"
#include "I${FEATURE_NAME_TITLE}Presenter.h"
#include <QObject>

class ${FEATURE_NAME_TITLE}Presenter : public I${FEATURE_NAME_TITLE}Presenter {
  Q_OBJECT
  Q_INTERFACES(I${FEATURE_NAME_TITLE}Presenter)

public:
  explicit ${FEATURE_NAME_TITLE}Presenter(I${FEATURE_NAME_TITLE}Model *model,
                                I${FEATURE_NAME_TITLE}Widget *view,
                                QObject *parent = nullptr);
  void shutdown() override;

private slots:
  // Concrete slots for handling events

private:
  I${FEATURE_NAME_TITLE}Model *m_model;
  I${FEATURE_NAME_TITLE}Widget *m_view;
};

EOF
format "${PRESENTER_DIR}/${FEATURE_NAME_TITLE}Presenter.h"

# PRESENTER CPP
cat > "${PRESENTER_DIR}/${FEATURE_NAME_TITLE}Presenter.cpp" <<EOF
#include "${FEATURE_NAME_TITLE}Presenter.h"

${FEATURE_NAME_TITLE}Presenter::${FEATURE_NAME_TITLE}Presenter(I${FEATURE_NAME_TITLE}Model *model,
                                           I${FEATURE_NAME_TITLE}Widget *view,
                                           QObject *parent)
    : I${FEATURE_NAME_TITLE}Presenter(parent), m_model(model), m_view(view) {

  if (m_model == nullptr) {
    qWarning() << "${FEATURE_NAME_TITLE}Presenter instantiated without model";
  }
  if (m_view == nullptr) {
    qWarning() << "${FEATURE_NAME_TITLE}Presenter instantiated without view";
  }
  // Connects view/model signals to presenter slots
}

// Implements presenter slots

void ${FEATURE_NAME_TITLE}Presenter::shutdown() {
  if (m_view != nullptr) {
    m_view->shutdown();
  }
  if (m_model != nullptr) {
    m_model->shutdown();
  }
}

EOF
format "${PRESENTER_DIR}/${FEATURE_NAME_TITLE}Presenter.cpp"

# WIDGET INTERFACE
mkdir -p "${WIDGET_DIR}"
cat > "${WIDGET_DIR}/I${FEATURE_NAME_TITLE}Widget.h" <<EOF
#pragma once
#include "../../IWidget.h"
#include "../../featurescommon.h"
#include "../${FEATURE_NAME_LOWER}common.h"
#include <QWidget>
#include <QtPlugin>

class I${FEATURE_NAME_TITLE}Widget : public QWidget, public IWidget {
  Q_OBJECT

public:
  explicit I${FEATURE_NAME_TITLE}Widget(QWidget *parent = nullptr) : QWidget(parent) {}
  virtual ~I${FEATURE_NAME_TITLE}Widget() = default;

  I${FEATURE_NAME_TITLE}Widget(const I${FEATURE_NAME_TITLE}Widget &) = delete;
  I${FEATURE_NAME_TITLE}Widget &operator=(const I${FEATURE_NAME_TITLE}Widget &) = delete;
  I${FEATURE_NAME_TITLE}Widget(I${FEATURE_NAME_TITLE}Widget &&) = delete;
  I${FEATURE_NAME_TITLE}Widget &operator=(I${FEATURE_NAME_TITLE}Widget &&) = delete;
  // Widget API methods and connections
};

Q_DECLARE_INTERFACE(I${FEATURE_NAME_TITLE}Widget,
                    ${FEATURE_NAME_UPPER}_FEATURE_ID FEATURE_WIDGET_SUFFIX)

EOF
format "${WIDGET_DIR}/I${FEATURE_NAME_TITLE}Widget.h"

# WIDGET HEADER
cat > "${WIDGET_DIR}/${FEATURE_NAME_TITLE}Widget.h" <<EOF
#pragma once
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

  ${FEATURE_NAME_TITLE}Widget(const ${FEATURE_NAME_TITLE}Widget &) = delete;
  ${FEATURE_NAME_TITLE}Widget &operator=(const ${FEATURE_NAME_TITLE}Widget &) = delete;
  ${FEATURE_NAME_TITLE}Widget(${FEATURE_NAME_TITLE}Widget &&) = delete;
  ${FEATURE_NAME_TITLE}Widget &operator=(${FEATURE_NAME_TITLE}Widget &&) = delete;

  void shutdown() override;

  // Implements I${FEATURE_NAME_TITLE}Widget methods

signals:
  // Signals emitted by this concrete Widget

private slots:
  // Slots for UI events (auto-connected by Qt Designer)

private:
  Ui::${FEATURE_NAME_TITLE}Widget *ui;
};

EOF
format "${WIDGET_DIR}/${FEATURE_NAME_TITLE}Widget.h"

# WIDGET CPP
cat > "${WIDGET_DIR}/${FEATURE_NAME_TITLE}Widget.cpp" <<EOF
#include "${FEATURE_NAME_TITLE}Widget.h"

${FEATURE_NAME_TITLE}Widget::${FEATURE_NAME_TITLE}Widget(QWidget *parent)
    : I${FEATURE_NAME_TITLE}Widget(parent), ui(new Ui::${FEATURE_NAME_TITLE}Widget) {
  ui->setupUi(this);
}

${FEATURE_NAME_TITLE}Widget::~${FEATURE_NAME_TITLE}Widget() { delete ui; }

// Implements I${FEATURE_NAME_TITLE}Widget methods and internal connections

// Implements UI slots, typically emitting signals to the Presenter

void ${FEATURE_NAME_TITLE}Widget::shutdown() {}

EOF
format "${WIDGET_DIR}/${FEATURE_NAME_TITLE}Widget.cpp"

# WIDGET UI
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

# MODEL TEST
mkdir -p "${TEST_MODEL_DIR}"
cat > "${TEST_MODEL_DIR}/${FEATURE_NAME_TITLE}ModelTest.cpp" <<EOF
// NOLINTBEGIN
#include <gtest/gtest.h>
#include "${MODEL_DIR_RELATIVE}/${FEATURE_NAME_TITLE}Model.h"

class ${FEATURE_NAME_TITLE}ModelTest : public ::testing::Test {
protected:
  ${FEATURE_NAME_TITLE}Model *model;

  void SetUp() override { model = new ${FEATURE_NAME_TITLE}Model(); }

  void TearDown() override { delete model; }
};

TEST_F(${FEATURE_NAME_TITLE}ModelTest, Placeholder) {
  // Add your model tests here
  EXPECT_TRUE(true);
}
// NOLINTEND

EOF
format "${TEST_MODEL_DIR}/${FEATURE_NAME_TITLE}ModelTest.cpp" 

# PRESENTER TEST
mkdir -p "${TEST_PRESENTER_DIR}"
cat > "${TEST_PRESENTER_DIR}/${FEATURE_NAME_TITLE}PresenterTest.cpp" <<EOF
// NOLINTBEGIN
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "${PRESENTER_DIR_RELATIVE}/${FEATURE_NAME_TITLE}Presenter.h"
#include "${MODEL_DIR_RELATIVE}/I${FEATURE_NAME_TITLE}Model.h"
#include "${WIDGET_DIR_RELATIVE}/I${FEATURE_NAME_TITLE}Widget.h"

using ::testing::NiceMock;

class Mock${FEATURE_NAME_TITLE}Model : public I${FEATURE_NAME_TITLE}Model {
public:
  // Mock model methods
};

class Mock${FEATURE_NAME_TITLE}Widget : public I${FEATURE_NAME_TITLE}Widget {
public:
  // Mock widget methods
};

class ${FEATURE_NAME_TITLE}PresenterTest : public ::testing::Test {
protected:
  Mock${FEATURE_NAME_TITLE}Model *mockModel;
  Mock${FEATURE_NAME_TITLE}Widget *mockWidget;
  ${FEATURE_NAME_TITLE}Presenter *presenter;

  void SetUp() override {
    mockModel = new Mock${FEATURE_NAME_TITLE}Model();
    mockWidget = new Mock${FEATURE_NAME_TITLE}Widget();
    presenter = new ${FEATURE_NAME_TITLE}Presenter(mockModel, mockWidget);
  }

  void TearDown() override {
    delete presenter;
    delete mockModel;
    delete mockWidget;
  }

};

// Add your presenter tests here
TEST_F(${FEATURE_NAME_TITLE}PresenterTest, Placeholder) {
  EXPECT_TRUE(true);
}
// NOLINTEND

EOF
format "${TEST_PRESENTER_DIR}/${FEATURE_NAME_TITLE}PresenterTest.cpp"

# WIDGET TEST
mkdir -p "${TEST_WIDGET_DIR}"
cat > "${TEST_WIDGET_DIR}/${FEATURE_NAME_TITLE}WidgetTest.cpp" <<EOF
// NOLINTBEGIN
#include <gtest/gtest.h>
#include "${WIDGET_DIR_RELATIVE}/${FEATURE_NAME_TITLE}Widget.h"

class ${FEATURE_NAME_TITLE}WidgetTest : public ::testing::Test {
protected:
  ${FEATURE_NAME_TITLE}Widget *widget = nullptr;

  void SetUp() override {
    widget = new ${FEATURE_NAME_TITLE}Widget();
    widget->show();
  }

  void TearDown() override { 
    widget->hide();
    delete widget; 
  }
};

// Add your widget tests here
TEST_F(${FEATURE_NAME_TITLE}WidgetTest, Placeholder) {
  EXPECT_TRUE(true);
}
// NOLINTEND

EOF
format "${TEST_WIDGET_DIR}/${FEATURE_NAME_TITLE}WidgetTest.cpp"
