#include "electricitygenerator.h"
#include <QTimer>
#include <QtNetwork>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

int ElectricityGenerator::_nextId = 1;

static double generatePower()
{
    quint32 num = QRandomGenerator::global()->bounded(500, 1001);

    return static_cast<double>(num) / 1000.0;
}

ElectricityGenerator::ElectricityGenerator(const QString &url, int interval)
  : QObject(nullptr)
  , _isInitiated(false)
  , _timer(nullptr)
  , _id(_nextId++)
  , _url(url)
  , _interval(interval)
{
}

ElectricityGenerator::~ElectricityGenerator()
{
    --_nextId;

    if (_timer)
        delete _timer;
}

int ElectricityGenerator::id() const
{
    return _id;
}

void ElectricityGenerator::init()
{
    if (_isInitiated)
        return;

    _timer = new QTimer;

    connect(_timer, &QTimer::timeout, [&]()
    {
        _timer->stop();

        QString resource = QString("%1/api/log-entries").arg(_url);
        QUrl url(resource);

        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        emit sendRequest(request, generateRequestData());
    });

    _timer->setInterval(_interval);
    _timer->start();

    _isInitiated = true;
}

void ElectricityGenerator::exit()
{
    if (_timer->isActive())
        _timer->stop();
}

void ElectricityGenerator::processReply(QNetworkReply *reply)
{
    qDebug() << _id << reply->readAll();
    reply->deleteLater();

    _timer->start();
}

QByteArray ElectricityGenerator::generateRequestData() const
{
    QJsonObject obj;

    obj["generatorId"] = _id;
    obj["power"] = generatePower();
    obj["measurementTime"] = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss.zzz");

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
