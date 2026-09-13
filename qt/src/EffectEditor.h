#pragma once

#include "EffectSpecs.h"

#include <QString>
#include <QWidget>

#include <array>
#include <vector>

class Patch;
class ParamRow;
class QLabel;
class QStackedWidget;

class EffectEditor : public QWidget
{
  Q_OBJECT

public:
  explicit EffectEditor(QWidget* parent = nullptr);

  void setPatch(Patch* patch);
  void setIdentity(int identity);
  void refresh();

  [[nodiscard]] int identity() const { return identity_; }
  [[nodiscard]] EffectKind currentKind() const { return kind_; }
  [[nodiscard]] QString titleText() const;
  [[nodiscard]] int paramCount() const;
  [[nodiscard]] QString paramLabel(int index) const;
  [[nodiscard]] int paramOffset(int index) const;
  [[nodiscard]] int paramRawValue(int index) const;
  void setParamRawValue(int index, int raw);

signals:
  void parameterEdited(int offset, int byteWidth, int value);

private:
  void rebuildPages();
  QWidget* buildPage(const EffectSpec& spec);
  void reload();
  void loadCurrentPage();
  [[nodiscard]] const std::vector<ParamRow*>& currentRows() const;
  [[nodiscard]] ParamRow* rowAt(int index) const;

  Patch* patch_ = nullptr;
  int identity_ = -1;
  EffectKind kind_ = EffectKind::Count;

  QLabel* title_ = nullptr;
  QStackedWidget* stack_ = nullptr;
  std::array<std::vector<ParamRow*>, static_cast<std::size_t>(EffectKind::Count)> rows_{};
};
