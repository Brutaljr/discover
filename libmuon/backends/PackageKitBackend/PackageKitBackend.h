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

#ifndef PACKAGEKITBACKEND_H
#define PACKAGEKITBACKEND_H

#include <resources/AbstractResourcesBackend.h>
#include <QVariantList>
#include <QStringList>
#include <PackageKit/packagekit-qt2/Transaction>

class StandardBackendUpdater;
struct ApplicationData
{
    QString pkgname;
    QString id;
    QHash<QString, QString> name;
    QHash<QString, QString> summary;
    QString icon;
    QString url;
    QHash<QString, QStringList> keywords;
    QStringList appcategories;
    QStringList mimetypes;
};

class PackageKitBackend : public AbstractResourcesBackend
{
    Q_OBJECT
    Q_INTERFACES(AbstractResourcesBackend)
    public:
        explicit PackageKitBackend(QObject* parent, const QVariantList& args);
        
        virtual AbstractBackendUpdater* backendUpdater() const;
        virtual AbstractReviewsBackend* reviewsBackend() const;
        
        virtual QVector< AbstractResource* > allResources() const;
        virtual AbstractResource* resourceByPackageName(const QString& name) const;
        virtual QStringList searchPackageName(const QString& searchText);
        virtual int updatesCount() const;
        
        virtual void installApplication(AbstractResource* app);
        virtual void installApplication(AbstractResource* app, AddonList addons);
        virtual void removeApplication(AbstractResource* app);
        virtual void cancelTransaction(AbstractResource* app);
        virtual bool isValid() const { return true; }
        virtual QList<AbstractResource*> upgradeablePackages() const;
        
    public slots:
        void addPackage(PackageKit::Transaction::Info info, const QString &packageId, const QString &summary);
        void removeTransaction(Transaction* t);
        void finished(PackageKit::Transaction::Exit,uint);

    private:
        void populateCache();

        QVector<AbstractResource*> m_packages;
        QHash<QString, ApplicationData> m_appdata;
        QList<Transaction*> m_transactions;
        StandardBackendUpdater* m_updater;
};

#endif // PACKAGEKITBACKEND_H
