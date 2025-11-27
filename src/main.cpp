#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "http/http_client.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    HttpClient httpClient;

    engine.loadFromModule("EAutoCheckUI", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
