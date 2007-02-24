/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Where it all began ...
  begin:     Die Jan 15 05:59:05 CET 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <dcopclient.h>
#include <dcopref.h>
#include <kdebug.h>

#include "konversationapplication.h"
#include "version.h"
#include "commit.h"

/*
  Don't use i18n() here, use I18N_NOOP() instead!
  i18n() will only work as soon as a kapplication object was made.
*/

static const char* shortDescription=I18N_NOOP("A user friendly icecap client");

static const KCmdLineOptions options[] =
{
    { "server <server>", I18N_NOOP("Server to connect"), 0 },
    { "port <port>", I18N_NOOP("Port to use"), "6667"},
    { "channel <channel>", I18N_NOOP("Channel to join after connection"), ""},
    { "nick <nickname>", I18N_NOOP("Nickname to use"),""},
    { "password <password>", I18N_NOOP("Password for connection"),""},
    { "ssl", I18N_NOOP("Use SSL for connection"),"false"},
    KCmdLineLastOption
};

int main(int argc, char* argv[])
{
    KAboutData aboutData("icekap",
        I18N_NOOP("Icekap"),
        KONVI_VERSION,
        shortDescription,
        KAboutData::License_GPL,
        I18N_NOOP("(C) 2007 by the Icekap team"),
        I18N_NOOP("Icekap is a client for the Icecap protocol.\n"
        "Meet friends on the net, make new acquaintances and lose yourself in\n"
        "talk about your favorite subject."),
        "http://trac.allenjb.me.uk/icekap/");

    aboutData.addAuthor("Allen Brooker",I18N_NOOP("Original Author"),"icekap@allenjb.me.uk");
    aboutData.addAuthor("The Konversation Team",I18N_NOOP("Authors of Konversation, on which Icekap is based"),"");

    KCmdLineArgs::init(argc,argv,&aboutData);
    KCmdLineArgs::addCmdLineOptions(options);
    KApplication::addCmdLineOptions();

    if(!KUniqueApplication::start())
    {
        return 0;
    }

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    KonversationApplication app;

    if(args->isSet("server"))
        app.delayedConnectToServer(args->getOption("server"), args->getOption("port"),
            args->getOption("channel"), args->getOption("nick"),
            args->getOption("password"), args->isSet("ssl"));

    return app.exec();
}
