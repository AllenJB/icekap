/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The panel where the server status messages go
  begin:     Sam Jan 18 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef STATUSPANEL_H
#define STATUSPANEL_H

#include "chatwindow.h"
#include "icecapstatuspanel.h"

#include <qstring.h>

/*
  @author Dario Abatianni
*/

class QPushButton;
class QCheckBox;
class QLabel;
class QComboBox;

class IRCInput;
class NickChangeDialog;

class StatusPanel : public IcecapStatusPanel
{
    Q_OBJECT

    public:
        explicit StatusPanel(QWidget* parent);
        ~StatusPanel();

        virtual void setChannelEncoding(const QString& encoding);
        virtual QString getChannelEncoding();
        virtual QString getChannelEncodingDefaultDesc();

        virtual void setIdentity(const Identity *newIdentity);

    signals:
        void sendFile();

    public slots:
        void setNickname(const QString& newNickname);
        virtual void indicateAway(bool show);
        void showNicknameBox(bool show);
        void updateAppearanceExtra();

    protected slots:
        void sendFileMenu();
        void changeNickname(const QString& newNickname);
        void nicknameComboboxChanged();
        //Used to disable functions when not connected
        virtual void serverOnline(bool online);

    protected:
        bool awayChanged;
        bool awayState;

        void showEvent(QShowEvent* event);

        QComboBox* nicknameCombobox;
        QString oldNick;

        StatusPanel* statusPanel;
};
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
