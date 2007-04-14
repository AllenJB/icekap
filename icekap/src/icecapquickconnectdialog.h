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

#ifndef ICECAPQUICKCONNECTDIALOG_H
#define ICECAPQUICKCONNECTDIALOG_H

#include <kdialogbase.h>

/*
    @author Michael Goettsche
*/

class KLineEdit;

class IcecapQuickConnectDialog : public KDialogBase
{
    Q_OBJECT

        public:
        IcecapQuickConnectDialog(QWidget* parent=0);
        ~IcecapQuickConnectDialog();

        signals:
        void connectClicked(const QString& name,
            const QString& hostName,
            const QString& port,
            const QString& password,
            const bool& useSSL
            );

    protected slots:
        void slotOk();

    protected:
        KLineEdit*  nameInput;
        KLineEdit*  hostNameInput;
        KLineEdit*  portInput;
//        KLineEdit*  passwordInput;
//        KLineEdit*  nickInput;
//        QCheckBox*      sslCheckBox;
};
#endif

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
