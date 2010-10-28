/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
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

#include "Application.h"

// Qt includes
#include <QtCore/QFile>
#include <QtCore/QStringList>

// KDE includes
#include <KLocale>
#include <KDebug>

// QApt includes
#include <LibQApt/Package>
#include <LibQApt/Backend>

Application::Application(const QString &fileName, QApt::Backend *backend)
        : m_fileName(fileName)
        , m_backend(backend)
        , m_package(0)
        , m_isValid(true)
{
    m_data = desktopContents();
}

Application::Application(QApt::Package *package)
        : m_package(package)
{
}

Application::~Application()
{
}

QString Application::name()
{
    QString name = getField("Name");
    if (name.isEmpty()) {
        return package()->name();
    }

    return i18n(name.toUtf8());
}

QString Application::comment()
{
    QString comment = getField("Comment");
    if (comment.isEmpty()) {
        // Sometimes GenericName is used instead of Comment
        comment = getField("GenericName");
        if (comment.isEmpty()) {
            QApt::Package *pkg = package();

            if (pkg) {
                return pkg->shortDescription();
            }
        }
    }

    return i18n(comment.toUtf8());
}

QApt::Package *Application::package()
{
    if (!m_package) {
        QString packageName = getField("X-AppInstall-Package");
        if (m_backend) {
            m_package = m_backend->package(packageName);
        }
    }

    return m_package;
}

QString Application::icon()
{
    QString icon = getField("Icon");

    return icon;
}

QString Application::categories()
{
    return getField("Categories");
}

int Application::popconScore()
{
    QString popconString = getField("X-AppInstall-Popcon");

    return popconString.toInt();
}

bool Application::isValid()
{
    return m_isValid;
}

QByteArray Application::getField(const QByteArray &field)
{
    return m_data.value(field);
}

QHash<QByteArray, QByteArray> Application::desktopContents()
{
    QHash<QByteArray, QByteArray> contents;

    QFile file(m_fileName);
    if (!file.open(QFile::ReadOnly)) {
        return contents;
    }

    int lineIndex = 0;
    QByteArray buffer = file.readAll();

    QList<QByteArray> lines = buffer.split('\n');

    while (lineIndex < lines.size()) {
        QByteArray line = lines.at(lineIndex);
        if (line.isEmpty() || line.at(0) == '#') {
            lineIndex++;
            continue;
        }

        QByteArray aKey;
        QByteArray aValue;
        int eqpos = line.indexOf('=');

        if (eqpos < 0) {
            // Invalid
            lineIndex++;
            continue;
        } else {
            aKey = line.left(eqpos);
            aValue = line.right(line.size() - eqpos -1);

            contents[aKey] = aValue;
        }

        lineIndex++;
    }

    if (contents.isEmpty()) {
        m_isValid = false;
    }

    return contents;
}
