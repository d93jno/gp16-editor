#include "PatchDisplayWidget.h"

#include "Patch.h"

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

namespace {

constexpr int kBezelH = 120;
constexpr int kLcdCols = 16;
constexpr int kLcdRows = 2;
constexpr int kGlyphW = 5;
constexpr int kGlyphH = 7;

const QColor kBezel{16, 16, 16};
const QColor kBezelLine{48, 48, 48};
const QColor kLedOff{28, 28, 28};
const QColor kLedA{255, 140, 16};
const QColor kLedB{40, 200, 56};
const QColor kLedOut{255, 36, 36};
const QColor kSegOn{80, 255, 56};
const QColor kSegOff{14, 28, 14};
const QColor kSegWell{4, 4, 4};
const QColor kLcdWell{154, 214, 36};
const QColor kLcdDotOn{22, 40, 12};
const QColor kLcdDotOff{142, 200, 32};
const QColor kLabel{196, 196, 196};
const QColor kCaption{220, 220, 220};
const QColor kSilk{232, 232, 232};

constexpr const char* kBlockALegend[] = {
    "BLOCK-A",
    "1.COMPRESSOR",
    "2.DIST/OVERDRIVE",
    "3.PICKING FILTER",
    "4.PHASER",
    "5.PARAMETRIC EQ.",
    "6.NOISE SUPPRESSOR",
};
constexpr const char* kBlockBLegend[] = {
    "BLOCK-B",
    "1.SHORT DELAY",
    "2.CH/FL/PS/SP",
    "3.AUTO PANPOT",
    "4.TAP DELAY",
    "5.REVERB",
    "6.LINEOUT FILTER",
};

// 5×7 HD44780-style glyphs, column-major, bit 0 = top row. ASCII 32–126.
constexpr std::array<std::uint8_t, 5> kBox{0x7F, 0x41, 0x41, 0x41, 0x7F};

std::array<std::uint8_t, 5> glyph(char ch)
{
  switch (static_cast<unsigned char>(ch)) {
    case ' ': return {0x00, 0x00, 0x00, 0x00, 0x00};
    case '!': return {0x00, 0x00, 0x5F, 0x00, 0x00};
    case '"': return {0x00, 0x07, 0x00, 0x07, 0x00};
    case '#': return {0x14, 0x7F, 0x14, 0x7F, 0x14};
    case '$': return {0x24, 0x2A, 0x7F, 0x2A, 0x12};
    case '%': return {0x23, 0x13, 0x08, 0x64, 0x62};
    case '&': return {0x36, 0x49, 0x55, 0x22, 0x50};
    case '\'': return {0x00, 0x05, 0x03, 0x00, 0x00};
    case '(': return {0x00, 0x1C, 0x22, 0x41, 0x00};
    case ')': return {0x00, 0x41, 0x22, 0x1C, 0x00};
    case '*': return {0x14, 0x08, 0x3E, 0x08, 0x14};
    case '+': return {0x08, 0x08, 0x3E, 0x08, 0x08};
    case ',': return {0x00, 0x50, 0x30, 0x00, 0x00};
    case '-': return {0x08, 0x08, 0x08, 0x08, 0x08};
    case '.': return {0x00, 0x60, 0x60, 0x00, 0x00};
    case '/': return {0x20, 0x10, 0x08, 0x04, 0x02};
    case '0': return {0x3E, 0x51, 0x49, 0x45, 0x3E};
    case '1': return {0x00, 0x42, 0x7F, 0x40, 0x00};
    case '2': return {0x42, 0x61, 0x51, 0x49, 0x46};
    case '3': return {0x21, 0x41, 0x45, 0x4B, 0x31};
    case '4': return {0x18, 0x14, 0x12, 0x7F, 0x10};
    case '5': return {0x27, 0x45, 0x45, 0x45, 0x39};
    case '6': return {0x3C, 0x4A, 0x49, 0x49, 0x30};
    case '7': return {0x01, 0x71, 0x09, 0x05, 0x03};
    case '8': return {0x36, 0x49, 0x49, 0x49, 0x36};
    case '9': return {0x06, 0x49, 0x49, 0x29, 0x1E};
    case ':': return {0x00, 0x36, 0x36, 0x00, 0x00};
    case ';': return {0x00, 0x56, 0x36, 0x00, 0x00};
    case '<': return {0x08, 0x14, 0x22, 0x41, 0x00};
    case '=': return {0x14, 0x14, 0x14, 0x14, 0x14};
    case '>': return {0x00, 0x41, 0x22, 0x14, 0x08};
    case '?': return {0x02, 0x01, 0x51, 0x09, 0x06};
    case '@': return {0x32, 0x49, 0x79, 0x41, 0x3E};
    case 'A': return {0x7E, 0x11, 0x11, 0x11, 0x7E};
    case 'B': return {0x7F, 0x49, 0x49, 0x49, 0x36};
    case 'C': return {0x3E, 0x41, 0x41, 0x41, 0x22};
    case 'D': return {0x7F, 0x41, 0x41, 0x22, 0x1C};
    case 'E': return {0x7F, 0x49, 0x49, 0x49, 0x41};
    case 'F': return {0x7F, 0x09, 0x09, 0x09, 0x01};
    case 'G': return {0x3E, 0x41, 0x49, 0x49, 0x7A};
    case 'H': return {0x7F, 0x08, 0x08, 0x08, 0x7F};
    case 'I': return {0x00, 0x41, 0x7F, 0x41, 0x00};
    case 'J': return {0x20, 0x40, 0x41, 0x3F, 0x01};
    case 'K': return {0x7F, 0x08, 0x14, 0x22, 0x41};
    case 'L': return {0x7F, 0x40, 0x40, 0x40, 0x40};
    case 'M': return {0x7F, 0x02, 0x0C, 0x02, 0x7F};
    case 'N': return {0x7F, 0x04, 0x08, 0x10, 0x7F};
    case 'O': return {0x3E, 0x41, 0x41, 0x41, 0x3E};
    case 'P': return {0x7F, 0x09, 0x09, 0x09, 0x06};
    case 'Q': return {0x3E, 0x41, 0x51, 0x21, 0x5E};
    case 'R': return {0x7F, 0x09, 0x19, 0x29, 0x46};
    case 'S': return {0x46, 0x49, 0x49, 0x49, 0x31};
    case 'T': return {0x01, 0x01, 0x7F, 0x01, 0x01};
    case 'U': return {0x3F, 0x40, 0x40, 0x40, 0x3F};
    case 'V': return {0x1F, 0x20, 0x40, 0x20, 0x1F};
    case 'W': return {0x3F, 0x40, 0x38, 0x40, 0x3F};
    case 'X': return {0x63, 0x14, 0x08, 0x14, 0x63};
    case 'Y': return {0x07, 0x08, 0x70, 0x08, 0x07};
    case 'Z': return {0x61, 0x51, 0x49, 0x45, 0x43};
    case '[': return {0x00, 0x7F, 0x41, 0x41, 0x00};
    case '\\': return {0x02, 0x04, 0x08, 0x10, 0x20};
    case ']': return {0x00, 0x41, 0x41, 0x7F, 0x00};
    case '^': return {0x04, 0x02, 0x01, 0x02, 0x04};
    case '_': return {0x40, 0x40, 0x40, 0x40, 0x40};
    case '`': return {0x00, 0x01, 0x02, 0x04, 0x00};
    case 'a': return {0x20, 0x54, 0x54, 0x54, 0x78};
    case 'b': return {0x7F, 0x48, 0x44, 0x44, 0x38};
    case 'c': return {0x38, 0x44, 0x44, 0x44, 0x20};
    case 'd': return {0x38, 0x44, 0x44, 0x48, 0x7F};
    case 'e': return {0x38, 0x54, 0x54, 0x54, 0x18};
    case 'f': return {0x08, 0x7E, 0x09, 0x01, 0x02};
    case 'g': return {0x08, 0x54, 0x54, 0x54, 0x3C};
    case 'h': return {0x7F, 0x08, 0x04, 0x04, 0x78};
    case 'i': return {0x00, 0x44, 0x7D, 0x40, 0x00};
    case 'j': return {0x20, 0x40, 0x44, 0x3D, 0x00};
    case 'k': return {0x7F, 0x10, 0x28, 0x44, 0x00};
    case 'l': return {0x00, 0x41, 0x7F, 0x40, 0x00};
    case 'm': return {0x7C, 0x04, 0x18, 0x04, 0x78};
    case 'n': return {0x7C, 0x08, 0x04, 0x04, 0x78};
    case 'o': return {0x38, 0x44, 0x44, 0x44, 0x38};
    case 'p': return {0x7C, 0x14, 0x14, 0x14, 0x08};
    case 'q': return {0x08, 0x14, 0x14, 0x18, 0x7C};
    case 'r': return {0x7C, 0x08, 0x04, 0x04, 0x08};
    case 's': return {0x48, 0x54, 0x54, 0x54, 0x20};
    case 't': return {0x04, 0x3F, 0x44, 0x40, 0x20};
    case 'u': return {0x3C, 0x40, 0x40, 0x20, 0x7C};
    case 'v': return {0x1C, 0x20, 0x40, 0x20, 0x1C};
    case 'w': return {0x3C, 0x40, 0x30, 0x40, 0x3C};
    case 'x': return {0x44, 0x28, 0x10, 0x28, 0x44};
    case 'y': return {0x0C, 0x50, 0x50, 0x50, 0x3C};
    case 'z': return {0x44, 0x64, 0x54, 0x4C, 0x44};
    case '{': return {0x00, 0x08, 0x36, 0x41, 0x00};
    case '|': return {0x00, 0x00, 0x7F, 0x00, 0x00};
    case '}': return {0x00, 0x41, 0x36, 0x08, 0x00};
    case '~': return {0x08, 0x04, 0x08, 0x10, 0x08};
    default: return kBox;
  }
}

// Standard 7-seg bits: 0=a 1=b 2=c 3=d 4=e 5=f 6=g
constexpr std::uint8_t kDigitSeg[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

void drawLed(QPainter& p, const QRectF& r, const QColor& on, bool lit)
{
  p.setPen(Qt::NoPen);
  p.setBrush(lit ? on : kLedOff);
  p.drawEllipse(r);
  if (lit) {
    QColor glow = on;
    glow.setAlpha(80);
    p.setBrush(glow);
    p.drawEllipse(r.adjusted(-2, -2, 2, 2));
    p.setBrush(QColor(255, 255, 255, 90));
    p.drawEllipse(QRectF(r.x() + r.width() * 0.22, r.y() + r.height() * 0.18,
                         r.width() * 0.35, r.height() * 0.28));
  }
}

void drawDigit(QPainter& p, const QRectF& box, int digit)
{
  const std::uint8_t mask = (digit >= 0 && digit <= 9) ? kDigitSeg[digit] : 0;
  const qreal t = box.height() * 0.14;
  const qreal n = t * 0.5;
  const qreal x = box.x();
  const qreal y = box.y();
  const qreal w = box.width();
  const qreal h = box.height();
  const qreal mid = y + h * 0.5 - t * 0.5;

  auto paint = [&](int bit, const QPainterPath& path) {
    p.fillPath(path, (mask & (1u << bit)) ? kSegOn : kSegOff);
  };

  auto hHex = [&](qreal sx, qreal sy, qreal sw) {
    QPainterPath path;
    path.moveTo(sx + n, sy);
    path.lineTo(sx + sw - n, sy);
    path.lineTo(sx + sw, sy + t * 0.5);
    path.lineTo(sx + sw - n, sy + t);
    path.lineTo(sx + n, sy + t);
    path.lineTo(sx, sy + t * 0.5);
    path.closeSubpath();
    return path;
  };

  auto vHex = [&](qreal sx, qreal sy, qreal sh) {
    QPainterPath path;
    path.moveTo(sx + t * 0.5, sy);
    path.lineTo(sx + t, sy + n);
    path.lineTo(sx + t, sy + sh - n);
    path.lineTo(sx + t * 0.5, sy + sh);
    path.lineTo(sx, sy + sh - n);
    path.lineTo(sx, sy + n);
    path.closeSubpath();
    return path;
  };

  const qreal gap = std::max(1.0, t * 0.1);
  const qreal hx = x + n + gap;
  const qreal hw = w - 2 * n - 2 * gap;
  paint(0, hHex(hx, y, hw));
  paint(6, hHex(hx, mid, hw));
  paint(3, hHex(hx, y + h - t, hw));

  const qreal vTop = y + n + gap;
  const qreal vUpperH = mid + t * 0.5 - vTop - gap;
  const qreal vLowerY = mid + t * 0.5 + gap;
  const qreal vLowerH = y + h - n - gap - vLowerY;
  paint(5, vHex(x, vTop, vUpperH));
  paint(1, vHex(x + w - t, vTop, vUpperH));
  paint(4, vHex(x, vLowerY, vLowerH));
  paint(2, vHex(x + w - t, vLowerY, vLowerH));
}

void drawLcdChar(QPainter& p, const QRectF& cell, char ch, qreal dot)
{
  const auto cols = glyph(ch);
  const qreal gap = std::max(0.5, dot * 0.18);
  for (int cx = 0; cx < kGlyphW; ++cx) {
    const std::uint8_t col = cols[static_cast<std::size_t>(cx)];
    for (int ry = 0; ry < kGlyphH; ++ry) {
      const bool on = (col & (1u << ry)) != 0;
      p.setBrush(on ? kLcdDotOn : kLcdDotOff);
      p.drawRect(QRectF(cell.x() + cx * (dot + gap),
                        cell.y() + ry * (dot + gap),
                        dot, dot));
    }
  }
}

} // namespace

PatchDisplayWidget::PatchDisplayWidget(QWidget* parent)
    : QWidget(parent)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setMinimumHeight(kBezelH);
}

void PatchDisplayWidget::setPatch(const Patch* patch)
{
  patch_ = patch;
  update();
}

void PatchDisplayWidget::refresh()
{
  update();
}

char PatchDisplayWidget::groupLetter() const
{
  if (!patch_ || !patch_->isPresent())
    return '\0';
  return Patch::groupLetterFor(patch_->index());
}

int PatchDisplayWidget::bankDigit() const
{
  if (!patch_ || !patch_->isPresent())
    return 0;
  return Patch::bankDigitFor(patch_->index());
}

int PatchDisplayWidget::numberDigit() const
{
  if (!patch_ || !patch_->isPresent())
    return 0;
  return Patch::numberDigitFor(patch_->index());
}

QString PatchDisplayWidget::lcdLine1() const
{
  return QString::fromStdString(patch_ ? patch_->playModeLcdLine1() : std::string(16, ' '));
}

QString PatchDisplayWidget::lcdLine2() const
{
  return QString::fromStdString(patch_ ? patch_->playModeLcdLine2() : std::string(16, ' '));
}

QSize PatchDisplayWidget::sizeHint() const
{
  return {760, kBezelH};
}

QSize PatchDisplayWidget::minimumSizeHint() const
{
  return {640, kBezelH};
}

void PatchDisplayWidget::paintEvent(QPaintEvent*)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);
  const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

  p.setPen(kBezelLine);
  p.setBrush(kBezel);
  p.drawRoundedRect(r, 2, 2);

  const qreal pad = 6;
  const qreal y = r.y() + pad;
  const qreal innerH = r.height() - pad * 2;
  qreal x = r.x() + pad;

  QFont caption = font();
  caption.setPixelSize(std::max(8, static_cast<int>(innerH * 0.13)));
  caption.setBold(true);
  caption.setLetterSpacing(QFont::AbsoluteSpacing, 0.4);
  QFont letter = caption;
  letter.setPixelSize(std::max(10, static_cast<int>(innerH * 0.16)));

  auto drawLedColumn = [&](const QString& title, const QString& top, const QString& bot,
                           const QColor& topOn, const QColor& botOn, bool topLit, bool botLit,
                           qreal colW) {
    p.setFont(caption);
    p.setPen(kLabel);
    p.drawText(QRectF(x, y, colW, innerH * 0.22), Qt::AlignHCenter | Qt::AlignBottom, title);
    const qreal led = std::clamp(innerH * 0.16, 7.0, 11.0);
    const QRectF ledTop(x + colW * 0.18, y + innerH * 0.32, led, led);
    const QRectF ledBot(x + colW * 0.18, y + innerH * 0.62, led, led);
    drawLed(p, ledTop, topOn, topLit);
    drawLed(p, ledBot, botOn, botLit);
    p.setFont(letter);
    p.setPen(kCaption);
    p.drawText(QRectF(ledTop.right() + 3, ledTop.y() - 2, colW * 0.5, led + 4),
               Qt::AlignVCenter | Qt::AlignLeft, top);
    p.drawText(QRectF(ledBot.right() + 3, ledBot.y() - 2, colW * 0.5, led + 4),
               Qt::AlignVCenter | Qt::AlignLeft, bot);
    x += colW;
  };

  const char group = groupLetter();
  int outCh = -1;
  if (patch_ && patch_->isPresent())
    outCh = static_cast<int>(patch_->byteAt(0x63) & 0x03);

  drawLedColumn(QStringLiteral("GROUP"), QStringLiteral("A"), QStringLiteral("B"),
                kLedA, kLedB, group == 'A', group == 'B', 52);
  drawLedColumn(QStringLiteral("OUT CH"), QStringLiteral("1"), QStringLiteral("2"),
                kLedOut, kLedOut, outCh == 0 || outCh == 2, outCh == 1 || outCh == 2, 52);

  // Bank / Number 7-seg (smaller than the LEDs on the 1U)
  const qreal digitH = innerH * 0.52;
  const qreal digitW = digitH * 0.50;
  const qreal digitGap = 6;
  const qreal digitsW = digitW * 2 + digitGap;
  const qreal segWellW = digitsW + 18;
  const QRectF segWell(x, y, segWellW, innerH);
  p.setPen(Qt::NoPen);
  p.setBrush(kSegWell);
  p.drawRect(segWell);
  p.setFont(caption);
  p.setPen(kLabel);
  const qreal digitX = x + (segWellW - digitsW) * 0.5;
  const qreal digitY = y + innerH * 0.28;
  p.drawText(QRectF(digitX, y, digitW, innerH * 0.22),
             Qt::AlignHCenter | Qt::AlignBottom, QStringLiteral("BANK"));
  p.drawText(QRectF(digitX + digitW + digitGap, y, digitW, innerH * 0.22),
             Qt::AlignHCenter | Qt::AlignBottom, QStringLiteral("NUMBER"));
  drawDigit(p, QRectF(digitX, digitY, digitW, digitH), bankDigit() > 0 ? bankDigit() : -1);
  drawDigit(p, QRectF(digitX + digitW + digitGap, digitY, digitW, digitH),
            numberDigit() > 0 ? numberDigit() : -1);
  x = segWell.right() + 8;

  // Silkscreen legend between the 7-seg and the LCD
  QFont silk = font();
  silk.setPixelSize(std::max(7, static_cast<int>(innerH / 15.5)));
  silk.setBold(false);
  silk.setStretch(QFont::SemiCondensed);
  QFont silkHead = silk;
  silkHead.setBold(true);
  const qreal legendW = 132;
  const int legendRows = 14;
  const qreal rowH = innerH / legendRows;
  auto drawLegend = [&](const char* const* lines, int count, qreal row) {
    for (int i = 0; i < count; ++i) {
      const bool head = i == 0;
      p.setFont(head ? silkHead : silk);
      p.setPen(kSilk);
      p.drawText(QRectF(x, y + (row + i) * rowH, legendW, rowH),
                 Qt::AlignVCenter | Qt::AlignLeft,
                 QString::fromLatin1(lines[i]));
    }
  };
  drawLegend(kBlockALegend, 7, 0);
  drawLegend(kBlockBLegend, 7, 7);
  x += legendW + 8;

  // LCD: black bezel, green glass, BLOCK-A / BLOCK-B under the glass
  const qreal lcdRight = r.right() - pad;
  const qreal lcdW = std::max(160.0, lcdRight - x);
  const QRectF frame(x, y, lcdW, innerH);
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(8, 8, 8));
  p.drawRect(frame);
  const qreal captionH = innerH * 0.18;
  const QRectF well = frame.adjusted(5, 4, -5, -(captionH + 2));
  p.setBrush(kLcdWell);
  p.drawRect(well);

  const qreal wellPadX = 6;
  const qreal wellPadY = 4;
  const qreal usableW = well.width() - wellPadX * 2;
  const qreal usableH = well.height() - wellPadY * 2;
  const qreal cellGapX = 2;
  const qreal cellGapY = 3;
  const qreal cellW = (usableW - cellGapX * (kLcdCols - 1)) / kLcdCols;
  const qreal cellH = (usableH - cellGapY) / kLcdRows;
  const qreal dot = std::max(1.0, std::min(cellW / (kGlyphW + 0.55), cellH / (kGlyphH + 0.55)));

  p.setRenderHint(QPainter::Antialiasing, false);
  p.setPen(Qt::NoPen);
  const QString line1 = lcdLine1();
  const QString line2 = lcdLine2();
  for (int row = 0; row < kLcdRows; ++row) {
    const QString& line = row == 0 ? line1 : line2;
    for (int col = 0; col < kLcdCols; ++col) {
      const QChar qc = col < line.size() ? line[col] : QChar(u' ');
      const QRectF cell(well.x() + wellPadX + col * (cellW + cellGapX),
                        well.y() + wellPadY + row * (cellH + cellGapY),
                        cellW, cellH);
      drawLcdChar(p, cell, qc.toLatin1(), dot);
    }
  }

  p.setRenderHint(QPainter::Antialiasing, true);
  p.setFont(caption);
  p.setPen(kLabel);
  const QRectF under(frame.x() + 5, well.bottom() + 1, frame.width() - 10, captionH);
  const qreal half = under.width() * 0.5;
  p.drawText(QRectF(under.x(), under.y(), half, under.height()),
             Qt::AlignHCenter | Qt::AlignVCenter, QStringLiteral("BLOCK-A"));
  p.drawText(QRectF(under.x() + half, under.y(), half, under.height()),
             Qt::AlignHCenter | Qt::AlignVCenter, QStringLiteral("BLOCK-B"));
}
