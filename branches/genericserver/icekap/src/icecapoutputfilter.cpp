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

#include <qstringlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <qmap.h>
#include <qvaluelist.h>
#include <qtextcodec.h>

#include <klocale.h>
#include <kdebug.h>
#include <kio/passdlg.h>
#include <kconfig.h>
#include <kdeversion.h>
#include <kshell.h>

#include <ksocketaddress.h>
#include <kresolver.h>
#include <kreverseresolver.h>

#include "icecapoutputfilter.h"
#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "ignore.h"
#include "icecapserver.h"
#include "irccharsets.h"
#include "konviiphelper.h"

#include "query.h"

namespace Icecap
{
    IcecapOutputFilter::IcecapOutputFilter(IcecapServer* server)
        : QObject(server)
    {
        m_server = server;
    }

    IcecapOutputFilter::~IcecapOutputFilter()
    {
    }

    // replace all aliases in the string and return true if anything got replaced at all
    bool IcecapOutputFilter::replaceAliases(QString& line)
    {
        QStringList aliasList=Preferences::aliasList();
        QString cc(Preferences::commandChar());
        // check if the line starts with a defined alias
        for(unsigned int index=0;index<aliasList.count();index++)
        {
            // cut alias pattern from definition
            QString aliasPattern(aliasList[index].section(' ',0,0));
            // cut first word from command line, so we do not wrongly find an alias
            // that starts with the same letters, like /m would override /me
            QString lineStart=line.section(' ',0,0);

            // pattern found?
            // TODO: cc may be a regexp character here ... we should escape it then
            if (lineStart==cc+aliasPattern)
            {
                QString aliasReplace;

                // cut alias replacement from definition
                if ( aliasList[index].contains("%p") )
                    aliasReplace = aliasList[index].section(' ',1);
                else
                    aliasReplace = aliasList[index].section(' ',1 )+' '+line.section(' ',1 );

                // protect "%%"
                aliasReplace.replace("%%","%\x01");
                // replace %p placeholder with rest of line
                aliasReplace.replace("%p",line.section(' ',1));
                // restore "%<1>" as "%%"
                aliasReplace.replace("%\x01","%%");
                // modify line
                line=aliasReplace;
                // return "replaced"
                return true;
            }
        }                                         // for

        return false;
    }

    QStringList IcecapOutputFilter::splitForEncoding(const QString& inputLine, int MAX)
    {
//        QString channelCodecName=Preferences::channelEncoding(m_server->getServerGroup(), destination);
        QString channelCodecName = "utf8";

        int sublen=0; //The encoded length since the last split
        int charLength=0; //the length of this char

        //MAX= 7; // for testing

        //FIXME should we run this through the encoder first, checking with "canEncode"?
        QString text=inputLine; // the text we'll send, currently in Unicode

        QStringList finals; // The strings we're going to output

        QChar   *c=(QChar*)text.unicode(),  // Pointer to the character we're looking at
                *end=c+text.length();       // If it were a char*, it would be pointing at the \0;

        QChar   *frag=c,                    // The beginning of this fragment
                *lastSBC=c,                 // The last single byte char we saw in case we have to chop
                *lastSpace=c;               // The last space we saw
        bool sbcGood = 0;                   // We can't trust that a single byte char was ever seen

        //Get the codec we're supposed to use. This must not fail. (not verified)
        QTextCodec* codec;

        // FIXME Doesn't this just return `getIdentity()->getCodec();` if no codec set?
        codec = Konversation::IRCCharsets::self()->codecForName(channelCodecName);

        Q_ASSERT(codec);

        while (c<end)
        {
            // The most important bit - turn the current char into a QCString so we can measure it
            QCString ch=codec->fromUnicode(*c);
            charLength=ch.length();

            // If adding this char puts us over the limit:
            if (charLength+sublen > MAX)
            {
                // If lastSpace isn't pointing to a space, we have to chop
                if ( !lastSpace->isSpace() ) //used in case we end up supporting unicode spaces
                {
                    // FIXME This is only theory, it might not work
                    if (sbcGood) // Copy up to and including the last SBC.
                    {
                        QString curs(frag, (++lastSBC)-frag); // Since there is a continue below, safe to increment here
                        finals+=curs;
                        lastSpace=frag=c=lastSBC;
                    }
                    else //Split right here
                    {
                        QString curs(frag, c-frag); // Don't include c
                        finals+=curs;
                        lastSpace=frag=c; //we need to see this char again, to collect its stats, so no increment
                    }
                }
                else // Most common case, we saw a space we can split at
                {
                    // Copy the current substring, but not the space (unlike the SBC case)
                    QString curs(frag, lastSpace-frag);
                    finals+=curs;

                    // Rewind to the last good splitpoint, dropping the space character as
                    //  it was technically replaced with a \n
                    frag=c=++lastSpace; //pre-increment
                }

                sbcGood=false;
                // Since c always gets reset to the splitpoint, sublen gets recalculated
                sublen=0;
                continue; // SLAM!!!
            }
            else if (*c==' ') // If we want to support typographic spaces, change this to c->isSpace(), and rewrite above
            {
                lastSpace=c;
            }
            // Won't get here if the string was split
            if (charLength==1)
            {
                sbcGood=1;
                lastSBC=c;
            }
            sublen+=charLength;
            ++c;
        }//wend
        //so when we hit this point, frag to end is all thats left
        Q_ASSERT((frag!=c));
        if (frag != c)
        {
            QString curs(frag, c-frag);
            finals+=curs;
        }
        return finals;
    }

    OutputFilterResult IcecapOutputFilter::parse(const QString& myNick,const QString& originalLine,const QString& name)
    {
        setCommandChar();

        OutputFilterResult result;
        destination=name;

        QString inputLine(originalLine);

        if(inputLine.isEmpty() || inputLine == "\n")
            return result;

        if(!Preferences::disableExpansion())
        {
            // replace placeholders
            inputLine.replace("%%","%\x01");      // make sure to protect double %%
            inputLine.replace("%B","\x02");       // replace %B with bold char
            inputLine.replace("%C","\x03");       // replace %C with color char
            inputLine.replace("%G","\x07");       // replace %G with ASCII BEL 0x07
            inputLine.replace("%I","\x09");       // replace %I with italics char
            inputLine.replace("%O","\x0f");       // replace %O with reset to default char
            inputLine.replace("%S","\x13");       // replace %S with strikethru char
            //  inputLine.replace(QRegExp("%?"),"\x15");
            inputLine.replace("%R","\x16");       // replace %R with reverse char
            inputLine.replace("%U","\x1f");       // replace %U with underline char
            inputLine.replace("%\x01","%");       // restore double %% as single %
        }

        QString line=inputLine.lower();

        if (line.startsWith (commandChar +"network"))
        {
            QString parameter = inputLine.section(' ', 1);
            parameter = parameter.stripWhiteSpace();
            result = parseNetwork (parameter);
        }
        else if (line.startsWith (commandChar +"presence"))
        {
            QString parameter = inputLine.section(' ', 1);
            parameter = parameter.stripWhiteSpace();
            result = parseMyPresence (parameter);
        }
        else if (line.startsWith (commandChar +"channel"))
        {
            QString parameter = inputLine.section (' ', 1);
            parameter = parameter.stripWhiteSpace();
            result = parseChannel (parameter);
        }
        else

        // Convert double command chars at the beginning to single ones
        if(line.startsWith(commandChar+commandChar) && !destination.isEmpty())
        {
            inputLine=inputLine.mid(1);
            goto BYPASS_COMMAND_PARSING;
        }
        // Server command?
        else if(line.startsWith(commandChar))
        {
            QString command = inputLine.section(' ', 0, 0).mid(1).lower();
            QString parameter = inputLine.section(' ', 1);

            if(command == "exec")     result = parseExec(parameter);
            else if(command == "raw")      result = parseRaw(parameter);
            else if(command == "konsole")  parseKonsole();
            else if(command == "server")   parseServer(parameter);
            else if(command == "reconnect")  emit reconnectServer();
            else if(command == "disconnect") emit disconnectServer();
            else if(command == "prefs")    result = parsePrefs(parameter);
            else if(command == "dns")      result = parseDNS(parameter);

            // Forward unknown commands to server
            else
            {
                result.toServer = inputLine.mid(1);
                result.type = Message;
            }
        }
        // Ordinary message to channel/query?
        else if(!destination.isEmpty())
        {
            BYPASS_COMMAND_PARSING:
            result.type = Message;
        }
        // Eveything else goes to the server unchanged
        else
        {
            result.toServer = inputLine;
            result.output = inputLine;
            result.typeString = i18n("Raw");
            result.type = Program;
        }

        return result;
    }


    OutputFilterResult IcecapOutputFilter::parseMyPresence (const QString& parameter)
    {
        OutputFilterResult result;
        if (parameter == "list")
        {
            result.toServer = "prslist;presence list";
            result.type = Command;
        }
        else if (parameter == "display")
        {
            m_server->presenceListDisplay ();
        }
        else if (parameter.startsWith ("add "))
        {
            QStringList parameterList = QStringList::split(" ", parameter);
            if (parameterList.size() < 3) {
                result.output = "Usage: /presence (list|display|add|del) [name] [network]";
                result.type = Program;
                return result;
            }
            QString myp = parameterList[1];
            parameterList.pop_front ();
            parameterList.pop_front ();
            QString network = parameterList.join (" ");
            result.toServer = "prsadd;presence add;mypresence="+ myp +";network="+ network;
            result.type = Command;
        }
        else if (parameter.startsWith ("del "))
        {
            QStringList parameterList = QStringList::split(" ", parameter);
            if (parameterList.size() < 3) {
                result.output = "Usage: /presence (list|display|add|del) [name] [network]";
                result.type = Program;
                return result;
            }
            QString myp = parameterList[1];
            parameterList.pop_front ();
            parameterList.pop_front ();
            QString network = parameterList.join (" ");
            result.toServer = "prsdel;presence remove;mypresence="+ myp +";network="+ network;
            result.type = Command;
        }
        else
        {
            result.output = "Usage: /presence (list|display|add|del) [name] [network]";
            result.type = Program;
        }
        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseNetwork (const QString& parameter)
    {
        OutputFilterResult result;
        if (parameter == "list")
        {
            result.toServer = "netlist;network list";
            result.type = Command;
        }
        else if (parameter == "display")
        {
            m_server->networkListDisplay ();
        }
        else if (parameter.startsWith ("add "))
        {
            QStringList parameterList = QStringList::split(" ", parameter);
            if (parameterList.size() < 3) {
                result.output = "Usage: /network (list|display|add|del) [protocol] [name]";
                result.type = Program;
                return result;
            }
            QString protocol = parameterList[1];
            parameterList.pop_front ();
            parameterList.pop_front ();
            QString network = parameterList.join (" ");
            result.toServer = "netadd;network add;protocol="+ protocol +";network="+ network;
            result.type = Command;
        }
        else if (parameter.startsWith ("del "))
        {
            QStringList parameterList = QStringList::split(" ", parameter);
            if (parameterList.size() < 3) {
                result.output = "Usage: /network (list|display|add|del) [protocol] [name]";
                result.type = Program;
                return result;
            }
            QString protocol = parameterList[1];
            parameterList.pop_front ();
            parameterList.pop_front ();
            QString network = parameterList.join (" ");
            result.toServer = "netdel;network remove;protocol="+ protocol +";network="+ network;
            result.type = Command;
        }
        else
        {
            result.output = "Usage: /network (list|display|add|del) [protocol] [name]";
            result.type = Program;
        }
        return result;
    }


    OutputFilterResult IcecapOutputFilter::parseChannel (const QString& parameter)
    {
        QString command = "channel";
        QString cmd = "ch";
        QString usage = "Usage: /"+ command +" (list|display|add|del) [channel] [mypresence] [network]";

        OutputFilterResult result;
        if (parameter == "list")
        {
            result.toServer = cmd +"list;"+ command +" list";
            result.type = Command;
        }
        else if (parameter == "display")
        {
            m_server->networkListDisplay ();
        }
        else if (parameter.startsWith ("add "))
        {
            QStringList parameterList = QStringList::split(" ", parameter);
            if (parameterList.size() < 4) {
                result.output = usage;
                result.type = Program;
                return result;
            }
            QString channel = parameterList[1];
            QString mypresence = parameterList[2];
            parameterList.pop_front ();
            parameterList.pop_front ();
            parameterList.pop_front ();
            QString network = parameterList.join (" ");
            result.toServer = cmd +"add;"+ command +" add;channel="+ channel +";mypresence="+ mypresence +";network="+ network;
            result.type = Command;
        }
        else if (parameter.startsWith ("del "))
        {
            QStringList parameterList = QStringList::split(" ", parameter);
            if (parameterList.size() < 4) {
                result.output = usage;
                result.type = Program;
                return result;
            }
            QString channel = parameterList[1];
            QString mypresence = parameterList[2];
            parameterList.pop_front ();
            parameterList.pop_front ();
            parameterList.pop_front ();
            QString network = parameterList.join (" ");
            result.toServer = cmd +"del;"+ command +" remove;channel="+ channel +";mypresence="+ mypresence +";network="+ network;
            result.type = Command;
        }
        else
        {
            result.output = usage;
            result.type = Program;
        }
        return result;
    }


    OutputFilterResult IcecapOutputFilter::parseExec(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1EXEC <script> [parameter list]").arg(commandChar));
        }
        else
        {
            QStringList parameterList = QStringList::split(' ', parameter);

            if(parameterList[0].find("../") == -1)
            {
                emit launchScript(destination, parameter);
            }
            else
            {
                result = error(i18n("Script name may not contain \"../\"!"));
            }
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseRaw(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty() || parameter == "open")
        {
            emit openRawLog(true);
        }
        else if(parameter == "close")
        {
            emit closeRawLog();
        }
        else
        {
            result = usage(i18n("Usage: %1RAW [OPEN | CLOSE]").arg(commandChar));
        }

        return result;
    }

    void IcecapOutputFilter::parseKonsole()
    {
        emit openKonsolePanel();
    }

    // Accessors

    void IcecapOutputFilter::setCommandChar() { commandChar=Preferences::commandChar(); }

    OutputFilterResult IcecapOutputFilter::usage(const QString& string)
    {
        OutputFilterResult result;
        result.typeString = i18n("Usage");
        result.output = string;
        result.type = Program;
        return result;
    }

    OutputFilterResult IcecapOutputFilter::info(const QString& string)
    {
        OutputFilterResult result;
        result.typeString = i18n("Info");
        result.output = string;
        result.type = Program;
        return result;
    }

    OutputFilterResult IcecapOutputFilter::error(const QString& string)
    {
        OutputFilterResult result;
        result.typeString = i18n("Error");
        result.output = string;
        result.type = Program;
        return result;
    }

    void IcecapOutputFilter::parseServer(const QString& parameter)
    {
        if(parameter.isEmpty())
        {
            emit reconnectServer();
        }
        else
        {
            QStringList splitted = QStringList::split(" ", parameter);
            QString host = splitted[0];
            QString port = "6667";
            QString password;

            // 'hostname port password'
            if (splitted.count()==3)
            {
                KonviIpHelper hostParser(host);
                host = hostParser.host();

                port = splitted[1];
                password = splitted[2];
            }
            // 'hostname:port password' or 'hostname port'
            else if (splitted.count()==2)
            {
                KonviIpHelper hostParser(host);
                host = hostParser.host();

                if (!hostParser.port().isEmpty())
                {
                    port = hostParser.port();
                    password = splitted[1];
                }
                else
                {
                    port = splitted[1];
                }
            }
            // 'hostname:port' or 'hostname'
            else
            {
                KonviIpHelper hostParser(host);
                host = hostParser.host();

                if (!hostParser.port().isEmpty())
                {
                    port = hostParser.port();
                }
            }

            if (Preferences::isServerGroup(host))
            {
                emit connectToServerGroup(host);
            }
            else
            {
                emit connectToServer(host, port, password);
            }
        }
    }

    OutputFilterResult IcecapOutputFilter::parsePrefs(const QString& parameter)
    {
        OutputFilterResult result;
        bool showUsage = false;

        if (parameter.isEmpty())
            showUsage = true;
        else
        {
            KConfig* config=KApplication::kApplication()->config();

            QStringList splitted = KShell::splitArgs(parameter);

            if (splitted.count() > 0)
            {
                QString group = splitted[0];
                QStringList groupList(config->groupList());
                uint i;
                if (group.lower() == "list")
                {
                    // List available groups.
                    result = usage(i18n("Available preferences groups: ") + groupList.join("|"));
                }
                else
                {
                    // Validate group.
                    bool validGroup = false;
                    for (i = 0; i < groupList.count(); ++i)
                    {
                        if (group.lower() == groupList[i].lower())
                        {
                            validGroup = true;
                            group = groupList[i];
                            break;
                        }
                    }
                    if (validGroup && splitted.count() > 1)
                    {
                        QString option = splitted[1];
                        QMap<QString,QString> options = config->entryMap(group);
                        QValueList<QString> optionList = options.keys();
                        QValueList<QString> optionValueList = options.values();

                        if (option.lower() == "list")
                        {
                            // List available options in group.
                            QString output = i18n("Available options in group %1:").arg( group );

                            for (i = 0; i < optionList.count(); ++i)
                            {
                                output += optionList[i] + '(' + optionValueList[i] + ")|";
                            }

                            result = usage(output);
                        }
                        else
                        {
                            // Validate option.
                            bool validOption = false;
                            for (i = 0; i < optionList.count(); ++i)
                            {
                                if (option.lower() == optionList[i].lower())
                                {
                                    validOption = true;
                                    option = optionList[i];
                                    break;
                                }
                            }
                            if (validOption)
                            {
                                if (splitted.count() > 2)
                                {
                                    // Set the desired option.
                                    config->setGroup(group);
                                    config->writeEntry(option, splitted[2]);
                                    config->sync();
                                    // Reload preferences object.
                                    dynamic_cast<KonversationApplication*>(kapp)->readOptions();
                                }
                                // If no value given, just display current value.
                                else
                                {
                                    result = usage(group + '/' + option + " = " + options[option]);
                                }
                            }
                            else
                            {
                                showUsage = true;
                            }
                        }
                    }
                    else
                    {
                        showUsage = true;
                    }
                }
            }
            else
            {
                showUsage = true;
            }
        }

        if (showUsage)
        {
            result = usage(i18n("Usage: %1PREFS <group> <option> <value> or %2PREFS LIST to list groups or %3PREFS group LIST to list options in group.  Quote parameters if they contain spaces.").arg(commandChar, commandChar, commandChar));
        }

        return result;
    }

    OutputFilterResult IcecapOutputFilter::parseDNS(const QString& parameter)
    {
        OutputFilterResult result;

        if(parameter.isEmpty())
        {
            result = usage(i18n("Usage: %1DNS <nick>").arg(commandChar));
        }
        else
        {
            QStringList splitted = QStringList::split(" ", parameter);
            QString target = splitted[0];

            KIpAddress address(target);

            // Parameter is an IP address
            if (address.isIPv4Addr() || address.isIPv6Addr())
            {
            // Disable the reverse resolve codepath on older KDE versions due to many
            // distributions shipping visibility-enabled KDE 3.4 and KNetwork not
            // coping with it.
#if KDE_IS_VERSION(3,5,1)
                KNetwork:: KInetSocketAddress socketAddress(address,0);
                QString resolvedTarget;
                QString serv; // We don't need this, but KReverseResolver::resolve does.

                if (KNetwork::KReverseResolver::resolve(socketAddress,resolvedTarget,serv))
                {
                    result.typeString = i18n("DNS");
                    result.output = i18n("Resolved %1 to: %2").arg(target).arg(resolvedTarget);
                    result.type = Program;
                }
                else
                {
                    result = error(i18n("Unable to resolve %1").arg(target));
                }
#else
                result = error(i18n("Reverse-resolving requires KDE version 3.5.1 or higher."));
#endif
            }
            // Parameter is presumed to be a host due to containing a dot. Yeah, it's dumb.
            // FIXME: The reason we detect the host by occurrence of a dot is the large penalty
            // we would incur by using inputfilter to find out if there's a user==target on the
            // server - once we have a better API for this, switch to it.
            else if (target.contains('.'))
            {
                KNetwork::KResolverResults resolved = KNetwork::KResolver::resolve(target,"");
                if(resolved.error() == KResolver::NoError && resolved.size() > 0)
                {
                    QString resolvedTarget = resolved.first().address().nodeName();
                    result.typeString = i18n("DNS");
                    result.output = i18n("Resolved %1 to: %2").arg(target).arg(resolvedTarget);
                    result.type = Program;
                }
                else
                {
                    result = error(i18n("Unable to resolve %1").arg(target));
                }
            }
            // Parameter is either host nor IP, so request a lookup from server, which in
            // turn lets inputfilter do the job.
        }

        return result;
    }

}
#include "icecapoutputfilter.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
