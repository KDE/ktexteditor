#include "emulatedcommandbarsetupandteardown.h"

#include <kateconfig.h>
#include <kateview.h>
#include <inputmode/kateviinputmode.h>
#include <vimode/emulatedcommandbar.h>

#include <QMainWindow>
#include <QApplication>
#include <QMetaObject>

//BEGIN: WindowKeepActive

WindowKeepActive::WindowKeepActive(QMainWindow *mainWindow)
: m_mainWindow(mainWindow)
{
    /* There's nothing to do here. */
}

bool WindowKeepActive::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object);

    if (event->type() == QEvent::WindowDeactivate) {
        // With some combinations of Qt and Xvfb, invoking/ dismissing a popup
        // will deactiveate the m_mainWindow, preventing it from receiving shortcuts.
        // If we detect this, set it back to being the active window again.
        event->ignore();
        QApplication::setActiveWindow(m_mainWindow);
        return true;
    }
    return false;
}

//END: WindowKeepActive

//BEGIN: EmulatedCommandBarSetUpAndTearDown

EmulatedCommandBarSetUpAndTearDown::EmulatedCommandBarSetUpAndTearDown(KateViInputMode *inputMode,
        KTextEditor::ViewPrivate *view,
        QMainWindow *window)
: m_view(view), m_window(window), m_windowKeepActive(window), m_viInputMode(inputMode)
{
    m_window->show();
    m_view->show();
    QApplication::setActiveWindow(m_window);
    m_view->setFocus();
    while (QApplication::hasPendingEvents()) {
        QApplication::processEvents();
    }
    KateViewConfig::global()->setViInputModeStealKeys(true);
    m_window->installEventFilter(&m_windowKeepActive);
}
EmulatedCommandBarSetUpAndTearDown::~EmulatedCommandBarSetUpAndTearDown()
{
    m_window->removeEventFilter(&m_windowKeepActive);
    // Use invokeMethod to avoid having to export KateViewBar for testing.
    QMetaObject::invokeMethod(m_viInputMode->viModeEmulatedCommandBar(), "hideMe");
    m_view->hide();
    m_window->hide();
    KateViewConfig::global()->setViInputModeStealKeys(false);
    while (QApplication::hasPendingEvents()) {
        QApplication::processEvents();
    }
}

//END: EmulatedCommandBarSetUpAndTearDown

