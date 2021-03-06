/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
    channel.h  -  The class that controls a channel
    begin:     Wed Jan 23 2002
    copyright: (C) 2002 by Dario Abatianni
               (C) 2004 by Peter Simonsson <psn@linux.se>
    email:     eisfuchs@tigress.com
*/

#ifndef CHANNEL_H
#define CHANNEL_H

#include <qtimer.h>
#include <qstring.h>

#include "icecapserver.h"
#include "chatwindow.h"

/*
  @author Dario Abatianni
*/

class QPushButton;
class QCheckBox;
class QLabel;
class QTimer;
class QListViewItem;
class QHBox;
class QStringList;
class QSplitter;
class QGrid;
class QComboBox;
class QDropEvent;
class QToolButton;

class KLineEdit;

class NickListView;
class QuickButton;
class ModeButton;
class IRCInput;
class NickChangeDialog;

namespace Konversation
{
    class TopicLabel;
    class ChannelOptionsDialog;
}

namespace Icecap
{
    class Channel;
    class ChannelPresence;
}

class ChannelWindow : public ChatWindow
{
    Q_OBJECT

    public:
        ChannelWindow(QWidget* parent, Icecap::Channel* channel);
        ~ChannelWindow();
//META
        virtual bool canBeFrontView();
        virtual bool searchView();

        void serverOnline(bool online);

//General administrative stuff
    public:
        void setName(const QString& newName);
        void setKey(const QString& newKey);
        QString getKey();

        virtual void setMyPresence (Icecap::MyPresence* p_mypresence);
        virtual void setServer(IcecapServer* newServer);

    public slots:
        void setNickname(const QString& newNickname);
        void userListUpdated ();
        void nicknameChanged ();

//Nicklist
    public:
        virtual void emitUpdateInfo();

//Topic
    public:
        /** Get the current channel topic.
         *
         * The topic may or may not have the author that set it at the start of the string,
         * like:  "<author> topic"
         *
         * The internal variable topicAuthorUnknown stores whether the "<author>" bit is there or not.
         *
         * */
        QString getTopic();
        /** Get the channel topic history sorted in reverse chronological order.
         *
         * Each topic may or may not have the author that set it at the start of the string,
         * like:  "<author> topic"
         *
         * @return a list of topics this channel used to have, current at the top.
         */
        QStringList getTopicHistory();

        void setTopic(const QString& topic);
        void setTopic(const QString& nickname, const QString& topic);
        void setTopicAuthor(const QString& author);

    signals:
        void topicHistoryChanged();


//Modes
//TODO: the only representation of the channel limit is held in the GUI

    public:
        /// Internal - Empty the modelist
        void clearModeList();
        /// Get the list of modes that this channel has - e.g. {+l,+s,-m}
        //TODO: does this method return a list of all modes, all modes that have been changed, or all modes that are +?
        QStringList getModeList() const { return m_modeList; }

        /** Outputs a message on the channel, and modifies the mode for a ChannelNick.
         *  @param sourceNick The server or the nick of the person that made the mode change.
         *  @param mode The mode that is changing one of v,h,o,a for voice halfop op admin
         *  @param plus True if the mode is being granted, false if it's being taken away.
         *  @param parameter This depends on what the mode change is.  In most cases it is the nickname of the person that is being given voice/op/admin etc.  See the code.
         */
        void updateMode(const QString& sourceNick, char mode, bool plus, const QString &parameter);

    signals:
        void modesChanged();

//Generic GUI
    public:
        virtual bool eventFilter(QObject* watched, QEvent* e);

//Specific GUI
    public:
        void updateModeWidgets(char mode, bool plus, const QString &parameter);
        void updateQuickButtons(const QStringList &newButtonList);

        /// Thunks to ircview->updateStyleSheet
        void updateStyleSheet();

        /// Get the contents of the input line.
        virtual QString getTextInLine();
        /// Sounds suspiciously like a destructor..
        virtual bool closeYourself();

        ///TODO: kill this, it has been reimplemented at the ChatWindow level
        bool allowNotifications() { return m_allowNotifications; }

// TODO: Re-implement to use Icecap::Channel
// Used by NickListView::updateActions
//        ChannelNickList getSelectedChannelNicks();
        bool getChannelCommand () { return channelCommand; }

        NickListView* getNickListView() const { return nicknameListView; }

    signals:
        void sendFile();

    public slots:
        void updateAppearance();
        void channelTextEntered();
        void channelPassthroughCommand();
        void sendChannelText(const QString& line);
        void showOptionsDialog();
        void showQuickButtons(bool show);
        void showModeButtons(bool show);

        void appendInputText(const QString& s);
        virtual void indicateAway(bool show);
        void showTopic(bool show);
        void showNicknameBox(bool show);
        void showNicknameList(bool show);

        void setAllowNotifications(bool allow) { m_allowNotifications = allow; }

    protected slots:
        void completeNick(); ///< I guess this is a GUI function, might be nice to have at DCOP level though --argonel
        void endCompleteNick();
        void quickButtonClicked(const QString& definition);
        void modeButtonClicked(int id,bool on);
        void channelLimitChanged();

        void popupChannelCommand(int id);         ///< Connected to IRCView::popupCommand()
        void popupCommand(int id);                ///< Connected to NickListView::popupCommand()
        void doubleClickCommand(QListViewItem*);  ///< Connected to NickListView::doubleClicked()
        // Dialogs
//        void changeNickname(const QString& newNickname);

        void textPasted(const QString& text); ///< connected to IRCInput::textPasted() - used to handle large/multiline pastings

        void sendFileMenu(); ///< connected to IRCInput::sendFile()
        void nicknameComboboxChanged();
        /// Enable/disable the mode buttons depending on whether you are op or not.
        void refreshModeButtons();

//only the GUI cares about sorted nicklists
        ///Request a delayed nicklist sorting
        void requestNickListSort();
        ///Sort the nicklist
        void sortNickList();

    protected:
        void showEvent(QShowEvent* event);
        void syncSplitters();
        /// Called from ChatWindow adjustFocus
        virtual void childAdjustFocus();
        /// Close the channel then come back in
        void cycleChannel(); ///< TODO this is definately implemented and hooked incorrectly.

        bool channelCommand;///< True if nick context menu is executed from IRCView

        // to take care of redraw problem if hidden
        bool quickButtonsChanged;
        bool quickButtonsState;
        bool modeButtonsChanged;
        bool modeButtonsState;
        bool awayChanged;
        bool awayState;
        bool splittersInitialized;
        bool topicSplitterHidden;
        bool channelSplitterHidden;

        unsigned int completionPosition;

        QSplitter* m_horizSplitter;
        QSplitter* m_vertSplitter;
        QWidget* topicWidget;
        QToolButton* m_topicButton;
        Konversation::TopicLabel* topicLine;

        //TODO: abstract these
        QHBox* modeBox;
        ModeButton* modeT;
        ModeButton* modeN;
        ModeButton* modeS;
        ModeButton* modeI;
        ModeButton* modeP;
        ModeButton* modeM;
        ModeButton* modeK;
        ModeButton* modeL;

        KLineEdit* limit; //TODO: this GUI element is the only storage for the mode

        NickListView* nicknameListView;
        QColor abgCache; ///< Caches the alternate background color for the nicklist
        QHBox* commandLineBox;
        QVBox* nickListButtons;
        QGrid* buttonsGrid;
        QComboBox* nicknameCombobox;
        QString oldNick; ///< GUI
        QLabel* awayLabel;
        IRCInput* channelInput;

        NickChangeDialog* nickChangeDialog;
        QPtrList<QuickButton> buttonList;

//Members from here to end are not GUI
        QStringList m_topicHistory;
        bool topicAuthorUnknown; ///< Stores whether the "<author>" bit is there or not.

        QString key;

        uint m_currentIndex;

        QTimer* m_processingTimer;
        QTimer* m_delayedSortTimer;

        QStringList m_modeList;

        bool m_allowNotifications; ///<TODO: remove this, its been implemented on the chatwindow object

        Konversation::ChannelOptionsDialog *m_optionsDialog;
};
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
