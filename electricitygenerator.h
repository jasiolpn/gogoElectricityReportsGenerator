#ifndef ELECTRICITYGENERATOR_H
#define ELECTRICITYGENERATOR_H

#include <QObject>

class QTimer;
class QNetworkReply;
class QNetworkRequest;

class ElectricityGenerator : public QObject
{
    Q_OBJECT
public:
    explicit ElectricityGenerator(const QString &url, int interval);
    virtual ~ElectricityGenerator();

    int id() const;

public slots:
    void init();
    void exit();
    void processReply(QNetworkReply *reply);

signals:
    void sendRequest(const QNetworkRequest &request, const QByteArray &data);

private:
    QByteArray generateRequestData() const;

    bool _isInitiated;
    QTimer *_timer;
    int _id;
    QString _url;
    int _interval;

    static int _nextId;
};

#endif // ELECTRICITYGENERATOR_H
