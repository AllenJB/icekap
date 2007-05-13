/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "texteventhandler.h"

#include <klocale.h>

#include "icecapserver.h"
#include "icecapmypresence.h"

TextEventHandler::TextEventHandler (IcecapServer* server)
{
    m_server = server;
}

void TextEventHandler::processEvent (const QString type, const QMap<QString, QString> parameter)
{
    if (type == "preauth") {
        m_server->appendStatusMessage (i18n("Welcome"), "Successfully connected to Icecap server.");
    } else if (type == "network_init") {
        m_server->appendStatusMessage (i18n("Network"), "Network added: ["+ parameter["protocol"] +"] "+ parameter["network"]);
    } else if (type == "network_deinit") {
        m_server->appendStatusMessage (i18n("Network"), "Network deleted: "+ parameter["network"]);
    } else if (type == "local_presence_init") {
        m_server->appendStatusMessage (i18n("Presence"), "Presence added: ["+ parameter["mypresence"] +"] "+ parameter["network"]);
    } else if (type == "local_presence_deinit") {
        m_server->appendStatusMessage (i18n("Presence"), "Presence deleted: ["+ parameter["mypresence"] +"] "+ parameter["network"]);
    } else if ( type == "channel_init" ) {
        m_server->appendStatusMessage ( i18n("Channel"), "Channel added: "+ parameter["channel"] +" to presence: "+ parameter["mypresence"] +" on network: "+ parameter["network"]);
    } else if ( type == "channel_deinit" ) {
        m_server->appendStatusMessage ( i18n("Channel"), "Channel deleted: "+ parameter["channel"] +" from presence: "+ parameter["mypresence"] +" on network: "+ parameter["network"]);
    }

    else if ( type == "channel_connection_init" ) {
        Icecap::MyPresence* myp = m_server->mypresence(parameter["mypresence"], parameter["network"]);
        myp->appendStatusMessage (i18n ("Channel"), "Connected to channel: "+ parameter["channel"]);
    }
    else if ( type == "channel_connection_deinit" ) {
        Icecap::MyPresence* myp = m_server->mypresence(parameter["mypresence"], parameter["network"]);
        if (parameter["reason"].length () > 0) {
            myp->appendStatusMessage (i18n ("Channel"), "Disconnected from channel: "+ parameter["channel"] +": "+ parameter["reason"]);
        } else {
            myp->appendStatusMessage (i18n ("Channel"), "Disconnected from channel: "+ parameter["channel"]);
        }
    }

    else if (type == "network_list_error") {
        m_server->appendStatusMessage (i18n ("Network List"),
            "Network List Error: An unhandled error occurred.");
    } else if (type == "network_list") {
        QString message = i18n ("%1 Network: %2").arg (parameter["protocol"]).arg (parameter["network"]);
        m_server->appendStatusMessage (i18n ("Network List"), message);
    } else if (type == "network_list_end") {
        m_server->appendStatusMessage (i18n ("Network List"), "End of network list");
    }

    else if (type == "network_add") {
        m_server->appendStatusMessage (i18n ("Network"), "Network added successfully.");
    } else if (type == "network_add_error") {
        m_server->appendStatusMessage (i18n ("Network"),
            "Network Add: Error: An unhandled error occurred.");
    } else if (type == "network_del") {
        m_server->appendStatusMessage (i18n ("Network"), "Network deleted successfully.");
    } else if (type == "network_del_error") {
        m_server->appendStatusMessage (i18n ("Network"),
            "Network Del: Error: An unhandled error occurred.");
    }

    else if (type == "presence_list_end") {
        m_server->appendStatusMessage (i18n ("Presence List"), "End of presence list");
    } else if (type == "presence_list") {
        QString message = i18n ("%1 Network: %2", "%1 Network: %2").arg (parameter["mypresence"]).arg (parameter["network"]);
        m_server->appendStatusMessage (i18n ("Presence List"), message);
    } else if (type == "presence_list_error") {
        m_server->appendStatusMessage (i18n ("Presence List"),
            "Presence List Error: An unhandled error occurred.");
    }

    else if (type == "presence_add") {
        m_server->appendStatusMessage (i18n ("Presence"), "Presence added successfully.");
    } else if (type == "presence_add_error") {
        m_server->appendStatusMessage (i18n ("Presence"),
            "Presence Add: Error: An unhandled error occurred.");
    } else if (type == "presence_del") {
        m_server->appendStatusMessage (i18n ("Presence"), "Presence deleted successfully.");
    } else if (type == "presence_del_error") {
        m_server->appendStatusMessage (i18n ("Presence"),
            "Presence Del: Error: An unhandled error occurred.");
    }

    else if (type == "channel_list_end") {
        m_server->appendStatusMessage (i18n ("Channel List"), "End of channel list");
    } else if (type == "channel_list") {
        QString message = i18n ("%1 Network: %2 Presence: %3", "%1 Network: %2 Presence: %3").arg(parameter["channel"]).arg (parameter["network"]).arg (parameter["mypresence"]);
        m_server->appendStatusMessage (i18n ("Channel List"), message);
    } else if (type == "channel_list_error") {
        m_server->appendStatusMessage (i18n ("Channel List"),
            "Channel List Error: An unhandled error occurred.");
    }

    else if (type == "channel_add") {
        m_server->appendStatusMessage (i18n ("Channel"), "Channel added successfully.");
    } else if (type == "channel_add_error") {
        m_server->appendStatusMessage (i18n ("Channel"),
            "Channel Add: Error: An unhandled error occurred.");
    } else if (type == "channel_del") {
        m_server->appendStatusMessage (i18n ("Channel"), "Channel deleted successfully.");
    } else if (type == "channel_del_error") {
        m_server->appendStatusMessage (i18n ("Channel"),
            "Channel Del: Error: An unhandled error occurred.");
    }

    else if (type == "gateway_list_end") {
        m_server->appendStatusMessage (i18n ("Gateway List"), "End of gateway list");
    } else if (type == "gateway_list") {
        QString message = i18n ("%1 Network: %2", "%1 Network: %2").arg(parameter["host"]).arg (parameter["network"]);
        m_server->appendStatusMessage (i18n ("Gateway List"), message);
    } else if (type == "gateway_list_error") {
        m_server->appendStatusMessage (i18n ("Gateway List"),
            "Gateway List Error: An unhandled error occurred.");
    }

    else if (type == "gateway_add") {
        m_server->appendStatusMessage (i18n ("Gateway"), "Gateway added successfully.");
    } else if (type == "gateway_add_error") {
        m_server->appendStatusMessage (i18n ("Gateway"),
            "Gateway Add: Error: An unhandled error occurred.");
    } else if (type == "gateway_del") {
        m_server->appendStatusMessage (i18n ("Gateway"), "Gateway deleted successfully.");
    } else if (type == "gateway_del_error") {
        m_server->appendStatusMessage (i18n ("Gateway"),
            "Gateway Del: Error: An unhandled error occurred.");
    }

    else if (type == "gateway_connecting") {
        QString message = i18n ("Connecting to gateway: %1:%2").arg(parameter["ip"]).arg (parameter["port"]);
        Icecap::MyPresence* myp = m_server->mypresence(parameter["mypresence"], parameter["network"]);
        myp->appendStatusMessage (i18n ("Gateway"), message);
    }
    else if (type == "gateway_connected") {
        QString message = i18n ("Connected to gateway: %1:%2 - in_charsets: %3 - out_charset: %4").arg(parameter["ip"]).arg (parameter["port"]).arg (parameter["in_charsets"]).arg (parameter["out_charset"]);
        m_server->mypresence(parameter["mypresence"], parameter["network"])->appendStatusMessage (i18n ("Gateway"), message);
    }
    else if (type == "gateway_disconnected") {
        QString message = i18n ("Disconnected from gateway.");
        m_server->mypresence(parameter["mypresence"], parameter["network"])->appendStatusMessage (i18n ("Gateway"), message);
    }
    else if (type == "gateway_motd") {
        m_server->mypresence(parameter["mypresence"], parameter["network"])->appendStatusMessage (i18n ("MOTD"), parameter["data"]);
    }
    else if (type == "gateway_motd_end") {
        QString message = i18n ("End of MOTD.");
        m_server->mypresence(parameter["mypresence"], parameter["network"])->appendStatusMessage (i18n ("MOTD"), message);
    }
    else if (type == "gateway_logged_in") {
        QString message = i18n ("Logged in to gateway.");
        m_server->mypresence(parameter["mypresence"], parameter["network"])->appendStatusMessage (i18n ("Gateway"), message);
    }

    else if (type == "channel_presence_added") {
        Icecap::Channel* channel = m_server->mypresence(parameter["mypresence"], parameter["network"])->channel (parameter["channel"]);
        channel->append (">>", "Join: "+ parameter["presence"]);
    }
    else if (type == "channel_presence_removed") {
        Icecap::Channel* channel = m_server->mypresence(parameter["mypresence"], parameter["network"])->channel (parameter["channel"]);
        if (parameter.contains ("type") && (parameter["type"] == "quit")) {
            if (parameter["reason"].length () > 0) {
                channel->append ("<<", "Quit: "+ parameter["presence"] +" Reason: "+ parameter["reason"]);
            } else {
                channel->append ("<<", "Quit: "+ parameter["presence"]);
            }
        } else {
            if (parameter["reason"].length () > 0) {
                channel->append ("<<", "Part: "+ parameter["presence"] +" Reason: "+ parameter["reason"]);
            } else {
                channel->append ("<<", "Part: "+ parameter["presence"]);
            }
        }
    }

    else if (type == "msg") {
        QString escapedMsg = parameter["msg"];
        escapedMsg.replace ("\\.", ";");
        if (parameter["presence"].length () < 1) {
            if (parameter["irc_target"] == "AUTH") {
                m_server->mypresence(parameter["mypresence"], parameter["network"])->appendStatusMessage (parameter["irc_target"], escapedMsg);
            } else {
                m_server->mypresence(parameter["mypresence"], parameter["network"])->appendStatusMessage (i18n("Message"), escapedMsg);
            }
        } else
        if (parameter["channel"].length () > 0) {
            Icecap::Channel* channel = m_server->mypresence(parameter["mypresence"], parameter["network"])->channel (parameter["channel"]);
            if ((parameter["type"].length () > 0) && (parameter["type"] == "action")) {
                channel->appendAction (parameter["presence"], escapedMsg);
            } else {
                channel->append (parameter["presence"], escapedMsg);
            }
        }
        else if (parameter["irc_target"] == "$*") {
            m_server->mypresence(parameter["mypresence"], parameter["network"])->appendStatusMessage (parameter["presence"], escapedMsg);
        }
    }

    // TODO: presence_init (own)
/*
    else if (type == "presence_init") {
        QString message = i18n ("Disconnected from gateway.");
        m_server->mypresence(parameter["mypresence"], parameter["network"])->appendStatusMessage (i18n ("Gateway"), message);
    }
*/

    else {
        // Let developers know when they forget to add something. Users should never see this.
        m_server->appendStatusMessage (i18n ("ERROR"), "ERROR: Unknown message type sent to TextEventHandler: "+ type);
    }
}

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
