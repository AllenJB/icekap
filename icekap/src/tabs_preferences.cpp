/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2006 by Eike Hein
  email:     sho@eikehein.com
*/

#include <qcombobox.h>
#include <qcheckbox.h>

#include "tabs_preferences.h"
#include "tabs_preferencesui.h"

Tabs_Config::Tabs_Config(QWidget *parent, const char *name)
 : Tabs_PreferencesUI(parent, name)
{
    connect(kcfg_TabPlacement, SIGNAL(activated(int)), this, SLOT(toggleCheckBoxes(int)));
}

Tabs_Config::~Tabs_Config()
{
}

void Tabs_Config::show()
{
    QWidget::show();

    if (kcfg_TabPlacement->currentItem() == 0 || kcfg_TabPlacement->currentItem() == 1)
    {
        kcfg_ShowTabBarCloseButton->setEnabled(true);
        kcfg_UseMaxSizedTabs->setEnabled(true);
    }
    else
    {
        kcfg_ShowTabBarCloseButton->setEnabled(false);
        kcfg_UseMaxSizedTabs->setEnabled(false);
    }
}

void Tabs_Config::toggleCheckBoxes(int activated)
{
    if (activated == 0 || activated == 1)
    {
        kcfg_ShowTabBarCloseButton->setEnabled(true);
        kcfg_UseMaxSizedTabs->setEnabled(true);
    }
    else
    {
        kcfg_ShowTabBarCloseButton->setEnabled(false);
        kcfg_UseMaxSizedTabs->setEnabled(false);
    }
}

#include "tabs_preferences.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
