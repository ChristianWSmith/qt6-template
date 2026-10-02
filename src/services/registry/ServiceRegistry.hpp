#pragma once

namespace services {

/// Retains free-function EventSystem subscriptions only. Qt-object services
/// use QObject parent-child ownership. No general DI container.
///
/// Explicit service registration lifecycle:
/// Precondition: registerAll() runs after QApplication construction.
/// EventSystem free-function subscribe requires a running QCoreApplication;
/// guards otherwise log qCCritical and return an empty Subscription.
/// QApplication exists → registerAll() → application runs → unregisterAll()
/// → QApplication destruction.
void registerAll();
void unregisterAll();

} // namespace services
