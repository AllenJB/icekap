/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Dialog for quick connection to an IRC network without adding a server in the Server List.
  begin:     Sat June 05 2004
  copyright: (C) 2004 by Michael Goettsche
  email:     mail@tuxipuxi.de
*/

#include <qlayout.h>
#include <qwhatsthis.h>
#include <qlabel.h>
#include <qcheckbox.h>

#include <klineedit.h>
#include <klocale.h>

#include "icecapquickconnectdialog.h"
#include "konversationapplication.h"

IcecapQuickConnectDialog::IcecapQuickConnectDialog(QWidget *parent)
:KDialogBase(parent, "quickconnect", true, i18n("Icecap Quick Connect"),
KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok, true)
{
    QWidget* page = new QWidget(this);
    setMainWidget(page);

    QGridLayout* layout = new QGridLayout(page, 4, 4);
    layout->setSpacing(spacingHint());
    layout->setColStretch(1, 10);

    QLabel* nameLabel = new QLabel(i18n("&Server name:"), page);
    QString nameWT = i18n("Enter a name for the server here.");
    QWhatsThis::add(nameLabel, nameWT);
    nameInput = new KLineEdit("Icecap Server", page);
    QWhatsThis::add(nameInput, nameWT);
    nameLabel->setBuddy(nameInput);

    QLabel* hostNameLabel = new QLabel(i18n("Server &host:"), page);
    QString hostNameWT = i18n("Enter the host of the server here.");
    QWhatsThis::add(hostNameLabel, hostNameWT);
    hostNameInput = new KLineEdit("127.0.0.1", page);
    QWhatsThis::add(hostNameInput, hostNameWT);
    hostNameLabel->setBuddy(hostNameInput);

    QLabel* portLabel = new QLabel(i18n("&Port:"), page);
    QString portWT = i18n("The port that the Icecap server is using.");
    QWhatsThis::add(portLabel, portWT);
    portInput = new KLineEdit("1027", page );
    QWhatsThis::add(portInput, portWT);
    portLabel->setBuddy(portInput);
/*
    QLabel* nickLabel = new QLabel(i18n("&Nick:"), page);
    QString nickWT = i18n("The nick you want to use.");
    QWhatsThis::add(nickLabel, nickWT);
    nickInput = new KLineEdit(Preferences::nickname(0), page);
    QWhatsThis::add(nickInput, nickWT);
    nickLabel->setBuddy(nickInput);

    QLabel* passwordLabel = new QLabel(i18n("P&assword:"), page);
    QString passwordWT = i18n("If the Icecap server requires a password, enter it here (most servers do not require a password.)");
    QWhatsThis::add(passwordLabel, passwordWT);
    passwordInput = new KLineEdit(page);
    QWhatsThis::add(passwordInput, passwordWT);
    passwordLabel->setBuddy(passwordInput);

    sslCheckBox = new QCheckBox(page, "sslCheckBox");
    sslCheckBox->setText(i18n("&Use SSL"));
*/
    layout->addWidget (nameLabel, 0, 0);
    layout->addWidget (nameInput, 0, 1);

    layout->addWidget(hostNameLabel, 1, 0);
    layout->addWidget(hostNameInput, 1, 1);

    layout->addWidget(portLabel, 1, 2);
    layout->addWidget(portInput, 1, 3);
/*
    layout->addWidget(nickLabel, 2, 0);
    layout->addWidget(nickInput, 2, 1);

    layout->addWidget(passwordLabel, 2, 2);
    layout->addWidget(passwordInput, 2, 3);

    layout->addWidget(sslCheckBox, 3, 0);
*/
    hostNameInput->setFocus();

    setButtonOK(KGuiItem(i18n("C&onnect"),"connect_creating",i18n("Connect to the Icecap server")));
}

IcecapQuickConnectDialog::~IcecapQuickConnectDialog()
{
}

void IcecapQuickConnectDialog::slotOk()
{
    if(!hostNameInput->text().isEmpty() &&
        !portInput->text().isEmpty() &&
        !nickInput->text().isEmpty())
    {
        emit connectClicked(nameInput->text().stripWhiteSpace(),
            hostNameInput->text().stripWhiteSpace(),
            portInput->text(),
            "", // passwords not implemented
            false);
        delayedDestruct();
    }
}

#include "icecapquickconnectdialog.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
