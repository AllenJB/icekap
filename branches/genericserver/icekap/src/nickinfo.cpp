/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Nick Information
  begin:     Sat Jan 17 2004
  copyright: (C) 2004 by Gary Cramblitt
  email:     garycramblitt@comcast.net
*/

#include "nickinfo.h"

#include <qtooltip.h>
#include "icecapserver.h"
#include <klocale.h>

#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "viewcontainer.h"

/*
  @author Gary Cramblitt
*/

/*
  The NickInfo object is a data container for information about a single nickname.
  It is owned by the Server object and should NOT be deleted by anything other than Server.
  Store a pointer to this with NickInfoPtr
*/

NickInfo::NickInfo(const QString& nick, IcecapServer* server): KShared()
{
    m_nickname = nick;
    m_loweredNickname = nick.lower();
    m_owningServer = server;
    m_away = false;
    m_notified = false;
    m_identified = false;
    m_printedOnline = true;

    m_changedTimer = new QTimer( this);
    connect(m_changedTimer, SIGNAL( timeout()), SLOT(emitNickInfoChanged()));

    // reset nick color
    m_nickColor = 0;
}

NickInfo::~NickInfo()
{
}

// Get properties of NickInfo object.
QString NickInfo::getNickname() const { return m_nickname; }
QString NickInfo::loweredNickname() const { return m_loweredNickname; }
QString NickInfo::getHostmask() const { return m_hostmask; }

bool NickInfo::isAway() const { return m_away; }
QString NickInfo::getAwayMessage() const { return m_awayMessage; }
QString NickInfo::getIdentdInfo() const { return m_identdInfo; }
QString NickInfo::getVersionInfo() const { return m_versionInfo; }
bool NickInfo::isNotified() const { return m_notified; }
QString NickInfo::getRealName() const { return m_realName; }
QString NickInfo::getNetServer() const { return m_netServer; }
QString NickInfo::getNetServerInfo() const { return m_netServerInfo; }
QDateTime NickInfo::getOnlineSince() const { return m_onlineSince; }

uint NickInfo::getNickColor()
{
    // do we already have a color?
    if(!m_nickColor)
    {
        int nickvalue = 0;

        for (uint index = 0; index < m_nickname.length(); index++)
        {
            nickvalue += m_nickname[index].unicode();
        }

        m_nickColor = (nickvalue % 8) + 1;
    }
    // return color offset -1 (since we store it +1 for 0 checking)
    return m_nickColor-1;
}

bool NickInfo::isIdentified() const { return m_identified; }

QString NickInfo::getPrettyOnlineSince() const
{
    QString prettyOnlineSince;
    int daysto = m_onlineSince.date().daysTo( QDate::currentDate());
    if(daysto == 0) prettyOnlineSince = i18n("Today");
    else if(daysto == 1) prettyOnlineSince = i18n("Yesterday");
    else prettyOnlineSince = m_onlineSince.toString("ddd d MMMM yyyy");
    //TODO - we should use KLocale for this
    prettyOnlineSince += ", " + m_onlineSince.toString("h:mm ap");

    return prettyOnlineSince;
}

// Return the Server object that owns this NickInfo object.
IcecapServer* NickInfo::getServer() const { return m_owningServer; }

// Set properties of NickInfo object.
void NickInfo::setNickname(const QString& newNickname)
{
    Q_ASSERT(!newNickname.isEmpty());
    if(newNickname == m_nickname) return;

    m_nickname = newNickname;
    m_loweredNickname = newNickname.lower();

    startNickInfoChangedTimer();
}

void NickInfo::emitNickInfoChanged()
{
//    m_owningServer->emitNickInfoChanged(this);
    emit nickInfoChanged();
}

void NickInfo::startNickInfoChangedTimer()
{
    if(!m_changedTimer->isActive())
    m_changedTimer->start(3000, true /*single shot*/);
}

void NickInfo::setHostmask(const QString& newMask)
{
    if (newMask.isEmpty() || newMask == m_hostmask) return;
    m_hostmask = newMask;

    startNickInfoChangedTimer();
}

void NickInfo::setAway(bool state)
{
    if(state == m_away) return;
    m_away = state;

    startNickInfoChangedTimer();
}

void NickInfo::setIdentified(bool identified)
{
    if(identified == m_identified) return;
    m_identified = identified;
    startNickInfoChangedTimer();
}

void NickInfo::setAwayMessage(const QString& newMessage)
{
    if(m_awayMessage == newMessage) return;
    m_awayMessage = newMessage;

    startNickInfoChangedTimer();
}

void NickInfo::setIdentdInfo(const QString& newIdentdInfo)
{
    if(m_identdInfo == newIdentdInfo) return;
    m_identdInfo = newIdentdInfo;
    startNickInfoChangedTimer();
}

void NickInfo::setVersionInfo(const QString& newVersionInfo)
{
    if(m_versionInfo == newVersionInfo) return;
    m_versionInfo = newVersionInfo;

    startNickInfoChangedTimer();
}

void NickInfo::setNotified(bool state)
{
    if(state == m_notified) return;
    m_notified = state;
    startNickInfoChangedTimer();
}

void NickInfo::setRealName(const QString& newRealName)
{
    if (newRealName.isEmpty() || m_realName == newRealName) return;
    m_realName = newRealName;
    startNickInfoChangedTimer();
}

void NickInfo::setNetServer(const QString& newNetServer)
{
    if (newNetServer.isEmpty() || m_netServer == newNetServer) return;
    m_netServer = newNetServer;
    startNickInfoChangedTimer();
}

void NickInfo::setNetServerInfo(const QString& newNetServerInfo)
{
    if (newNetServerInfo.isEmpty() || newNetServerInfo == m_netServerInfo) return;
    m_netServerInfo = newNetServerInfo;
    startNickInfoChangedTimer();
}

void NickInfo::setOnlineSince(const QDateTime& datetime)
{
    if (datetime.isNull() || datetime == m_onlineSince) return;
    m_onlineSince = datetime;

    startNickInfoChangedTimer();
}

QString NickInfo::tooltip() const
{

    QString strTooltip;
    QTextStream tooltip( &strTooltip, IO_WriteOnly );
    tooltip << "<qt>";

    tooltip << "<table cellspacing=\"0\" cellpadding=\"0\">";
    tooltipTableData(tooltip);
    tooltip << "</table></qt>";
    return strTooltip;
}

void NickInfo::tooltipTableData(QTextStream &tooltip) const
{
    tooltip << "<tr><td colspan=\"2\" valign=\"top\">";

    bool dirty = false;
    bool isimage=false;
    tooltip << "<b>" << (isimage?"":"<center>");
	if(!getRealName().isEmpty() && getRealName().lower() != loweredNickname())
    {
        QString escapedRealName( getRealName() );
        escapedRealName.replace("<","&lt;").replace(">","&gt;");
        tooltip << escapedRealName;
        dirty = true;
    }
    else
    {
        tooltip << getNickname();
        //Don't set dirty if all we have is their nickname
    }
    if(m_identified) tooltip << i18n(" (identified)");
    tooltip << (isimage?"":"</center>") << "</b>";

    tooltip << "</td></tr>";
    if(!getHostmask().isEmpty())
    {
        tooltip << "<tr><td><b>" << i18n("Hostmask:") << " </b></td><td>" << getHostmask() << "</td></tr>";
        dirty=true;
    }
    if(isAway())
    {
        tooltip << "<tr><td><b>" << i18n("Away Message:") << " </b></td><td>";
        if(!getAwayMessage().isEmpty())
            tooltip << getAwayMessage();
        else
            tooltip << i18n("(unknown)");
        tooltip << "</td></tr>";
        dirty=true;
    }
    if(!getOnlineSince().toString().isEmpty())
    {
        tooltip << "<tr><td><b>" << i18n("Online Since:") << " </b></td><td>" << getPrettyOnlineSince() << "</td></tr>";
        dirty=true;
    }

}

void NickInfo::setPrintedOnline(bool printed)
{
    m_printedOnline=printed;
}

bool NickInfo::getPrintedOnline()
{
    if(this)
        return m_printedOnline;
    else
        return false;
}

#include "nickinfo.moc"
