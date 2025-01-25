/*
    SPDX-FileCopyrightText: 2012-2013 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KTEXTEDITOR_MESSAGE_H
#define KTEXTEDITOR_MESSAGE_H

#include <QObject>

#include <ktexteditor_export.h>

class QIcon;
class QAction;

namespace KTextEditor
{
class View;
class Document;

/*!
 * \class KTextEditor::Message
 * \inmodule KTextEditor
 * \inheaderfile KTextEditor/Message
 *
 * \brief This class holds a Message to display in Views.
 *
 * \target message_intro
 * \section1 Introduction
 *
 * The Message class holds the data used to display interactive message widgets
 * in the editor. Use the Document::postMessage() to post a message as follows:
 *
 * \code
 * // always use a QPointer to guard your Message, if you keep a pointer
 * // after calling postMessage()
 * QPointer<KTextEditor::Message> message =
 *     new KTextEditor::Message("text", KTextEditor::Message::Information);
 * message->setWordWrap(true);
 * message->addAction(...); // add your actions...
 * document->postMessage(message);
 * \endcode
 *
 * A Message is deleted automatically if the Message gets closed, meaning that
 * you usually can forget the pointer. If you really need to delete a message
 * before the user processed it, always guard it with a QPointer!
 *
 * \target message_creation
 * \section1 Message Creation and Deletion
 *
 * To create a new Message, use code like this:
 * \code
 * QPointer<KTextEditor::Message> message =
 *     new KTextEditor::Message("My information text", KTextEditor::Message::Information);
 * message->setWordWrap(true);
 * // ...
 * \endcode
 *
 * Although discouraged in general, the text of the Message can be changed
 * on the fly when it is already visible with setText().
 *
 * Once you posted the Message through Document::postMessage(), the
 * lifetime depends on the user interaction. The Message gets automatically
 * deleted either if the user clicks a closing action in the message, or for
 * instance if the document is reloaded.
 *
 * If you posted a message but want to remove it yourself again, just delete
 * the message. But beware of the following warning!
 *
 * \warning Always use QPointer\<Message\> to guard the message pointer from
 *          getting invalid, if you need to access the Message after you posted
 *          it.
 *
 * \target message_positioning
 * \section1 Positioning
 *
 * By default, the Message appears right above of the View. However, if desired,
 * the position can be changed through setPosition(). For instance, the
 * search-and-replace code in Kate Part shows the number of performed replacements
 * in a message floating in the view. For further information, have a look at
 * the enum MessagePosition.
 *
 * \target message_hiding
 * \section1 Autohiding Messages
 *
 * Message%s can be shown for only a short amount of time by using the autohide
 * feature. With setAutoHide() a timeout in milliseconds can be set after which
 * the Message is automatically hidden. Further, use setAutoHideMode() to either
 * trigger the autohide timer as soon as the widget is shown (AutoHideMode::Immediate),
 * or only after user interaction with the view (AutoHideMode::AfterUserInteraction).
 *
 * The default autohide mode is set to AutoHideMode::AfterUserInteraction.
 * This way, it is unlikely the user misses a notification.
 *
 * \since 4.11
 */
class KTEXTEDITOR_EXPORT Message : public QObject
{
    Q_OBJECT

    //
    // public data types
    //
public:
    /*!
       \enum KTextEditor::Message::MessageType

       Message types used as visual indicator.

       The message types match exactly the behavior of KMessageWidget::MessageType.
       For simple notifications either use Positive or Information.

       \value Positive
       Positive information message
       \value Information
       Information message type
       \value Warning
       Warning message type
       \value Error
       Error message type
     */
    enum MessageType {
        Positive = 0,
        Information,
        Warning,
        Error
    };

    /*!
       \enum KTextEditor::Message::MessagePosition

       Message position used to place the message either above or below of the
       KTextEditor::View.

       \value AboveView
       Show message above view.
       \value BelowView
       Show message below view.
       \value TopInView
       Show message as view overlay in the top right corner.
       \value BottomInView
       Show message as view overlay in the bottom right corner.
       \value CenterInView
       Show message as view overlay in the center of the view. \since 5.42
     */
    enum MessagePosition {
        AboveView = 0,
        BelowView,
        TopInView,
        BottomInView,
        CenterInView
    };

    /*!
       \enum KTextEditor::Message::AutoHideMode

       \brief The AutoHideMode determines when to trigger the autoHide timer.

       \sa setAutoHide(), autoHide()

       \value Immediate
       Auto-hide is triggered as soon as the message is shown
       \value AfterUserInteraction
       Auto-hide is triggered only after the user interacted with the view
     */
    enum AutoHideMode {
        Immediate = 0,
        AfterUserInteraction
    };

public:
    /*!
     * Constructor for new messages.
     *
     * \a type is the message type, e.g. MessageType::Information
     *
     * \a richtext is the text to be displayed
     *
     */
    Message(const QString &richtext, MessageType type = Message::Information);

    /*!
     * Destructor.
     */
    ~Message() override;

    /*!
     * Returns the text set in the constructor.
     */
    QString text() const;

    /*!
     * Returns the icon of this message.
     * If the message has no icon set, a null icon is returned.
     * \sa setIcon()
     */
    QIcon icon() const;

    /*!
     * Returns the message type set in the constructor.
     */
    MessageType messageType() const;

    /*!
     * Adds an action to the message.
     *
     * By default (\a closeOnTrigger = true), the action closes the message
     * displayed in all Views. If \a closeOnTrigger is false, the message
     * is stays open.
     *
     * The actions will be displayed in the order you added the actions.
     *
     * To connect to an action, use the following code:
     * \code
     * connect(action, &QAction::triggered, receiver, &ReceiverType::slotActionTriggered);
     * \endcode
     *
     * \a action is the action to be added
     *
     * \a closeOnTrigger when triggered, the message widget is closed
     *
     * \warning The added actions are deleted automatically.
     *          So do \b not delete the added actions yourself.
     */
    void addAction(QAction *action, bool closeOnTrigger = true);

    /*!
     * Accessor to all actions, mainly used in the internal implementation
     * to add the actions into the gui.
     * \sa addAction()
     */
    QList<QAction *> actions() const;

    /*!
     * Set the auto hide time to \a delay milliseconds.
     * If \a delay < 0, auto hide is disabled.
     * If \a delay = 0, auto hide is enabled and set to a sane default
     * value of several seconds.
     *
     * By default, auto hide is disabled.
     *
     * \sa autoHide(), setAutoHideMode()
     */
    void setAutoHide(int delay = 0);

    /*!
     * Returns the auto hide time in milliseconds.
     * Please refer to setAutoHide() for an explanation of the return value.
     *
     * \sa setAutoHide(), autoHideMode()
     */
    int autoHide() const;

    /*!
     * Sets the auto hide mode to \a mode.
     * The default mode is set to AutoHideMode::AfterUserInteraction.
     *
     * \a mode is the auto hide mode
     *
     * \sa autoHideMode(), setAutoHide()
     */
    void setAutoHideMode(KTextEditor::Message::AutoHideMode mode);

    /*!
     * Get the Message's auto hide mode.
     * The default mode is set to AutoHideMode::AfterUserInteraction.
     * \sa setAutoHideMode(), autoHide()
     */
    KTextEditor::Message::AutoHideMode autoHideMode() const;

    /*!
     * Enabled word wrap according to \a wordWrap.
     * By default, auto wrap is disabled.
     *
     * Word wrap is enabled automatically, if the Message's width is larger than
     * the parent widget's width to avoid breaking the gui layout.
     *
     * \sa wordWrap()
     */
    void setWordWrap(bool wordWrap);

    /*!
     * Returns true if word wrap is enabled.
     *
     * \sa setWordWrap()
     */
    bool wordWrap() const;

    /*!
     * Set the priority of this message to \a priority.
     * Messages with higher priority are shown first.
     * The default priority is 0.
     *
     * \sa priority()
     */
    void setPriority(int priority);

    /*!
     * Returns the priority of the message.
     *
     * \sa setPriority()
     */
    int priority() const;

    /*!
     * Set the associated view of the message.
     * If \a view is 0, the message is shown in all View%s of the Document.
     * If \a view is given, i.e. non-zero, the message is shown only in this view.
     * \a view the associated view the message should be displayed in
     */
    void setView(KTextEditor::View *view);

    /*!
     * This function returns the view you set by setView(). If setView() was
     * not called, the return value is 0.
     */
    KTextEditor::View *view() const;

    /*!
     * Set the document pointer to \a document.
     * This is called by the implementation, as soon as you post a message
     * through Document::postMessage(), so that you do not have to
     * call this yourself.
     * \sa document()
     */
    void setDocument(KTextEditor::Document *document);

    /*!
     * Returns the document pointer this message was posted in.
     * This pointer is 0 as long as the message was not posted.
     */
    KTextEditor::Document *document() const;

    /*!
     * Sets the position of the message to \a position.
     * By default, the position is set to MessagePosition::AboveView.
     * \sa MessagePosition
     */
    void setPosition(MessagePosition position);

    /*!
     * Returns the message position of this message.
     * \sa setPosition(), MessagePosition
     */
    MessagePosition position() const;

public Q_SLOTS:
    /*!
     * Sets the notification contents to \a richtext.
     * If the message was already sent through Document::postMessage(),
     * the displayed text changes on the fly.
     * \note Change text on the fly with care, since changing the text may
     *       resize the notification widget, which may result in a distracting
     *       user experience.
     *
     * \a richtext is the new notification text (rich text supported)
     *
     * \sa textChanged()
     */
    void setText(const QString &richtext);

    /*!
     * Add an optional \a icon for this notification which will be shown next to
     * the message text. If the message was already sent through Document::postMessage(),
     * the displayed icon changes on the fly.
     *
     * \note Change the icon on the fly with care, since changing the text may
     *       resize the notification widget, which may result in a distracting
     *       user experience.
     *
     * \a icon is the icon to be displayed
     *
     * \sa iconChanged()
     */
    void setIcon(const QIcon &icon);

Q_SIGNALS:
    /*!
     * This signal is emitted before the \a message is deleted. Afterwards, this
     * pointer is invalid.
     *
     * Use the function document() to access the associated Document.
     *
     * \a message the closed/processed message
     */
    void closed(KTextEditor::Message *message);

    /*!
     * This signal is emitted whenever setText() was called.
     *
     * \a text the new notification text (rich text supported)
     *
     * \sa setText()
     */
    void textChanged(const QString &text);

    /*!
     * This signal is emitted whenever setIcon() was called.
     *
     * \a icon is the new notification icon
     *
     * \sa setIcon()
     */
    void iconChanged(const QIcon &icon);

private:
    class MessagePrivate *const d;
};

}

#endif
