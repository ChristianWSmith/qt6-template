#pragma once

#include "../../events/system/EventSystem.hpp"

namespace services {

/// Explicit service registration lifecycle:
/// QApplication exists → registerAll() → application runs → unregisterAll()
/// → QApplication destruction.
void registerAll();
void unregisterAll();

} // namespace services
