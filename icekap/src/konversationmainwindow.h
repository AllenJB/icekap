/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
  Copyright (C) 2005 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2005 Eike Hein <sho@eikehein.com>
*/

#ifndef _KONVERSATIONMAINWINDOW_H_
#define _KONVERSATIONMAINWINDOW_H_

#include <qstringlist.h>

#include <kmainwindow.h>
#include <kaction.h>

#include "preferences.h"
#include "ssllabel.h"
#include "icecapserver.h"

class KToggleAction;
class KScriptManager;

class KonviBookmarkHandler;
class Ignore;
class NicksOnline;
class QuickButtonsDialog;
class KonviSettingsDialog;
class ViewContainer;
class KonversationStatusBar;

namespace Konversation
{
    class ServerListDialog;
    class TrayIcon;
}

class KonversationMainWindow : public KMainWindow
{
    Q_OBJECT

    public:
        KonversationMainWindow();
        ~KonversationMainWindow();

        ViewContainer* getViewContainer() { return m_viewContainer; }
        Konversation::TrayIcon* systemTrayIcon() const { return m_trayIcon; }

        /** Some errors need to be shown, even when konversation is minimized.
         *  For example, when a kimiface call is received to query a person,
         *  (e.g. the user choses "Chat with X" in kmail) but that person isn't
         *  recognised, we need to give immediate feedback to the user.
         */
        void focusAndShowErrorMessage(const QString &errorMsg);

        QString currentURL(bool passNetwork);
        QString currentTitle();

    signals:
        void prefsChanged();
        void startNotifyTimer(int msec);
        void showQuickConnectDialog();
        void showIcecapQuickConnectDialog();
        void nicksNowOnline(IcecapServer*);
        void endNotification();
        void quitServer();
        void serverStateChanged(IcecapServer* server, IcecapServer::State state);
        void insertRememberLine();
        void insertRememberLine(IcecapServer* server);

    public slots:
        void updateTrayIcon();

        void openServerList();

        void openIdentitiesDialog();
        IdentityPtr editIdentity(IdentityPtr identity);

        void setOnlineList(IcecapServer* notifyServer,const QStringList& list, bool changed);

    protected slots:
        /** This is connected to the preferences settingsChanged signal and acts to compress
        *  multiple successively settingsChanged() signals into a single output
        *  appearanceChanged() signal.
        *
        *  Do not connect to the settingsChanged signal elsewhere.  If you want to know when
        *  the settings have changed, connect to:
        *  KonversationApplication::instance(), SIGNAL(appearanceChanged())
        */
        void settingsChangedSlot();

        /** This is connected to the appearanceChanged signal.
        *  @see settingsChangedSlot()
        */
        void resetHasDirtySettings();

        void toggleMenubar(bool dontShowWarning = false);

        int configureToolbar();
        void saveToolbarConfig();

        void openPrefsDialog();
        void openKeyBindings();
        void openQuickConnectDialog();
        void openIcecapQuickConnectDialog();

        void openNotify();
        // it seems that moc does not honor #ifs in compile so we create an
        // empty slot in our .cpp file rather than #if this slot out
        void openNotifications();
        void notifyAction(const QString& serverName,const QString& nick);

        void quitProgram();
        void hideEvent(QHideEvent* e);
        void focusOutEvent(QFocusEvent* e);
        void leaveEvent(QEvent* e);

        void openURL(const QString& url, const QString& title);

    protected:
        int confirmQuit();
        bool queryClose();
        virtual bool event(QEvent* e);

        ViewContainer* m_viewContainer;
        KonversationStatusBar* m_statusBar;
        Konversation::TrayIcon* m_trayIcon;

        KToggleAction* hideMenuBarAction;

        KPopupMenu* m_bookmarks;
        KonviBookmarkHandler* m_bookmarkHandler;
        KonviSettingsDialog *m_settingsDialog;
        Konversation::ServerListDialog* m_serverListDialog;

        /** @see settingsChangedSlot() */
        bool m_hasDirtySettings;
        bool m_closeApp;
};

#endif
