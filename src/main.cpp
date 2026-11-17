#include <QApplication>
#include <QStringList>

#include "main_window.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("PdfBookmarkViewer"));
    QApplication::setOrganizationName(QStringLiteral("PdfBookmarkViewer"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    MainWindow window;
    window.show();

    const QStringList arguments = QApplication::arguments();
    if (arguments.size() > 1)
        window.openFile(arguments.at(1));

    return app.exec();
}
