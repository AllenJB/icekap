/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Base class for all chat panels
  begin:     Fri Feb 1 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <qvbox.h>
#include <qfile.h>

#include "identity.h"
#include "common.h"

/*
  @author Dario Abatianni
*/

class IRCView;
class IcecapServer;
class KonversationMainWindow;
class ViewContainer;

class ChatWindow : public QVBox
{
    Q_OBJECT

    public:
        explicit ChatWindow(QWidget* parent);
        ~ChatWindow();

        enum WindowType
        {
            Status=0,
            Channel,
            Query,
            DccChat,
            DccPanel,
            RawLog,
            Notice,
            SNotice,
            ChannelList,
            Konsole,
            UrlCatcher,
            NicksOnline,
            LogFileReader
        };

        /** This should be called and set with a non-null server as soon
         *  as possibly after ChatWindow is created.
         *  @param newServer The server to set it to.
         */
        virtual void setServer(IcecapServer* newServer);
        /** This should be called if setServer is not called - e.g.
         *  in the case of konsolepanel.  This should be set as soon
         *  as possible after creation.
         */

        /** Get the server this is linked to.
         *  @return The server it is associated with, or null if none.
         */
        IcecapServer* getServer();
        virtual void setIdentity(const Identity *newIdentity);
        void setTextView(IRCView* newView);
        IRCView* getTextView() const;
        void setLog(bool activate);

        QString getName();

        void setType(WindowType newType);
        WindowType getType();

        virtual void insertRememberLine();
        virtual void append(const QString& nickname,const QString& message);
        virtual void appendRaw(const QString& message, bool suppressTimestamps=false);
        virtual void appendQuery(const QString& nickname,const QString& message, bool usenotifications = false);
        virtual void appendAction(const QString& nickname,const QString& message, bool usenotifications = false);
        virtual void appendServerMessage(const QString& type,const QString& message, bool parseURL = true);
        virtual void appendCommandMessage(const QString& command, const QString& message, bool important = true,
            bool parseURL = true, bool self = false);
        virtual void appendBacklogMessage(const QString& firstColumn,const QString& message);

        QWidget* parentWidget;

        virtual QString getTextInLine();
        /** Clean up and close this tab.  Return false if you want to cancel the close. */
        virtual bool closeYourself();
        /** Reimplement this to return true in all classes that /can/ become front view.
         */
        virtual bool canBeFrontView();

        /** Reimplement this to return true in all classes that you can search in - i.e. use "Edit->Find Text" in.
         */
        virtual bool searchView();

        virtual bool notificationsEnabled() { return m_notificationsEnabled; }

        virtual bool eventFilter(QObject* watched, QEvent* e);

        QString logFileName() { return logfile.name(); }

        virtual void setChannelEncoding(const QString& /* encoding */) {}
        virtual QString getChannelEncoding() { return QString(); }
        virtual QString getChannelEncodingDefaultDesc() { return QString(); }
        bool isChannelEncodingSupported() const;

        /** Force updateInfo(info) to be emitted.
         *  Useful for when this tab has just gained focus
         */
        virtual void emitUpdateInfo();

        /** child classes have to override this and return true if they want the
         *  "insert character" item on the menu to be enabled.
         */
        virtual bool isInsertSupported() { return false; }

        /** child classes have to override this and return true if they want the
         *  "irc color" item on the menu to be enabled.
         */
        virtual bool areIRCColorsSupported() {return false; }

        Konversation::TabNotifyType currentTabNotification() { return m_currentTabNotify; }
        QColor highlightColor();

        signals:
        void nameChanged(ChatWindow* view, const QString& newName);
        void online(ChatWindow* myself, bool state);
        /** Emit this signal when you want to change the status bar text for this tab.
         *  It is ignored if this tab isn't focused.
         */
        void updateInfo(const QString &info);
        void updateTabNotification(ChatWindow* chatWin, const Konversation::TabNotifyType& type);

        void setStatusBarTempText(const QString&);
        void clearStatusBarTempText();

        void closing(ChatWindow* myself);

    public slots:
        void updateAppearance();

        void logText(const QString& text);

        /**
         * This is called when a chat window gains focus.
         * It enables and disables the appropriate menu items,
         * then calls childAdjustFocus.
         * You can call this manually to focus this tab.
         */
        void adjustFocus();

        virtual void appendInputText(const QString&);
        virtual void indicateAway(bool away);

        virtual void setNotificationsEnabled(bool enable) { m_notificationsEnabled = enable; }
        void resetTabNotification();

    protected slots:
        ///Used to disable functions when not connected
        virtual void serverOnline(bool online);

        ///Checks if we should update tab notification or not
        void activateTabNotification(Konversation::TabNotifyType type);

    protected:

        /** Some children may handle the name themselves, and not want this public.
         *  Increase the visibility in the subclass if you want outsiders to call this.
         *  The name is the string that is shown in the tab.
         *  @param newName The name to show in the tab
         */
        virtual void setName(const QString& newName);

        /** Called from adjustFocus */
        virtual void childAdjustFocus() = 0;

        void setLogfileName(const QString& name);
        void setChannelEncodingSupported(bool enabled);
        void cdIntoLogPath();

        int spacing();
        int margin();

        bool log;
        bool firstLog;
        QString name;
        QString logName;

        QFont font;

        IRCView* textView;
        /** A pointer to the server this chatwindow is part of.
         *  Not always non-null - e.g. for konsolepanel
         */
        IcecapServer* m_server;
        Identity identity;
        QFile logfile;
        WindowType type;

        bool m_notificationsEnabled;

        bool m_channelEncodingSupported;

        Konversation::TabNotifyType m_currentTabNotify;
};
#endif
