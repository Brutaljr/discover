/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "AbstractBackendUpdater.h"
#include "AbstractResource.h"

AbstractBackendUpdater::AbstractBackendUpdater(QObject* parent)
    : QObject(parent)
{}

void AbstractBackendUpdater::cancel()
{
    Q_ASSERT(isCancelable() && "only call cancel when cancelable");
    Q_ASSERT(false && "if it can be canceled, then ::cancel() must be implemented");
}

void AbstractBackendUpdater::fetchChangelog() const
{
    foreach(auto res, toUpdate()) {
        res->fetchChangelog();
    }
}

void AbstractBackendUpdater::enableNeedsReboot()
{
    if (m_needsReboot)
        return;

    m_needsReboot = true;
    Q_EMIT needsRebootChanged();
}

bool AbstractBackendUpdater::needsReboot() const
{
    return m_needsReboot;
}
