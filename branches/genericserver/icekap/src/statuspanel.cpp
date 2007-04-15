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

#include <qpushbutton.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qhbox.h>
#include <qtextcodec.h>
#include <qlineedit.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "channel.h"
#include "statuspanel.h"
#include "konversationapplication.h"
#include "ircinput.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "icecapserver.h"

StatusPanel::StatusPanel(QWidget* parent) : IcecapStatusPanel (parent)
{
//    IcecapStatusPanel (parent);

    setChannelEncodingSupported(true);

    awayChanged=false;
    awayState=false;

    QHBox* commandLineBox=new QHBox(this);
    commandLineBox->setSpacing(spacing());
    commandLineBox->setMargin(0);

    nicknameCombobox=new QComboBox(commandLineBox);
    nicknameCombobox->setEditable(true);
    nicknameCombobox->insertStringList(Preferences::nicknameList());
    oldNick=nicknameCombobox->currentText();

    awayLabel=new QLabel(i18n("(away)"),commandLineBox);
    awayLabel->hide();

    statusInput=new IRCInput(commandLineBox);

    connect(nicknameCombobox,SIGNAL (activated(int)),this,SLOT(nicknameComboboxChanged()));
    Q_ASSERT(nicknameCombobox->lineEdit());       //it should be editedable.  if we design it so it isn't, remove these lines.
    if(nicknameCombobox->lineEdit())
        connect(nicknameCombobox->lineEdit(), SIGNAL (lostFocus()),this,SLOT(nicknameComboboxChanged()));

    updateAppearanceExtra();
}

StatusPanel::~StatusPanel()
{
}

void StatusPanel::setNickname(const QString& newNickname)
{
    nicknameCombobox->setCurrentText(newNickname);
}

void StatusPanel::updateAppearanceExtra()
{
    if (Preferences::customTextFont())
    {
        nicknameCombobox->setFont(Preferences::textFont());
    }
    else
    {
        nicknameCombobox->setFont(KGlobalSettings::generalFont());
    }

    showNicknameBox(Preferences::showNicknameBox());

    ChatWindow::updateAppearance();
}

void StatusPanel::sendFileMenu()
{
    emit sendFile();
}

void StatusPanel::indicateAway(bool show)
{
    // QT does not redraw the label properly when they are not on screen
    // while getting hidden, so we remember the "soon to be" state here.
    if(isHidden())
    {
        awayChanged=true;
        awayState=show;
    }
    else
    {
        if(show)
            awayLabel->show();
        else
            awayLabel->hide();
    }
}

// fix Qt's broken behavior on hidden QListView pages
void StatusPanel::showEvent(QShowEvent*)
{
    if(awayChanged)
    {
        awayChanged=false;
        indicateAway(awayState);
    }
}

void StatusPanel::nicknameComboboxChanged()
{
    QString newNick=nicknameCombobox->currentText();
//    oldNick=m_server->getNickname();
    if(oldNick!=newNick)
    {
      nicknameCombobox->setCurrentText(oldNick);
      m_server->queue("NICK "+newNick);
    }
    // return focus to input line
    statusInput->setFocus();
}

void StatusPanel::changeNickname(const QString& newNickname)
{
    m_server->queue("NICK "+newNickname);
}

                                                  // virtual
void StatusPanel::setChannelEncoding(const QString& encoding)
{
//    Preferences::setChannelEncoding(m_server->getServerGroup(), ":server", encoding);
}

QString StatusPanel::getChannelEncoding()         // virtual
{
//    return Preferences::channelEncoding(m_server->getServerGroup(), ":server");
    return "utf8";
}

                                                  // virtual
QString StatusPanel::getChannelEncodingDefaultDesc()
{
//    return i18n("Identity Default ( %1 )").arg(getServer()->getIdentity()->getCodecName());
    return "Unimplemented (utf8)";
}

//Used to disable functions when not connected
void StatusPanel::serverOnline(bool online)
{
    //statusInput->setEnabled(online);
    getTextView()->setNickAndChannelContextMenusEnabled(online);
    nicknameCombobox->setEnabled(online);
}

void StatusPanel::showNicknameBox(bool show)
{
    if(show)
    {
        nicknameCombobox->show();
    }
    else
    {
        nicknameCombobox->hide();
    }
}

void StatusPanel::setIdentity(const Identity *newIdentity)
{
    if(!newIdentity)
    {
        return;
    }

    ChatWindow::setIdentity(newIdentity);
    nicknameCombobox->clear();
    nicknameCombobox->insertStringList(newIdentity->getNicknameList());
}

#include "statuspanel.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
