/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "icecappresence.h"

#include "klocale.h"

#include "icecapquery.h"

namespace Icecap
{

    Presence::Presence (const QString& name)
    {
        m_connected = false;
        m_nickColor = 0;

        m_name = name;
        m_address = QString ();
        m_real_name = "Unimplemented";
//        m_irc_server = QString ();
        m_away = false;
        m_away_reason = QString ();
        m_identified = false;
        m_login_time = 0;
        m_idle_started = 0;
    }

    Presence::Presence (const QString& name, const QString& address)
    {
        m_name = name;
        m_address = address;
        m_connected = false;
        m_away = false;
        m_real_name = "Unimplemented";
        m_nickColor = 0;
    }

    /**
     * Change presence name
     * @param name New name
     */
    void Presence::setName (const QString& name)
    {
        m_name = name;
        emit nickInfoChanged ();
    }

    /**
     * Set presence connection state
     * @param status New status
     */
    void Presence::setConnected (bool status)
    {
        m_connected = status;
        emit nickInfoChanged ();
    }

    /**
     * Comparison operator
     * @param compareTo Object to compare to
     * @return Same object?
     */
    bool Presence::operator== (Presence compareTo)
    {
        return (compareTo.name() == m_name);
    }

/*
    bool Presence::isNull ()
    {
        return (m_name.isNull());
    }
*/

    /**
     * Update this presence using a parameter map
     * @param parameterList Parameters
     * @todo AllenJB: What other information can we get here? Modes? Away status?
     * @todo AllenJB: idle_started
     */
    void Presence::update (QMap<QString, QString> parameterList)
    {
        // Address (Hostmask)
        if (parameterList.contains ("address")) {
            m_address = parameterList["address"];
        }

        // Nickname
        if (parameterList.contains ("name")) {
            setName (parameterList["name"]);
        }

        // Real name
        if (parameterList.contains ("real_name")) {
            m_real_name = parameterList["real_name"];
        }

        // Away, with reason
        if (parameterList.contains ("reason")) {
            m_away = true;
            m_away_reason = parameterList["reason"];
        }
        else if ((parameterList.contains ("type")) && (parameterList["type"] == "away")) {
            m_away = true;
            m_away_reason = QString ();
        }

        // Last seen active
        if (parameterList.contains ("idle_started")) {
            m_idle_started = parameterList["idle_started"].toUInt();
        }

        // Identified with services, when
        if (parameterList.contains("login_time")) {
            m_identified = true;
            m_login_time = parameterList["login_time"].toUInt();
        }
        else if (parameterList.contains("irc_signon_time")) {
            m_identified = true;
            m_login_time = parameterList["irc_signon_time"].toUInt();
        }
    }

    /**
     * Obtain the nick color for this nick. This is only used for per-nick colors
     * @return Nick color
     */
    uint Presence::getNickColor()
    {
        // do we already have a color?
        if(!m_nickColor)
        {
            int nickvalue = 0;
            for (uint index = 0; index < m_name.length(); index++)
            {
                nickvalue += m_name[index].unicode();
            }
            m_nickColor = (nickvalue % 8) + 1;
        }
        // return color offset -1 (since we store it +1 for 0 checking)
        return m_nickColor-1;
    }

    /**
     * Obtain the most nicely formatted name possible for this presence
     * @return Name
     * @todo AllenJB: Real name support
     */
    QString Presence::getBestAddresseeName()
    {
        return m_name;
/*
        if(!m_addressee.formattedName().isEmpty())
        {
            return m_addressee.formattedName();
        }
        else if(!m_addressee.realName().isEmpty())
        {
            return m_addressee.realName();
        }
        else if(!getRealName().isEmpty() && getRealName().lower() != loweredNickname())
        {
            return getRealName();
        }
        else
        {
            return getNickname();
        }
*/
    }

    /**
     * Obtain the HTML for a tooltip for this presence
     * @param tooltip Tooltip text stream
     * @todo AllenJB: KDE contact support
     */
    void Presence::tooltipTableData(QTextStream &tooltip) const
    {
        tooltip << "<tr><td colspan=\"2\" valign=\"top\">";

        bool dirty = false;
//        KABC::Picture photo = m_addressee.photo();
//        KABC::Picture logo = m_addressee.logo();
        bool isimage=false;
/*
        if(photo.isIntern())
        {
            QMimeSourceFactory::defaultFactory()->setImage( "photo", photo.data() );
            tooltip << "<img src=\"photo\">";
            dirty=true;
            isimage=true;
        }
        else if(!photo.url().isEmpty())
        {
            //JOHNFLUX FIXME TODO: Are there security problems with this?  loading from an external refrence? Assuming not.
            tooltip << "<img src=\"" << photo.url() << "\">";
            dirty=true;
            isimage=true;
        }
        if(logo.isIntern())
        {
            QMimeSourceFactory::defaultFactory()->setImage( "logo", logo.data() );
            tooltip << "<img src=\"logo\">";
            dirty=true;
            isimage=true;
        }
        else if(!logo.url().isEmpty())
        {
            //JOHNFLUX FIXME TODO: Are there security problems with this?  loading from an external refrence? Assuming not.
            tooltip << "<img src=\"" << logo.url() << "\">";
            dirty=true;
            isimage=true;
        }
*/
        tooltip << "<b>" << (isimage?"":"<center>");
/*
        TODO: AllenJB: Real Name support
        if(!m_addressee.formattedName().isEmpty())
        {
            tooltip << m_addressee.formattedName();
            dirty = true;
        }
        else if(!m_addressee.realName().isEmpty())
        {
            tooltip << m_addressee.realName();
            dirty = true;
        }
        else if(!getRealName().isEmpty() && getRealName().lower() != loweredNickname())
        {
            QString escapedRealName( getRealName() );
            escapedRealName.replace("<","&lt;").replace(">","&gt;");
            tooltip << escapedRealName;
            dirty = true;
        }
        else
        {
*/
            tooltip << getNickname();
            //Don't set dirty if all we have is their nickname
//        }
        // TODO AllenJB: Identified support
//        if(m_identified) tooltip << i18n(" (identified)");
        tooltip << (isimage?"":"</center>") << "</b>";

        tooltip << "</td></tr>";
/*
        TODO: AllenJB: KDE contact support
        if(!m_addressee.emails().isEmpty())
        {
            tooltip << "<tr><td><b>" << i18n("Email") << ": </b></td><td>";
            tooltip << m_addressee.emails().join(", ");
            tooltip << "</td></tr>";
            dirty=true;
        }

        if(!m_addressee.organization().isEmpty())
        {
            tooltip << "<tr><td><b>" << m_addressee.organizationLabel() << ": </b></td><td>" << m_addressee.organization() << "</td></tr>";
            dirty=true;
        }
        if(!m_addressee.role().isEmpty())
        {
            tooltip << "<tr><td><b>" << m_addressee.roleLabel() << ": </b></td><td>" << m_addressee.role() << "</td></tr>";
            dirty=true;
        }
        KABC::PhoneNumber::List numbers = m_addressee.phoneNumbers();
        for( KABC::PhoneNumber::List::ConstIterator it = numbers.begin(); it != numbers.end(); ++it)
        {
            tooltip << "<tr><td><b>" << (*it).label() << ": </b></td><td>" << (*it).number() << "</td></tr>";
            dirty=true;
        }
        if(!m_addressee.birthday().toString().isEmpty() )
        {
            tooltip << "<tr><td><b>" << m_addressee.birthdayLabel() << ": </b></td><td>" << m_addressee.birthday().toString("ddd d MMMM yyyy") << "</td></tr>";
            dirty=true;
        }
*/
        if(!getHostmask().isEmpty())
        {
            tooltip << "<tr><td><b>" << i18n("Hostmask:") << " </b></td><td>" << getHostmask() << "</td></tr>";
            dirty=true;
        }
/*
        TODO AllenJB: Away support
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
*/
/*
        TODO: Online since support
        if(!getOnlineSince().toString().isEmpty())
        {
            tooltip << "<tr><td><b>" << i18n("Online Since:") << " </b></td><td>" << getPrettyOnlineSince() << "</td></tr>";
            dirty=true;
        }
*/
    }

}

#include "icecappresence.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
