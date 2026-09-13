#pragma once

#include <QString>
#include <QWidget>

class Patch;

class PatchDisplayWidget : public QWidget
{
  Q_OBJECT

public:
  explicit PatchDisplayWidget(QWidget* parent = nullptr);

  void setPatch(const Patch* patch);
  void refresh();

  [[nodiscard]] char groupLetter() const;
  [[nodiscard]] int bankDigit() const;
  [[nodiscard]] int numberDigit() const;
  [[nodiscard]] QString lcdLine1() const;
  [[nodiscard]] QString lcdLine2() const;

  [[nodiscard]] QSize sizeHint() const override;
  [[nodiscard]] QSize minimumSizeHint() const override;

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  const Patch* patch_ = nullptr;
};
