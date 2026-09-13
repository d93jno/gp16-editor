#include "MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("GP-16 Editor"));
  QApplication::setOrganizationName(QStringLiteral("gp16-editor"));
  QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

  MainWindow window;
  window.show();
  return app.exec();
}
