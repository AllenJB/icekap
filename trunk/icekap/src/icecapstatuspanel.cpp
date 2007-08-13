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

#include "icecapstatuspanel.h"
#include "konversationapplication.h"
#include "ircinput.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "icecapserver.h"

IcecapStatusPanel::IcecapStatusPanel(QWidget* parent, bool p_isPresenceStatus) : ChatWindow(parent)
{
    isPresenceStatus = p_isPresenceStatus;

    setType(ChatWindow::IcecapStatus);

    // set up text view, will automatically take care of logging
    IRCViewBox* ircBox = new IRCViewBox(this, 0); // Server will be set later in setServer()
    setTextView(ircBox->ircView());

    if (! isPresenceStatus) {
        QHBox* commandLineBox=new QHBox(this);
        commandLineBox->setSpacing(spacing());
        commandLineBox->setMargin(0);

        statusInput=new IRCInput(commandLineBox);

        getTextView()->installEventFilter(statusInput);
        statusInput->installEventFilter(this);
    }

    setLog(Preferences::log());


    connect(getTextView(), SIGNAL(updateTabNotification(Konversation::TabNotifyType)),
        this, SLOT(activateTabNotification(Konversation::TabNotifyType)));
//    connect(getTextView(),SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
    if (! isPresenceStatus) {
        connect(getTextView(),SIGNAL (autoText(const QString&)),this,SLOT (sendStatusText(const QString&)) );
        connect(getTextView(),SIGNAL (gotFocus()),statusInput,SLOT (setFocus()) );

        connect(statusInput,SIGNAL (submit()),this,SLOT(statusTextEntered()) );
        connect(statusInput,SIGNAL (textPasted(const QString&)),this,SLOT(textPasted(const QString&)) );
        connect(getTextView(), SIGNAL(textPasted(bool)), statusInput, SLOT(paste(bool)));

        updateAppearance();
    }
}

IcecapStatusPanel::~IcecapStatusPanel()
{
}

void IcecapStatusPanel::childAdjustFocus()
{
    statusInput->setFocus();
}

void IcecapStatusPanel::sendStatusText(const QString& sendLine)
{
    // create a work copy
    QString outputAll(sendLine);
    // replace aliases and wildcards
/*
    TODO: Fix this so we don't need a nickname (because this status panel is used for the Icecap server, where no nickname exists)
    if(m_server->getOutputFilter()->replaceAliases(outputAll))
    {
        outputAll = m_server->parseWildcards(outputAll, m_server->getNickname(), QString::null, QString::null, QString::null, QString::null);
    }
*/

    // Send all strings, one after another
    QStringList outList=QStringList::split('\n',outputAll);
    for(unsigned int index=0;index<outList.count();index++)
    {
        QString output(outList[index]);

        // encoding stuff is done in Server()
        Icecap::OutputFilterResult result = m_server->getOutputFilter()->parse("", output, QString::null);

        if(!result.output.isEmpty())
        {
            appendServerMessage(result.typeString, result.output);
        }
        else if (!result.outputList.isEmpty ())
        {
            QStringList::const_iterator end = result.outputList.end();
            for ( QStringList::ConstIterator it = result.outputList.begin(); it != end; ++it ) {
                appendServerMessage(result.typeString, *it);
            }
        }
        m_server->queue(result.toServer);
    } // for
}

void IcecapStatusPanel::statusTextEntered()
{
    QString line=statusInput->text();
    statusInput->clear();

    if(line.lower()==Preferences::commandChar()+"clear") textView->clear();
    else
    {
        if(line.length()) sendStatusText(line);
    }
}

void IcecapStatusPanel::textPasted(const QString& text)
{
    if(m_server)
    {
        QStringList multiline=QStringList::split('\n',text);
        for(unsigned int index=0;index<multiline.count();index++)
        {
            QString line=multiline[index];
            QString cChar(Preferences::commandChar());
            // make sure that lines starting with command char get escaped
            if(line.startsWith(cChar)) line=cChar+line;
            sendStatusText(line);
        }
    }
}

void IcecapStatusPanel::updateAppearance()
{
    QColor fg;
    QColor bg;
    if(Preferences::inputFieldsBackgroundColor())
    {
        fg=Preferences::color(Preferences::ChannelMessage);
        bg=Preferences::color(Preferences::TextViewBackground);
    }
    else
    {
        fg=colorGroup().foreground();
        bg=colorGroup().base();
    }

    statusInput->unsetPalette();
    statusInput->setPaletteForegroundColor(fg);
    statusInput->setPaletteBackgroundColor(bg);

    getTextView()->unsetPalette();

    if(Preferences::showBackgroundImage())
    {
        getTextView()->setViewBackground(Preferences::color(Preferences::TextViewBackground),
            Preferences::backgroundImage());
    }
    else
    {
        getTextView()->setViewBackground(Preferences::color(Preferences::TextViewBackground),
            QString::null);
    }

    if (Preferences::customTextFont())
    {
        getTextView()->setFont(Preferences::textFont());
        statusInput->setFont(Preferences::textFont());
    }
    else
    {
        getTextView()->setFont(KGlobalSettings::generalFont());
        statusInput->setFont(KGlobalSettings::generalFont());
    }

    ChatWindow::updateAppearance();
}

void IcecapStatusPanel::setName(const QString& newName)
{
    ChatWindow::setName(newName);
    setLogfileName(newName.lower());
}

void IcecapStatusPanel::updateName()
{
    QString newName = getServer()->name();
    setName (newName);
    setLogfileName (newName.lower());
}

QString IcecapStatusPanel::getTextInLine() { return statusInput->text(); }

bool IcecapStatusPanel::canBeFrontView()        { return true; }
bool IcecapStatusPanel::searchView()       { return true; }

// TODO: What is NotificationsEnabled? Flash on activity? or IRC nickname notifications?
void IcecapStatusPanel::setNotificationsEnabled(bool enable)
{
//    m_server->serverGroupSettings()->setNotificationsEnabled(enable);
    m_notificationsEnabled = enable;
}

bool IcecapStatusPanel::closeYourself()
{
    int result;

    //FIXME: Show "Do you really want to close ..." warnings in
    // disconnected state instead of closing directly. Can't do
    // that due to string freeze at the moment.
    if (!m_mypresence->connected())
    {
        result = KMessageBox::Continue;
    }
    else
    {
        result = KMessageBox::warningContinueCancel(
            this,
            i18n("Do you want to disconnect from '%1'?").arg(m_server->getServerName()),
            i18n("Disconnect From Server"),
            i18n("Disconnect"),
            "QuitServerTab");
    }

    if(result==KMessageBox::Continue)
    {
//        m_server->serverGroupSettings()->setNotificationsEnabled(notificationsEnabled());
        m_server->quitServer();
        //Why are these separate?  why would deleting the server not quit it? FIXME
        delete m_server;
        m_server=0;
        deleteLater();                            //NO NO!  Deleting the server should delete this! FIXME
        return true;
    }
    return false;
}

void IcecapStatusPanel::emitUpdateInfo()
{
    QString info = m_server->name();
    emit updateInfo(info);
}

void IcecapStatusPanel::appendInputText(const QString& text)
{
    statusInput->setText(statusInput->text() + text);
}

#include "icecapstatuspanel.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
