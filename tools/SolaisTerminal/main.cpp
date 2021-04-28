#include <QApplication>
#include <boost/asio.hpp>
#include "QAsioEventDispatcher.h"
#include "MainWindow.h"

boost::asio::io_context ioContext;

int main(int argc, char *argv[]) {

    QApplication::setEventDispatcher(new QAsioEventDispatcher(ioContext));
    QApplication a(argc, argv);

    meta::MainWindow w(ioContext);
    w.setWindowState(Qt::WindowMaximized);
    w.show();

    return a.exec();
}
