#include <QCoreApplication>
#include <QtNetwork>
#include <QTextStream>
#include <QString>
#include <QThread>
#include <QDebug>
#include "electricitygenerator.h"

#include <signal.h>
#include <unistd.h>

void catchUnixSignals(std::initializer_list<int> quitSignals)
{
    auto handler = [](int sig) -> void
    {
        Q_UNUSED(sig)

        printf("Shut down, bye!\n");

        qApp->exit(0);
    };

    sigset_t blocking_mask;

    sigemptyset(&blocking_mask);

    for (auto sig: quitSignals)
        sigaddset(&blocking_mask, sig);

    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_mask    = blocking_mask;
    sa.sa_flags   = 0;

    for (auto sig : quitSignals)
        sigaction(sig, &sa, nullptr);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QNetworkAccessManager networkMng;

    QList<QThread *> threads;
    QList<ElectricityGenerator *> generators;

    QObject::connect(&a, &QCoreApplication::aboutToQuit, [&]()
    {
        for (ElectricityGenerator *gen: generators)
            QMetaObject::invokeMethod(gen, &ElectricityGenerator::exit, Qt::BlockingQueuedConnection);

        for (QThread *thread: threads)
        {
            thread->quit();
            thread->wait();

            delete thread;
        }
    });

    catchUnixSignals({SIGINT});

    QTextStream out(stdout, QIODevice::WriteOnly), in(stdin, QIODevice::ReadWrite);

    int genNumber = 0;

    do
    {
        out << "How many generators do you want to emulate? (2-20): " << Qt::flush;
        in >> genNumber;

        if (genNumber < 2 || genNumber > 20)
            out << "You have entered wrong number. Try again." << Qt::endl << Qt::endl;
        else
            break;

    } while (true);

    QString url;

    out << "Enter url (default https://localhost:8000): " << Qt::flush;
    in << " " << Qt::flush;
    in >> url;
    in.resetStatus();

    if (url.isEmpty())
        url = "https://localhost:8000";

    int interval = 0;

    out << "Enter request interval [ms] (default 500): " << Qt::flush;
    in << 0 << Qt::flush;
    in >> interval;
    in.resetStatus();

    if (interval == 0)
        interval = 500;

    for (int i = 0; i < genNumber; ++i)
    {
        QThread *thread = new QThread;
        ElectricityGenerator *gen = new ElectricityGenerator(url, interval);

        threads << thread;
        generators << gen;

        QObject::connect(thread, &QThread::finished, gen, &ElectricityGenerator::deleteLater);
        QObject::connect(gen, &ElectricityGenerator::sendRequest, &a, [&, gen](const QNetworkRequest &request, const QByteArray &data)
        {
            QNetworkReply *reply = networkMng.post(request, data);

            QObject::connect(reply, &QNetworkReply::finished, gen, std::bind(&ElectricityGenerator::processReply, gen, reply));
        });

        gen->moveToThread(thread);
        thread->start();

        QMetaObject::invokeMethod(gen, &ElectricityGenerator::init, Qt::BlockingQueuedConnection);
    }

    return a.exec();
}
