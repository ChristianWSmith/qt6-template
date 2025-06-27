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

mkdir -p "${MODEL_DIR}"
mkdir -p "${PRESENTER_DIR}"
mkdir -p "${WIDGET_DIR}"
mkdir -p "${TESTS_FEATURES_DIR}"

FEATURE_DIR_RELATIVE=${FEATURE_DIR#"${SRC_DIR}/"}
MODEL_DIR_RELATIVE=${MODEL_DIR#"${SRC_DIR}/"}
PRESENTER_DIR_RELATIVE=${PRESENTER_DIR#"${SRC_DIR}/"}
WIDGET_DIR_RELATIVE=${WIDGET_DIR#"${SRC_DIR}/"}

# COMMON
cat > "${FEATURE_DIR}/${FEATURE_NAME_LOWER}common.h" <<EOF
#pragma once

class ${FEATURE_NAME_TITLE}Test;

EOF
format "${FEATURE_DIR}/${FEATURE_NAME_LOWER}common.h"

# MODEL HEADER
cat > "${MODEL_DIR}/${FEATURE_NAME_TITLE}Model.h" <<EOF
#pragma once
#include "../${FEATURE_NAME_LOWER}common.h"
#include "../../../core/IPersistenceProvider.h"
#include "../../../core/IModel.h"
#include <QMetaMethod>
#include <QObject>
#include <QtPlugin>

class ${FEATURE_NAME_TITLE}Model : public QObject, public IModel {
  Q_OBJECT

public:
  explicit ${FEATURE_NAME_TITLE}Model(IPersistenceProvider *provider = nullptr,
                                      QObject *parent = nullptr);
  void shutdown() override;

signals:
  // Signals emitted by this concrete Model

private:
  friend class ${FEATURE_NAME_TITLE}Test;

  IPersistenceProvider *m_provider;
  const QString m_key{APP_ID ".${FEATURE_NAME_TITLE}State"};

  void saveState() const override;
  void loadState() override;

  // Private data members holding the Model's state
};

EOF
format "${MODEL_DIR}/${FEATURE_NAME_TITLE}Model.h"

# MODEL CPP
cat > "${MODEL_DIR}/${FEATURE_NAME_TITLE}Model.cpp" <<EOF
#include "${FEATURE_NAME_TITLE}Model.h"

namespace {
// Define persistence keys
} // namespace

${FEATURE_NAME_TITLE}Model::${FEATURE_NAME_TITLE}Model(IPersistenceProvider *provider, QObject *parent)
    : QObject(parent), m_provider(provider) {
  ${FEATURE_NAME_TITLE}Model::loadState();
}

void ${FEATURE_NAME_TITLE}Model::shutdown() {
  // Perform model shutdown actions
  saveState();
}

void ${FEATURE_NAME_TITLE}Model::saveState() const {
  if (m_provider == nullptr) {
    return;
  }
  // QJsonObject obj;
  // ...
  // m_provider->saveState(m_key, obj);
}

void ${FEATURE_NAME_TITLE}Model::loadState() {
  if (m_provider == nullptr) {
    return;
  }
  // QJsonObject obj = m_provider->loadState(m_key);
  // ...
}

EOF
format "${MODEL_DIR}/${FEATURE_NAME_TITLE}Model.cpp"

# PRESENTER HEADER
cat > "${PRESENTER_DIR}/${FEATURE_NAME_TITLE}Presenter.h" <<EOF
#pragma once
#include "../${FEATURE_NAME_LOWER}common.h"
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
  friend class ${FEATURE_NAME_TITLE}Test;
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
    : QObject(parent), m_model(model), m_view(view) {

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

  if (m_model != nullptr) {
    modelFuture = QtConcurrent::run([this]() { m_model->shutdown(); });
  }
  if (m_view != nullptr) {
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
#include "../${FEATURE_NAME_LOWER}common.h"
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
  friend class ${FEATURE_NAME_TITLE}Test;
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
cat > "${TESTS_FEATURES_DIR}/${FEATURE_NAME_TITLE}Test.cpp" <<EOF
// NOLINTBEGIN
#include "${FEATURE_DIR_RELATIVE}/${FEATURE_NAME_LOWER}common.h"
#include "${MODEL_DIR_RELATIVE}/${FEATURE_NAME_TITLE}Model.h"
#include "${PRESENTER_DIR_RELATIVE}/${FEATURE_NAME_TITLE}Presenter.h"
#include "${WIDGET_DIR_RELATIVE}/${FEATURE_NAME_TITLE}Widget.h"

#include <gtest/gtest.h>

class ${FEATURE_NAME_TITLE}Test : public ::testing::Test {
protected:
  ${FEATURE_NAME_TITLE}Model model;
  ${FEATURE_NAME_TITLE}Widget view;
  ${FEATURE_NAME_TITLE}Presenter presenter;

  ${FEATURE_NAME_TITLE}Test() : model(nullptr, nullptr), view(nullptr), presenter(&model, &view) {}
};

TEST_F(${FEATURE_NAME_TITLE}Test, Placeholder) {
  EXPECT_TRUE(true);
}

#include "${FEATURE_NAME_TITLE}Test.moc"

// NOLINTEND

EOF
format "${TESTS_FEATURES_DIR}/${FEATURE_NAME_TITLE}Test.cpp"