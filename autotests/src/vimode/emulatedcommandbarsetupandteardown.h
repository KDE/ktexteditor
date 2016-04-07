#include <QObject>

namespace KTextEditor
{
    class ViewPrivate;
}

class KateViInputMode;
class QMainWindow;

/**
 * This class is used by the EmulatedCommandBarSetUpAndTearDown class so
 * the main window is active all the time.
 */
class WindowKeepActive : public QObject
{
    Q_OBJECT

    public:
        WindowKeepActive(QMainWindow *mainWindow);

        public Q_SLOTS:
            bool eventFilter(QObject *object, QEvent *event) Q_DECL_OVERRIDE;

    private:
        QMainWindow *m_mainWindow;
};

/**
 * Helper class that is used to setup and tear down tests affecting
 * the command bar in any way.
 */
class EmulatedCommandBarSetUpAndTearDown
{
    public:
        EmulatedCommandBarSetUpAndTearDown(KateViInputMode *inputMode,
                KTextEditor::ViewPrivate *view,
                QMainWindow *window);

        ~EmulatedCommandBarSetUpAndTearDown();

    private:
        KTextEditor::ViewPrivate *m_view;
        QMainWindow *m_window;
        WindowKeepActive m_windowKeepActive;
        KateViInputMode *m_viInputMode;
};

