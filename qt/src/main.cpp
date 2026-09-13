#include "MainWindow.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("GP-16 Editor"));
  QApplication::setOrganizationName(QStringLiteral("gp16-editor"));
  QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
  if (auto* fusion = QStyleFactory::create(QStringLiteral("Fusion")))
    QApplication::setStyle(fusion);

  MainWindow window;
  if (argc > 1)
    window.openDumpFile(QString::fromLocal8Bit(argv[1]));
  window.show();
  return app.exec();
}
