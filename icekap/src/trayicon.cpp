/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  This class handles the system tray icon
  begin:     Sun Nov 9 2003
  copyright: (C) 2003 by Peter Simonsson
  email:     psn@linux.se
*/

#include "trayicon.h"
#include "konversationapplication.h"

#include <qtimer.h>
#include <qtooltip.h>

#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>

// #include "channel.h"
#include "icecapserver.h"
#include "chatwindow.h"

namespace Konversation
{

    TrayIcon::TrayIcon(QWidget* parent) : KSystemTray(parent)
    {
        m_notificationEnabled = false;
        m_nomessagePix = loadIcon("konversation");
        m_messagePix = loadIcon("konv_message");
        setPixmap(m_nomessagePix);
        m_blinkTimer = new QTimer(this);
        connect(m_blinkTimer, SIGNAL(timeout()), SLOT(blinkTimeout()));

        QToolTip::add(this,i18n("Icekap - Icecap Client"));
    }

    TrayIcon::~TrayIcon()
    {
    }

    void TrayIcon::startNotification()
    {
        if(!m_notificationEnabled)
        {
            return;
        }

        if(!m_blinkTimer->isActive())
        {
            setPixmap(m_messagePix);
            m_blinkOn = true;
            m_blinkTimer->start(500);
        }
    }

    void TrayIcon::endNotification()
    {
        if(m_blinkTimer->isActive())
        {
            m_blinkTimer->stop();
            setPixmap(m_nomessagePix);
        }
    }

    void TrayIcon::blinkTimeout()
    {
        m_blinkOn = !m_blinkOn;

        if(m_blinkOn)
        {
            setPixmap(m_messagePix);
        }
        else
        {
            setPixmap(m_nomessagePix);
        }
    }
}

#include "trayicon.moc"
