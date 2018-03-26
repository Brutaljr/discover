/***************************************************************************
 *   Copyright © 2013 Lukas Appelhans <l.appelhans@gmx.de>                 *
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
#include "ApplicationNotifier.h"

// Qt includes
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QtCore/QStandardPaths>
#include <QtCore/QProcess>
#include <QtGui/QIcon>

// KDE includes
#include <KDirWatch>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KNotification>

ApplicationNotifier::ApplicationNotifier(QObject* parent)
  : BackendNotifierModule(parent)
  , m_checkerProcess(Q_NULLPTR)
  , m_updateCheckerProcess(Q_NULLPTR)
  , m_securityUpdates(0)
  , m_normalUpdates(0)
{
    KDirWatch *stampDirWatch = new KDirWatch(this);
    stampDirWatch->addFile(QStringLiteral("/var/lib/update-notifier/dpkg-run-stamp"));
    connect(stampDirWatch, &KDirWatch::dirty, this, &ApplicationNotifier::distUpgradeEvent);

    QTimer* t = new QTimer(this);
    t->setSingleShot(true);
    t->setInterval(2000);
    connect(t, &QTimer::timeout, this, &ApplicationNotifier::recheckSystemUpdateNeeded);

    stampDirWatch = new KDirWatch(this);
    stampDirWatch->addDir(QStringLiteral("/var/lib/apt/lists/"));
    stampDirWatch->addDir(QStringLiteral("/var/lib/apt/lists/partial/"));
    stampDirWatch->addFile(QStringLiteral("/var/lib/update-notifier/updates-available"));
    stampDirWatch->addFile(QStringLiteral("/var/lib/update-notifier/dpkg-run-stamp"));
    connect(stampDirWatch, &KDirWatch::dirty, t, static_cast<void(QTimer::*)()>(&QTimer::start));
//     connect(stampDirWatch, &KDirWatch::dirty, this, [](const QString& dirty){ qDebug() << "dirty path" << dirty;});

    m_updateCheckerProcess = new QProcess(this);
    m_updateCheckerProcess->setProgram(QStringLiteral("/usr/lib/update-notifier/apt-check"));
    connect(m_updateCheckerProcess, static_cast<void(QProcess::*)(int)>(&QProcess::finished), this, &ApplicationNotifier::parseUpdateInfo);

    init();
}

ApplicationNotifier::~ApplicationNotifier() = default;

void ApplicationNotifier::init()
{
    recheckSystemUpdateNeeded();
    distUpgradeEvent();
}

void ApplicationNotifier::distUpgradeEvent()
{
    QString checkerFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("libdiscover/applicationsbackend/releasechecker"));
    if (checkerFile.isEmpty()) {
        qWarning() << "Couldn't find the releasechecker" << checkerFile << QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
        return;
    }
    m_checkerProcess = new QProcess(this);
    connect(m_checkerProcess, static_cast<void (QProcess::*)(int)>(&QProcess::finished), this, &ApplicationNotifier::checkUpgradeFinished);
    m_checkerProcess->start(QStringLiteral("/usr/bin/python3"), QStringList() << checkerFile);
}

void ApplicationNotifier::checkUpgradeFinished(int exitStatus)
{
    if (exitStatus == 0) {
        KNotification *n = KNotification::event(QStringLiteral("DistUpgrade"),
                                                i18n("System update available!"),
                                                i18nc("Notification when a new version of Kubuntu is available",
                                                      "A new version of Kubuntu is available"),
                                                QStringLiteral("system-software-update"),
                                                nullptr,
                                                KNotification::CloseOnTimeout,
                                                QStringLiteral("muonapplicationnotifier"));
        n->setActions(QStringList() << i18n("Upgrade"));
        connect(n, &KNotification::action1Activated, this, &ApplicationNotifier::upgradeActivated);
    }

    m_checkerProcess->deleteLater();
    m_checkerProcess = nullptr;
}

void ApplicationNotifier::upgradeActivated()
{
    const QString kdesu = QFile::decodeName(CMAKE_INSTALL_FULL_LIBEXECDIR_KF5 "/kdesu");
    QProcess::startDetached(kdesu, { QStringLiteral("--"), QStringLiteral("do-release-upgrade"), QStringLiteral("-m"), QStringLiteral("desktop"), QStringLiteral("-f"), QStringLiteral("DistUpgradeViewKDE") });
}

void ApplicationNotifier::recheckSystemUpdateNeeded()
{
    qDebug() << "should recheck..." << m_updateCheckerProcess << m_updateCheckerProcess->state();

    if (m_updateCheckerProcess->state() == QProcess::Running)
        return;
    
    m_updateCheckerProcess->start();
}
    
void ApplicationNotifier::parseUpdateInfo()
{
    if (!m_updateCheckerProcess)
        return;

#warning why does this parse stdout and not use qapt, wtf...
    // Weirdly enough, apt-check gives output on stderr
    QByteArray line = m_updateCheckerProcess->readAllStandardError();
    if (line.isEmpty())
        return;

    // Format updates;security
    int eqpos = line.indexOf(';');

    if (eqpos > 0) {
        QByteArray updatesString = line.left(eqpos);
        QByteArray securityString = line.right(line.size() - eqpos - 1);
        
        int securityUpdates = securityString.toInt();
        setUpdates(updatesString.toInt() - securityUpdates, securityUpdates);
    } else {
        //if the format is wrong consider as up to date
        setUpdates(0, 0);
    }
}

void ApplicationNotifier::setUpdates(int normal, int security)
{
    if (m_normalUpdates != normal || security != m_securityUpdates) {
        m_normalUpdates = normal;
        m_securityUpdates = security;
        emit foundUpdates();
    }
}

bool ApplicationNotifier::isSystemUpToDate() const
{
    return (m_securityUpdates+m_normalUpdates)==0;
}

uint ApplicationNotifier::securityUpdatesCount()
{
    return m_securityUpdates;
}

uint ApplicationNotifier::updatesCount()
{
    return m_normalUpdates;
}

#include "ApplicationNotifier.moc"