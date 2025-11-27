#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "client/http_client.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    client::HttpClient httpClient;

    engine.loadFromModule("GUI", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return QGuiApplication::exec();
}
