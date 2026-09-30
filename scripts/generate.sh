#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source ${SCRIPT_DIR}/env.sh

TYPE="${1:-}"
NAME_TITLE="${2:-}"

usage() {
  echo "Usage: $(basename ${0}) [feature|widget] <Name>"
  echo "  - <Name> should be a TitleCase valid file name"
  exit 1
}

format() {
  if command -v clang-format &> /dev/null; then
    clang-format -i "${1}"
  fi
}

# Portable in-place edit: GNU sed -i and BSD/macOS sed -i '' are incompatible.
sed_inplace() {
  local expr="$1"
  local file="$2"
  sed "${expr}" "${file}" > "${file}.sedtmp"
  mv "${file}.sedtmp" "${file}"
}

if [[ ! "${NAME_TITLE}" =~ ^[A-Z][A-Za-z0-9]*$ ]]; then 
  usage
fi

if [[ "${TYPE}" != "feature" ]] && \
   [[ "${TYPE}" != "widget" ]]; then 
  usage
fi

NAME_LOWER="$(echo "${NAME_TITLE}" | tr '[:upper:]' '[:lower:]')"

if [[ "${TYPE}" == "feature" ]]; then
  TARGET_DIR="${FEATURES_DIR}/${NAME_LOWER}"

  MODEL_DIR="${TARGET_DIR}/model"
  PRESENTER_DIR="${TARGET_DIR}/presenter"
  WIDGET_DIR="${TARGET_DIR}/widget"

  TARGET_DIR_RELATIVE=${TARGET_DIR#"${SRC_DIR}/"}
  MODEL_DIR_RELATIVE=${MODEL_DIR#"${SRC_DIR}/"}
  PRESENTER_DIR_RELATIVE=${PRESENTER_DIR#"${SRC_DIR}/"}
  WIDGET_DIR_RELATIVE=${WIDGET_DIR#"${SRC_DIR}/"}

  PRESENTER_STRING="${NAME_TITLE}Presenter"
elif [[ "${TYPE}" == "widget" ]]; then
  TARGET_DIR="${WIDGETS_DIR}/${NAME_LOWER}"

  WIDGET_DIR="${TARGET_DIR}"

  PRESENTER_STRING="Presenter"
fi


if [[ -e "${TARGET_DIR}" ]]; then
  if [[ "${TYPE}" == "feature" ]]; then
    echo "Feature already exists."
  elif [[ "${TYPE}" == "widget" ]]; then
    echo "Widget already exists."
  fi
  exit 1
fi

# WIDGET HEADER
mkdir -p "${WIDGET_DIR}"
cat > "${WIDGET_DIR}/${NAME_TITLE}Widget.h" <<EOF
#pragma once
<GEN:COMMON_H>
#include "ui_${NAME_TITLE}Widget.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class ${NAME_TITLE}Widget : public QWidget {
  Q_OBJECT

public:
  explicit ${NAME_TITLE}Widget(QWidget *parent = nullptr);
  ~${NAME_TITLE}Widget();

  ${NAME_TITLE}Widget(const ${NAME_TITLE}Widget &) = delete;
  ${NAME_TITLE}Widget &operator=(const ${NAME_TITLE}Widget &) = delete;
  ${NAME_TITLE}Widget(${NAME_TITLE}Widget &&) = delete;
  ${NAME_TITLE}Widget &operator=(${NAME_TITLE}Widget &&) = delete;

signals:
  // Signals emitted by this Widget to be connected to ${PRESENTER_STRING} Slots

private slots:
  // Slots for UI events (auto-connected by Qt Designer)

private:
  <GEN:FRIEND_TEST>
  Ui::${NAME_TITLE}Widget *ui;
};

EOF

# WIDGET CPP
cat > "${WIDGET_DIR}/${NAME_TITLE}Widget.cpp" <<EOF
#include "${NAME_TITLE}Widget.h"

${NAME_TITLE}Widget::${NAME_TITLE}Widget(QWidget *parent)
    : QWidget(parent), ui(new Ui::${NAME_TITLE}Widget) {
  ui->setupUi(this);
}

${NAME_TITLE}Widget::~${NAME_TITLE}Widget() { delete ui; }

// Implements UI slots, typically emitting signals to the Presenter

EOF
format "${WIDGET_DIR}/${NAME_TITLE}Widget.cpp"

# WIDGET UI
cat > "${WIDGET_DIR}/${NAME_TITLE}Widget.ui" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <class>${NAME_TITLE}Widget</class>
 <widget class="QWidget" name="${NAME_TITLE}Widget">
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

if [[ "${TYPE}" == "widget" ]]; then
  sed_inplace '/<GEN:COMMON_H>/d' "${WIDGET_DIR}/${NAME_TITLE}Widget.h"
  sed_inplace '/<GEN:FRIEND_TEST>/d' "${WIDGET_DIR}/${NAME_TITLE}Widget.h"
  format "${WIDGET_DIR}/${NAME_TITLE}Widget.h"
  exit 0
else
  sed_inplace "s|<GEN:COMMON_H>|#include \"../${NAME_LOWER}common.h\"|g" "${WIDGET_DIR}/${NAME_TITLE}Widget.h"
  sed_inplace "s|<GEN:FRIEND_TEST>|friend class ${NAME_TITLE}Test;|g" "${WIDGET_DIR}/${NAME_TITLE}Widget.h"
  format "${WIDGET_DIR}/${NAME_TITLE}Widget.h"
fi

# COMMON
mkdir -p "${TARGET_DIR}"
cat > "${TARGET_DIR}/${NAME_LOWER}common.h" <<EOF
#pragma once

class ${NAME_TITLE}Test;

EOF
format "${TARGET_DIR}/${NAME_LOWER}common.h"

# MODEL HEADER
mkdir -p "${MODEL_DIR}"
cat > "${MODEL_DIR}/${NAME_TITLE}Model.h" <<EOF
#pragma once
#include "../../../core/IModel.h"
#include "../../../core/IPersistenceProvider.h"
#include "../${NAME_LOWER}common.h"
#include <QObject>

class ${NAME_TITLE}Model : public QObject, public IModel {
  Q_OBJECT

public:
  explicit ${NAME_TITLE}Model(IPersistenceProvider &provider,
                              QObject *parent = nullptr);

  // IModel
  void saveState() const override;
  void loadState() override;

  // Methods to be called by ${NAME_TITLE}Presenter

signals:
  // Signals emitted by this Model to be connected to ${NAME_TITLE}Presenter Slots

private:
  friend class ${NAME_TITLE}Test;

  IPersistenceProvider &m_provider;
  const QString m_key{APP_ID ".${NAME_TITLE}State"};

  // Private data members holding the Model's state
};

EOF
format "${MODEL_DIR}/${NAME_TITLE}Model.h"

# MODEL CPP
cat > "${MODEL_DIR}/${NAME_TITLE}Model.cpp" <<EOF
#include "${NAME_TITLE}Model.h"
#include "logging/logging.h"
#include <QJsonObject>

namespace {
// constexpr auto KEY_MY_VALUE = "myValue";
} // namespace

${NAME_TITLE}Model::${NAME_TITLE}Model(IPersistenceProvider &provider,
                                       QObject *parent)
    : QObject(parent), m_provider(provider) {
  qCDebug(appPersistence) << "${NAME_TITLE}Model instantiated";
  ${NAME_TITLE}Model::loadState();
}

void ${NAME_TITLE}Model::saveState() const {
  // QJsonObject obj;
  // obj[KEY_MY_VALUE] = m_myvalue;
  // auto result = m_provider.saveState(m_key, obj);
  // if (result.hasError()) {
  //   qCWarning(appPersistence) << "Failed to save state:" << toString(result.error());
  // }
}

void ${NAME_TITLE}Model::loadState() {
  // auto result = m_provider.loadState(m_key);
  // if (result.hasError()) {
  //   if (result.error() != PersistenceError::NotFound) {
  //     qCWarning(appPersistence) << "Failed to load state:" << toString(result.error());
  //   }
  //   return;
  // }
  // const auto obj = result.value();
  // m_myvalue = obj[KEY_MY_VALUE].toInt();
}

EOF
format "${MODEL_DIR}/${NAME_TITLE}Model.cpp"

# PRESENTER HEADER
mkdir -p "${PRESENTER_DIR}"
cat > "${PRESENTER_DIR}/${NAME_TITLE}Presenter.h" <<EOF
#pragma once
#include "../${NAME_LOWER}common.h"
#include "../model/${NAME_TITLE}Model.h"
#include "../widget/${NAME_TITLE}Widget.h"
#include <QObject>

class ${NAME_TITLE}Presenter : public QObject {
  Q_OBJECT

public:
  explicit ${NAME_TITLE}Presenter(${NAME_TITLE}Model *model,
                                   ${NAME_TITLE}Widget *view,
                                   QObject *parent = nullptr);

private slots:
  // Slots to be connected to ${NAME_TITLE}Model/${NAME_TITLE}Widget Signals

private:
  friend class ${NAME_TITLE}Test;

  // AppMainWindow owns model, widget, and presenter; presenter does not
  // outlive its dependencies. Raw pointers are non-owning; their lifetime is
  // structurally guaranteed by the composition root.
  ${NAME_TITLE}Model *m_model;
  ${NAME_TITLE}Widget *m_view;
};

EOF
format "${PRESENTER_DIR}/${NAME_TITLE}Presenter.h"

# PRESENTER CPP
cat > "${PRESENTER_DIR}/${NAME_TITLE}Presenter.cpp" <<EOF
#include "${NAME_TITLE}Presenter.h"
#include "../../../logging/logging.h"

${NAME_TITLE}Presenter::${NAME_TITLE}Presenter(${NAME_TITLE}Model *model,
                                                ${NAME_TITLE}Widget *view,
                                                QObject *parent)
    : QObject(parent), m_model(model), m_view(view) {
  Q_ASSERT(m_model != nullptr);
  Q_ASSERT(m_view != nullptr);

  // Connect ${NAME_TITLE}Model/${NAME_TITLE}Widget Signals to ${NAME_TITLE}Presenter Slots
  // Example:
  // connect(m_view, &${NAME_TITLE}Widget::someAction, this,
  //         &${NAME_TITLE}Presenter::handleSomeAction);
  // connect(m_model, &${NAME_TITLE}Model::someStateChanged, this,
  //         &${NAME_TITLE}Presenter::handleSomeStateChanged);

  qCDebug(appFeature) << "${NAME_TITLE}Presenter instantiated";
}

// Implements presenter slots

EOF
format "${PRESENTER_DIR}/${NAME_TITLE}Presenter.cpp"

# MODEL TEST
mkdir -p "${TESTS_FEATURES_DIR}"
cat > "${TESTS_FEATURES_DIR}/${NAME_TITLE}Test.cpp" <<EOF
// NOLINTBEGIN
#include "MemoryPersistenceProvider.h"
#include "features/${NAME_LOWER}/${NAME_LOWER}common.h"
#include "features/${NAME_LOWER}/model/${NAME_TITLE}Model.h"
#include "features/${NAME_LOWER}/presenter/${NAME_TITLE}Presenter.h"
#include "features/${NAME_LOWER}/widget/${NAME_TITLE}Widget.h"

#include <gtest/gtest.h>

class ${NAME_TITLE}Test : public ::testing::Test {
protected:
  MemoryPersistenceProvider provider;
  ${NAME_TITLE}Model model;
  ${NAME_TITLE}Widget view;
  ${NAME_TITLE}Presenter presenter;

  ${NAME_TITLE}Test()
      : model(provider, nullptr), view(nullptr), presenter(&model, &view) {}
};

TEST_F(${NAME_TITLE}Test, Placeholder) {
  EXPECT_TRUE(true);
}

#include "${NAME_TITLE}Test.moc"

// NOLINTEND

EOF
format "${TESTS_FEATURES_DIR}/${NAME_TITLE}Test.cpp"
