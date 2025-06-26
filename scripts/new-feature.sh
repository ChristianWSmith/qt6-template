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

mkdir -p "${MODEL_DIR}"
mkdir -p "${PRESENTER_DIR}"
mkdir -p "${WIDGET_DIR}"
mkdir -p "${TEST_MODEL_DIR}"
mkdir -p "${TEST_PRESENTER_DIR}"
mkdir -p "${TEST_WIDGET_DIR}"

MODEL_DIR_RELATIVE=${MODEL_DIR#"${SRC_DIR}/"}
PRESENTER_DIR_RELATIVE=${PRESENTER_DIR#"${SRC_DIR}/"}
WIDGET_DIR_RELATIVE=${WIDGET_DIR#"${SRC_DIR}/"}

# MODEL HEADER
cat > "${MODEL_DIR}/${FEATURE_NAME_TITLE}Model.h" <<EOF
#pragma once
#include "../../../core/IModel.h"
#include <QMetaMethod>
#include <QObject>
#include <QtPlugin>

class ${FEATURE_NAME_TITLE}Model : public QObject, public IModel {
  Q_OBJECT

public:
  explicit ${FEATURE_NAME_TITLE}Model(QObject *parent = nullptr);
  void shutdown() override;

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

${FEATURE_NAME_TITLE}Model::${FEATURE_NAME_TITLE}Model(QObject *parent) : QObject(parent) {}

void ${FEATURE_NAME_TITLE}Model::shutdown() {
  // Perform model shutdown actions
}

EOF
format "${MODEL_DIR}/${FEATURE_NAME_TITLE}Model.cpp"

# PRESENTER HEADER
cat > "${PRESENTER_DIR}/${FEATURE_NAME_TITLE}Presenter.h" <<EOF
#pragma once
#include "../../../core/IPresenter.h"
#include "../model/${FEATURE_NAME_TITLE}Model.h"
#include "../widget/${FEATURE_NAME_TITLE}Widget.h"
#include <QObject>
#include <QtPlugin>

class ${FEATURE_NAME_TITLE}Presenter : public QObject, public IPresenter {
  Q_OBJECT

public:
  explicit ${FEATURE_NAME_TITLE}Presenter(${FEATURE_NAME_TITLE}Model *model,
                                   ${FEATURE_NAME_TITLE}Widget *view,
                                   QObject *parent = nullptr);
  void shutdown() override;

private slots:
  // Concrete slots for handling events

private:
  ${FEATURE_NAME_TITLE}Model *m_model;
  ${FEATURE_NAME_TITLE}Widget *m_view;
};

EOF
format "${PRESENTER_DIR}/${FEATURE_NAME_TITLE}Presenter.h"

# PRESENTER CPP
cat > "${PRESENTER_DIR}/${FEATURE_NAME_TITLE}Presenter.cpp" <<EOF
#include "${FEATURE_NAME_TITLE}Presenter.h"
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

${FEATURE_NAME_TITLE}Presenter::${FEATURE_NAME_TITLE}Presenter(${FEATURE_NAME_TITLE}Model *model,
                                                 ${FEATURE_NAME_TITLE}Widget *view,
                                                 QObject *parent)
    : m_model(model), m_view(view) {

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
  QFuture<void> modelFuture;
  QFuture<void> viewFuture;

  if (m_view != nullptr) {
    modelFuture = QtConcurrent::run([this]() { m_model->shutdown(); });
  }
  if (m_model != nullptr) {
    viewFuture = QtConcurrent::run([this]() { m_view->shutdown(); });
  }

  modelFuture.waitForFinished();
  viewFuture.waitForFinished();
}

EOF
format "${PRESENTER_DIR}/${FEATURE_NAME_TITLE}Presenter.cpp"

# WIDGET HEADER
cat > "${WIDGET_DIR}/${FEATURE_NAME_TITLE}Widget.h" <<EOF
#pragma once
#include "../../../core/IWidget.h"
#include "ui_${FEATURE_NAME_TITLE}Widget.h"
#include <QWidget>
#include <QtPlugin>

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class ${FEATURE_NAME_TITLE}Widget : public QWidget, public IWidget {
  Q_OBJECT

public:
  explicit ${FEATURE_NAME_TITLE}Widget(QWidget *parent = nullptr);
  ~${FEATURE_NAME_TITLE}Widget();

  ${FEATURE_NAME_TITLE}Widget(const ${FEATURE_NAME_TITLE}Widget &) = delete;
  ${FEATURE_NAME_TITLE}Widget &operator=(const ${FEATURE_NAME_TITLE}Widget &) = delete;
  ${FEATURE_NAME_TITLE}Widget(${FEATURE_NAME_TITLE}Widget &&) = delete;
  ${FEATURE_NAME_TITLE}Widget &operator=(${FEATURE_NAME_TITLE}Widget &&) = delete;

  void shutdown() override;

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
    : ui(new Ui::${FEATURE_NAME_TITLE}Widget) {
  ui->setupUi(this);
}

${FEATURE_NAME_TITLE}Widget::~${FEATURE_NAME_TITLE}Widget() { delete ui; }

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
cat > "${TEST_MODEL_DIR}/${FEATURE_NAME_TITLE}ModelTest.cpp" <<EOF
// NOLINTBEGIN
#include "${MODEL_DIR_RELATIVE}/${FEATURE_NAME_TITLE}Model.h"
#include <gtest/gtest.h>

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
cat > "${TEST_PRESENTER_DIR}/${FEATURE_NAME_TITLE}PresenterTest.cpp" <<EOF
// NOLINTBEGIN
#include "${PRESENTER_DIR_RELATIVE}/${FEATURE_NAME_TITLE}Presenter.h"
#include "${MODEL_DIR_RELATIVE}/${FEATURE_NAME_TITLE}Model.h"
#include "${WIDGET_DIR_RELATIVE}/${FEATURE_NAME_TITLE}Widget.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::NiceMock;

class Mock${FEATURE_NAME_TITLE}Model : public ${FEATURE_NAME_TITLE}Model {
public:
  // Mock model methods
};

class Mock${FEATURE_NAME_TITLE}Widget : public ${FEATURE_NAME_TITLE}Widget {
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
TEST_F(${FEATURE_NAME_TITLE}PresenterTest, Placeholder) { EXPECT_TRUE(true); }
// NOLINTEND

EOF
format "${TEST_PRESENTER_DIR}/${FEATURE_NAME_TITLE}PresenterTest.cpp"

# WIDGET TEST
cat > "${TEST_WIDGET_DIR}/${FEATURE_NAME_TITLE}WidgetTest.cpp" <<EOF
// NOLINTBEGIN
#include "${WIDGET_DIR_RELATIVE}/${FEATURE_NAME_TITLE}Widget.h"
#include <gtest/gtest.h>

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
TEST_F(${FEATURE_NAME_TITLE}WidgetTest, Placeholder) { EXPECT_TRUE(true); }
// NOLINTEND

EOF
format "${TEST_WIDGET_DIR}/${FEATURE_NAME_TITLE}WidgetTest.cpp"
