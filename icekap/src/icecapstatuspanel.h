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

#ifndef ICECAPSTATUSPANEL_H
#define ICECAPSTATUSPANEL_H

#include "chatwindow.h"

#include <qstring.h>

/*
  @author Dario Abatianni
*/

class QPushButton;
class QCheckBox;
class QLabel;
class QComboBox;

class IRCInput;

class IcecapStatusPanel : public ChatWindow
{
    Q_OBJECT

    public:
//        explicit IcecapStatusPanel(QWidget* parent);
		explicit IcecapStatusPanel(QWidget* parent, bool p_isPresenceStatus = false);
        ~IcecapStatusPanel();

        virtual void setName(const QString& newName);

        virtual QString getTextInLine();
        virtual bool closeYourself();
        virtual bool canBeFrontView();
        virtual bool searchView();

        virtual void emitUpdateInfo();

        virtual bool isInsertSupported() { return true; }

        virtual void setNotificationsEnabled(bool enable);

    signals:
        void sendFile();

    public slots:
        void updateAppearance();
        virtual void appendInputText(const QString&);
        void updateName();

    protected slots:
        void statusTextEntered();
        void sendStatusText(const QString& line);
        // connected to IRCInput::textPasted() - used for large/multiline pastes
        void textPasted(const QString& text);

    protected:

        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus();

        QLabel* awayLabel;
        IRCInput* statusInput;
        QCheckBox* logCheckBox;

		bool isPresenceStatus;
};
#endif
