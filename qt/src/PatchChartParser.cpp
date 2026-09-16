#include "PatchChartParser.h"

#include "RolandSysex.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace {

struct LabelTarget {
  const ParamSpec* spec = nullptr;
  enum class Special { None, BalanceE, BalanceD } special = Special::None;
};

std::string trim(std::string_view s)
{
  std::size_t a = 0;
  while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a])))
    ++a;
  std::size_t b = s.size();
  while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1])))
    --b;
  return std::string(s.substr(a, b - a));
}

std::string normalizeKey(std::string_view s)
{
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    if (std::isalnum(static_cast<unsigned char>(c)))
      out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  return out;
}

bool iequals(std::string_view a, std::string_view b)
{
  if (a.size() != b.size())
    return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(a[i]))
        != std::tolower(static_cast<unsigned char>(b[i])))
      return false;
  }
  return true;
}

std::vector<std::string> tokenize(std::string_view line)
{
  std::vector<std::string> tok;
  std::string cur;
  auto flush = [&] {
    if (!cur.empty()) {
      tok.push_back(cur);
      cur.clear();
    }
  };
  for (char c : line) {
    if (c == ' ' || c == '\t' || c == '\r' || c == ':')
      flush();
    else
      cur += c;
  }
  flush();
  return tok;
}

std::string joinFrom(const std::vector<std::string>& tok, std::size_t from)
{
  std::string s;
  for (std::size_t i = from; i < tok.size(); ++i) {
    if (!s.empty())
      s += ' ';
    s += tok[i];
  }
  return s;
}

bool isSeparator(std::string_view line)
{
  const std::string t = trim(line);
  if (t.size() < 8)
    return false;
  return std::all_of(t.begin(), t.end(), [](char c) { return c == '-' || c == '='; });
}

bool isBanner(std::string_view line)
{
  const std::string t = trim(line);
  return !t.empty() && std::all_of(t.begin(), t.end(), [](char c) {
    return c == '*' || std::isspace(static_cast<unsigned char>(c));
  });
}

bool isSlotToken(std::string_view t, int& identity)
{
  if (t.size() != 3)
    return false;
  if ((t[0] != 'A' && t[0] != 'B') || t[1] != '-')
    return false;
  if (t[2] < '1' || t[2] > '6')
    return false;
  const int local = t[2] - '1';
  identity = (t[0] == 'A') ? local : local + 6;
  return true;
}

int variantFromToken(std::string_view t)
{
  if (t.empty())
    return -1;
  const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(t[0])));
  if (c < 'a' || c > 'd')
    return -1;
  if (t.size() == 1 || (t.size() == 2 && t[1] == '.'))
    return c - 'a';
  return -1;
}

bool looksLikeEffectName(const std::vector<std::string>& tok, std::size_t from)
{
  const std::string n = normalizeKey(joinFrom(tok, from));
  return n.find("DISTORTION") != std::string::npos || n.find("OVERDRIVE") != std::string::npos
         || n.find("CHORUS") != std::string::npos || n.find("FLANGER") != std::string::npos
         || n.find("PITCH") != std::string::npos || n.find("SPACE") != std::string::npos;
}

const char* slotName(int identity)
{
  switch (identity) {
    case 0: return "A-1";
    case 1: return "A-2";
    case 2: return "A-3";
    case 3: return "A-4";
    case 4: return "A-5";
    case 5: return "A-6";
    case 6: return "B-1";
    case 7: return "B-2";
    case 8: return "B-3";
    case 9: return "B-4";
    case 10: return "B-5";
    case 11: return "B-6";
    default: return "?";
  }
}

bool isOnOffPattern(std::string_view t)
{
  if (t.size() != 6)
    return false;
  for (char c : t) {
    if (c != '*' && (c < '1' || c > '6'))
      return false;
  }
  return true;
}

int decaySecondsToRaw(double sec)
{
  if (sec <= 5.0)
    return std::clamp(static_cast<int>(std::lround((sec - 0.5) / 0.1)), 0, 45);
  return std::clamp(46 + static_cast<int>(std::lround((sec - 5.5) / 0.5)), 46, 75);
}

int cutoffHzToRaw(double hz)
{
  ParamSpec spec;
  spec.min = 0;
  spec.max = 199;
  spec.transform = DisplayTransform::FreqLog;
  spec.displayMin = 500;
  spec.displayMax = 8000;
  return displayToRaw(spec, hz);
}

int matchCombo(const ParamSpec& spec, std::string_view value)
{
  if (!spec.comboItems || spec.comboCount <= 0)
    return -1;
  const std::string norm = normalizeKey(value);
  if (norm.empty())
    return -1;
  int prefixHit = -1;
  int prefixCount = 0;
  for (int i = 0; i < spec.comboCount; ++i) {
    const char* item = spec.comboItems[i];
    if (iequals(item, value))
      return i;
    const std::string inorm = normalizeKey(item);
    if (inorm == norm)
      return i;
    if (inorm.find(norm) == 0 || norm.find(inorm) == 0) {
      prefixHit = i;
      ++prefixCount;
    }
  }
  return prefixCount == 1 ? prefixHit : -1;
}

struct ExtractedValue {
  bool hasNumber = false;
  bool thru = false;
  bool on = false;
  bool off = false;
  double number = 0;
  bool khz = false;
  bool hz = false;
  bool sec = false;
  std::string text;
};

ExtractedValue extractValue(std::string_view raw)
{
  ExtractedValue v;
  v.text = trim(raw);
  const std::string upper = [](std::string s) {
    for (char& c : s)
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
  }(v.text);

  if (upper.find("THRU") != std::string::npos) {
    v.thru = true;
    return v;
  }
  if (upper == "ON") {
    v.on = true;
    return v;
  }
  if (upper == "OFF") {
    v.off = true;
    return v;
  }

  std::size_t i = 0;
  while (i < v.text.size() && std::isspace(static_cast<unsigned char>(v.text[i])))
    ++i;
  if (i < v.text.size() && (v.text[i] == '+' || v.text[i] == '-'))
    ++i;
  const std::size_t numStart = (i > 0 && (v.text[i - 1] == '+' || v.text[i - 1] == '-')) ? i - 1 : i;
  bool sawDigit = false;
  bool sawDot = false;
  while (i < v.text.size()) {
    const char c = v.text[i];
    if (std::isdigit(static_cast<unsigned char>(c))) {
      sawDigit = true;
      ++i;
    } else if (c == '.' && !sawDot) {
      sawDot = true;
      ++i;
    } else {
      break;
    }
  }
  if (sawDigit) {
    try {
      v.number = std::stod(v.text.substr(numStart, i - numStart));
      v.hasNumber = true;
    } catch (...) {
      v.hasNumber = false;
    }
  }

  std::string unit = normalizeKey(v.text.substr(i));
  if (unit == "KHZ") {
    v.khz = true;
    v.hz = true;
    if (v.hasNumber)
      v.number *= 1000.0;
  } else if (unit == "HZ") {
    v.hz = true;
  } else if (unit == "SEC" || unit == "S") {
    v.sec = true;
  }
  return v;
}

bool looksPopulated(const ExtractedValue& v)
{
  return v.hasNumber || v.thru || v.on || v.off || !v.text.empty();
}

void putField(ParsedSlot& slot, const ParamSpec& spec, int raw)
{
  for (auto& f : slot.fields) {
    if (f.offset == spec.offset) {
      f.raw = raw;
      return;
    }
  }
  slot.fields.push_back({spec.offset, spec.byteWidth, raw, spec.label, spec.group});
  slot.present = true;
}

void addAlias(std::unordered_map<std::string, LabelTarget>& map, const char* key,
              const ParamSpec* spec, LabelTarget::Special special = LabelTarget::Special::None)
{
  map.insert_or_assign(key, LabelTarget{spec, special});
}

const ParamSpec* specByLabel(EffectKind kind, const char* label, const char* group = nullptr)
{
  const auto& effect = specFor(kind);
  for (const auto& p : effect.params) {
    if (std::string_view(p.label) != label)
      continue;
    if (group == nullptr && p.group == nullptr)
      return &p;
    if (group && p.group && std::string_view(p.group) == group)
      return &p;
  }
  return nullptr;
}

std::unordered_map<std::string, LabelTarget> labelsFor(EffectKind kind)
{
  std::unordered_map<std::string, LabelTarget> map;
  const auto& effect = specFor(kind);
  std::unordered_map<std::string, int> bareCount;
  for (const auto& p : effect.params)
    ++bareCount[normalizeKey(p.label)];

  for (const auto& p : effect.params) {
    const std::string bare = normalizeKey(p.label);
    if (bareCount[bare] == 1)
      map[bare] = LabelTarget{&p, LabelTarget::Special::None};
    if (p.group)
      map[normalizeKey(std::string(p.group) + p.label)] = LabelTarget{&p, LabelTarget::Special::None};
  }

  auto add = [&](const char* key, const char* label, const char* group = nullptr,
                 LabelTarget::Special special = LabelTarget::Special::None) {
    if (const ParamSpec* p = specByLabel(kind, label, group))
      addAlias(map, key, p, special);
  };

  add("DTIME", "Time");
  add("PDELAY", "Pre-Delay");
  add("PREDELAY", "Pre-Delay");
  add("ELEVEL", "E.Level");
  add("FBACK", "Feedback");
  add("CUTOFFFREQ", "Cutoff");
  add("LFOSTEP", "LFO Step");
  add("UPDOWN", "Up/Down");
  add("MIDDLE", "Level");

  add("HIFREQ", "Freq", "High");
  add("HILEVEL", "Level", "High");
  add("HMFREQ", "Freq", "High Mid");
  add("HMQ", "Q", "High Mid");
  add("HMLEVEL", "Level", "High Mid");
  add("LMFREQ", "Freq", "Low Mid");
  add("LMQ", "Q", "Low Mid");
  add("LMLEVEL", "Level", "Low Mid");
  add("LOFREQ", "Freq", "Low");
  add("LOLEVEL", "Level", "Low");
  add("OUTLEVEL", "Level", "Out");

  add("CTAP", "Time", "Center");
  add("LTAP", "Time", "Left");
  add("RTAP", "Time", "Right");
  add("CLEVEL", "Level", "Center");
  add("LLEVEL", "Level", "Left");
  add("RLEVEL", "Level", "Right");

  add("BALE", "Balance", nullptr, LabelTarget::Special::BalanceE);
  add("BALD", "Balance", nullptr, LabelTarget::Special::BalanceD);
  add("BALEFFECT", "Balance", nullptr, LabelTarget::Special::BalanceE);
  add("BALDIRECT", "Balance", nullptr, LabelTarget::Special::BalanceD);

  return map;
}

bool consumeLabel(const std::vector<std::string>& tok,
                  const std::unordered_map<std::string, LabelTarget>& labels, LabelTarget& out,
                  std::size_t& valueFrom)
{
  LabelTarget best;
  std::size_t bestUsed = 0;
  std::size_t bestKeyLen = 0;
  for (const auto& [key, target] : labels) {
    std::string acc;
    for (std::size_t used = 1; used <= tok.size(); ++used) {
      acc += normalizeKey(tok[used - 1]);
      if (acc == key) {
        if (key.size() > bestKeyLen) {
          best = target;
          bestUsed = used;
          bestKeyLen = key.size();
        }
        break;
      }
      if (key.find(acc) != 0)
        break;
    }
  }
  if (bestKeyLen == 0)
    return false;
  out = best;
  valueFrom = bestUsed;
  return true;
}

int convertToRaw(const ParamSpec& spec, const ExtractedValue& v, std::vector<std::string>& warnings,
                 const std::string& where)
{
  if (v.thru)
    return spec.max;

  if (spec.type == ParamType::Checkbox) {
    if (v.on)
      return spec.max;
    if (v.off)
      return spec.min;
  }

  if (spec.type == ParamType::Combo) {
    const int idx = matchCombo(spec, v.text);
    if (idx >= 0)
      return idx;
    warnings.push_back(where + ": could not match combo value '" + v.text + "' for " + spec.label);
    return spec.min;
  }

  if (!v.hasNumber) {
    warnings.push_back(where + ": no numeric value for " + spec.label);
    return spec.min;
  }

  double display = v.number;
  int raw = 0;
  const bool isCutoff = spec.offset == 0x4F || spec.offset == 0x53;
  const bool isDecay = spec.offset == 0x51;

  if (isDecay && (v.sec || display < 50.0)) {
    raw = decaySecondsToRaw(display);
  } else if (isCutoff && v.hz) {
    raw = cutoffHzToRaw(display);
  } else {
    raw = displayToRaw(spec, display);
  }

  int unclamped = raw;
  if (!isDecay && !isCutoff && spec.transform == DisplayTransform::Raw)
    unclamped = static_cast<int>(std::lround(display));
  if (unclamped < spec.min || unclamped > spec.max) {
    warnings.push_back(where + ": " + spec.label + " value outside " + std::to_string(spec.min)
                       + "–" + std::to_string(spec.max));
  }
  return std::clamp(raw, spec.min, spec.max);
}

void applyVariant(ParsedChart& chart, int identity, int variant)
{
  auto& slot = chart.effectSlots[static_cast<std::size_t>(identity)];
  if (identity == 1) {
    const bool dist = variant == 0;
    slot.kind = dist ? EffectKind::Distortion : EffectKind::Overdrive;
    chart.isDistortion = dist;
  } else if (identity == 7) {
    chart.blockB2Mode = variant;
    slot.kind = kindForSlot(7, variant, true);
  }
}

void parseSequence(ParsedChart& chart, char block, const std::string& digits,
                   std::vector<std::string>& warnings)
{
  if (digits.size() != 6) {
    warnings.push_back(std::string("SEQUENCE BLOCK ") + block + " is not 6 digits");
    return;
  }
  std::array<int, 6> order{};
  std::array<bool, 6> seen{};
  for (int i = 0; i < 6; ++i) {
    if (digits[static_cast<std::size_t>(i)] < '1' || digits[static_cast<std::size_t>(i)] > '6') {
      warnings.push_back(std::string("SEQUENCE BLOCK ") + block + " has invalid digit");
      return;
    }
    const int local = digits[static_cast<std::size_t>(i)] - '1';
    if (seen[static_cast<std::size_t>(local)]) {
      warnings.push_back(std::string("SEQUENCE BLOCK ") + block + " repeats a slot");
      return;
    }
    seen[static_cast<std::size_t>(local)] = true;
    order[static_cast<std::size_t>(i)] = (block == 'A') ? local : local + 6;
  }
  if (block == 'A')
    chart.blockAOrder = order;
  else
    chart.blockBOrder = order;
}

void parseOnOff(ParsedChart& chart, char block, const std::string& pattern)
{
  const auto& order = (block == 'A') ? chart.blockAOrder : chart.blockBOrder;
  for (int i = 0; i < 6; ++i) {
    const int identity = order[static_cast<std::size_t>(i)];
    chart.summaryEnabled[static_cast<std::size_t>(identity)] =
        pattern[static_cast<std::size_t>(i)] != '*';
  }
  if (block == 'A')
    chart.sawBlockAOnOff = true;
  else
    chart.sawBlockBOnOff = true;
}

bool parseProgramChange(ParsedChart& chart, const std::vector<std::string>& tok)
{
  if (tok.size() < 3)
    return false;
  if (!iequals(tok[0], "PROGRAM") || !iequals(tok[1], "CHANGE"))
    return false;
  chart.programChangeRaw = joinFrom(tok, 0);
  for (std::size_t i = 2; i + 1 < tok.size(); ++i) {
    const std::string g = tok[i];
    if ((g == "A" || g == "B" || g == "a" || g == "b")
        && !tok[i + 1].empty()
        && std::isdigit(static_cast<unsigned char>(tok[i + 1][0]))) {
      try {
        chart.programChangeGroup = static_cast<char>(std::toupper(static_cast<unsigned char>(g[0])));
        chart.programChangeNumber = std::stoi(tok[i + 1]);
      } catch (...) {
      }
      return true;
    }
  }
  return true;
}

bool isMetaKey(const std::vector<std::string>& tok, const char* a, const char* b = nullptr)
{
  if (tok.empty())
    return false;
  if (!iequals(tok[0], a))
    return false;
  if (b == nullptr)
    return true;
  return tok.size() >= 2 && iequals(tok[1], b);
}

std::string valueAfterColon(std::string_view line)
{
  const auto pos = line.find(':');
  if (pos == std::string_view::npos)
    return {};
  return trim(line.substr(pos + 1));
}

} // namespace

const ParsedField* findField(const ParsedSlot& slot, int offset)
{
  for (const auto& f : slot.fields) {
    if (f.offset == offset)
      return &f;
  }
  return nullptr;
}

ParsedChart parsePatchChart(std::string_view text)
{
  ParsedChart chart;
  for (int i = 0; i < 12; ++i) {
    chart.effectSlots[static_cast<std::size_t>(i)].identity = i;
    chart.effectSlots[static_cast<std::size_t>(i)].kind = kindForSlot(i, 0, true);
  }

  int currentIdentity = -1;
  bool inExpression = false;
  bool expressionHadValues = false;
  bool inComments = false;
  std::unordered_map<std::string, LabelTarget> currentLabels;
  int lineNo = 0;

  auto warn = [&](const std::string& msg) { chart.warnings.push_back(msg); };
  auto where = [&]() -> std::string {
    std::ostringstream os;
    os << "line " << lineNo;
    if (currentIdentity >= 0)
      os << " " << slotName(currentIdentity);
    return os.str();
  };

  std::string line;
  std::istringstream in{std::string(text)};
  while (std::getline(in, line)) {
    ++lineNo;
    if (!line.empty() && line.back() == '\r')
      line.pop_back();

    const std::string trimmed = trim(line);
    if (trimmed.empty() || isBanner(trimmed) || trimmed.front() == '*')
      continue;
    if (isSeparator(trimmed)) {
      currentIdentity = -1;
      inExpression = false;
      inComments = false;
      currentLabels.clear();
      continue;
    }

    const auto tok = tokenize(trimmed);
    if (tok.empty())
      continue;

    if (isMetaKey(tok, "Patch", "Name") || (iequals(tok[0], "Patch") && tok.size() > 1 && iequals(tok[1], "Name"))) {
      chart.name = valueAfterColon(trimmed);
      inComments = false;
      continue;
    }
    if (isMetaKey(tok, "Author")) {
      chart.author = valueAfterColon(trimmed);
      inComments = false;
      continue;
    }
    if (isMetaKey(tok, "Comments")) {
      chart.comments = valueAfterColon(trimmed);
      inComments = true;
      continue;
    }
    if (parseProgramChange(chart, tok)) {
      inComments = false;
      continue;
    }

    if (inComments && (trimmed.size() < 2 || trimmed[0] == ' ' || trimmed[0] == '\t'
                       || !std::isupper(static_cast<unsigned char>(trimmed[0])))) {
      if (!chart.comments.empty())
        chart.comments += ' ';
      chart.comments += trimmed;
      continue;
    }

    if (tok.size() >= 4 && iequals(tok[0], "SEQUENCE") && iequals(tok[1], "BLOCK")) {
      const char block = static_cast<char>(std::toupper(static_cast<unsigned char>(tok[2][0])));
      parseSequence(chart, block, tok[3], chart.warnings);
      inComments = false;
      continue;
    }
    if (tok.size() >= 3 && iequals(tok[0], "BLOCK")
        && (iequals(tok[1], "A") || iequals(tok[1], "B"))) {
      const char block = static_cast<char>(std::toupper(static_cast<unsigned char>(tok[1][0])));
      for (std::size_t i = 2; i < tok.size(); ++i) {
        if (isOnOffPattern(tok[i])) {
          parseOnOff(chart, block, tok[i]);
          break;
        }
      }
      inComments = false;
      continue;
    }

    if (iequals(tok[0], "EXPRESSION") || (tok.size() >= 2 && iequals(tok[0], "EXPRESSION") && iequals(tok[1], "PEDAL"))) {
      inExpression = true;
      currentIdentity = -1;
      inComments = false;
      continue;
    }
    if (iequals(tok[0], "OUTPUT") && tok.size() >= 2 && iequals(tok[1], "SELECT")) {
      inExpression = false;
      currentIdentity = -1;
      inComments = false;
      continue;
    }

    int headerIdentity = -1;
    if (isSlotToken(tok[0], headerIdentity)) {
      currentIdentity = headerIdentity;
      inExpression = false;
      inComments = false;
      auto& slot = chart.effectSlots[static_cast<std::size_t>(headerIdentity)];
      slot.sawHeader = true;
      for (std::size_t i = 1; i < tok.size(); ++i) {
        const int variant = variantFromToken(tok[i]);
        if (variant >= 0)
          applyVariant(chart, headerIdentity, variant);
      }
      currentLabels = labelsFor(slot.kind);
      continue;
    }

    if (currentIdentity == 1 || currentIdentity == 7) {
      const int variant = variantFromToken(tok[0]);
      if (variant >= 0 && (tok.size() == 1 || looksLikeEffectName(tok, 1))) {
        applyVariant(chart, currentIdentity, variant);
        currentLabels = labelsFor(chart.effectSlots[static_cast<std::size_t>(currentIdentity)].kind);
        continue;
      }
    }

    if (normalizeKey(joinFrom(tok, 0)).starts_with("MASTERVOLUME")
        || (tok.size() >= 2 && iequals(tok[0], "MASTER") && iequals(tok[1], "VOLUME"))) {
      inComments = false;
      inExpression = false;
      currentIdentity = -1;
      const auto globals = allGlobalParams();
      const ParamSpec& spec = globals[0];
      const std::size_t valueFrom = (tok.size() >= 2 && iequals(tok[0], "MASTER")) ? 2 : 1;
      const ExtractedValue v = extractValue(joinFrom(tok, valueFrom));
      if (looksPopulated(v) && (v.hasNumber || v.thru))
        chart.masterVolume = convertToRaw(spec, v, chart.warnings, where());
      continue;
    }

    if (iequals(tok[0], "CHANNEL")) {
      inComments = false;
      inExpression = false;
      currentIdentity = -1;
      const auto globals = allGlobalParams();
      const ParamSpec& spec = globals[1];
      const ExtractedValue v = extractValue(joinFrom(tok, 1));
      if (looksPopulated(v))
        chart.outputChannel = convertToRaw(spec, v, chart.warnings, where());
      continue;
    }

    if (inExpression) {
      std::size_t restFrom = 1;
      const std::string key = normalizeKey(tok[0]);
      if (key == "LFO" && tok.size() >= 2 && iequals(tok[1], "RATE"))
        restFrom = 2;
      else if (key == "EXPRESSION")
        restFrom = tok.size();
      const std::string rest = joinFrom(tok, restFrom);
      if (!rest.empty() && !iequals(rest, "OFF"))
        expressionHadValues = true;
      continue;
    }

    if (currentIdentity >= 0) {
      auto& slot = chart.effectSlots[static_cast<std::size_t>(currentIdentity)];
      if (currentLabels.empty())
        currentLabels = labelsFor(slot.kind);

      LabelTarget target;
      std::size_t valueFrom = 0;
      if (consumeLabel(tok, currentLabels, target, valueFrom) && target.spec) {
        const ExtractedValue v = extractValue(joinFrom(tok, valueFrom));
        if (!looksPopulated(v))
          continue;
        int raw = 0;
        if (target.special == LabelTarget::Special::BalanceE && v.hasNumber) {
          raw = std::clamp(static_cast<int>(std::lround(v.number * 2.0)), target.spec->min,
                           target.spec->max);
        } else if (target.special == LabelTarget::Special::BalanceD && v.hasNumber) {
          raw = std::clamp(static_cast<int>(std::lround((100.0 - v.number) * 2.0)),
                           target.spec->min, target.spec->max);
        } else {
          raw = convertToRaw(*target.spec, v, chart.warnings, where());
        }
        putField(slot, *target.spec, raw);
        continue;
      }
      warn(where() + ": unparsed '" + trimmed + "'");
      continue;
    }

    inComments = false;
    warn(std::string("line ") + std::to_string(lineNo) + ": unparsed '" + trimmed + "'");
  }

  if (expressionHadValues)
    warn("expression pedal parameters are not imported");

  for (int i = 0; i < 12; ++i) {
    const auto& slot = chart.effectSlots[static_cast<std::size_t>(i)];
    const bool summaryOn = chart.summaryEnabled[static_cast<std::size_t>(i)];
    const bool summaryKnown = (i < 6) ? chart.sawBlockAOnOff : chart.sawBlockBOnOff;
    const std::string who = std::string(slotName(i)) + " (" + specFor(slot.kind).name + ")";
    if (slot.sawHeader && !slot.present)
      warn(who + ": section header has no values");
    if (summaryKnown && summaryOn && !slot.present)
      warn(who + ": ON/OFF flags on but no section with values");
    if (summaryKnown && !summaryOn && slot.present)
      warn(who + ": ON/OFF flags off but section has values");
  }

  return chart;
}

ParsedChart parsePatchChartFile(const std::filesystem::path& path, std::string& error)
{
  error.clear();
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    error = "could not open " + path.string();
    ParsedChart chart;
    chart.warnings.push_back(error);
    return chart;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return parsePatchChart(ss.str());
}

namespace {

const ParamSpec* specAtOffset(EffectKind kind, int offset)
{
  for (const auto& p : specFor(kind).params) {
    if (p.offset == offset)
      return &p;
  }
  return nullptr;
}

void writeName(Patch& patch, std::string_view name)
{
  for (int i = 0; i < roland::kPatchNameLength; ++i) {
    const char ch = i < static_cast<int>(name.size()) ? name[static_cast<std::size_t>(i)] : ' ';
    const auto b = static_cast<unsigned char>(ch);
    patch.setByteAt(roland::kPatchNameOffset + i, b >= 32 && b < 127 ? b : ' ');
  }
  patch.setByteAt(roland::kPatchNameOffset + roland::kPatchNameLength, 0);
}

} // namespace

Patch chartToPatch(const ParsedChart& chart, int index)
{
  std::vector<std::uint8_t> data(static_cast<std::size_t>(roland::kPatchDataBytes), 0);
  Patch patch;
  patch.parse(data, index);

  for (int i = 0; i < 5; ++i)
    patch.setByteAt(i, static_cast<std::uint8_t>(chart.blockAOrder[static_cast<std::size_t>(i)]));
  patch.setByteAt(0x05, 5);
  for (int i = 0; i < 5; ++i)
    patch.setByteAt(6 + i, static_cast<std::uint8_t>(chart.blockBOrder[static_cast<std::size_t>(i)]));
  patch.setByteAt(0x0B, 11);

  patch.setByteAt(0x0C, static_cast<std::uint8_t>(chart.blockB2Mode.value_or(0) & 0x03));

  for (int id = 0; id < Patch::kEffectCount; ++id)
    patch.setEffectEnabled(id, chart.effectSlots[static_cast<std::size_t>(id)].present);

  std::uint8_t high = patch.byteAt(0x0D);
  if (chart.isDistortion.value_or(true))
    high = static_cast<std::uint8_t>(high & ~0x40u);
  else
    high = static_cast<std::uint8_t>(high | 0x40u);
  patch.setByteAt(0x0D, high);

  for (const auto& slot : chart.effectSlots) {
    if (!slot.present)
      continue;
    for (const auto& field : slot.fields) {
      if (const ParamSpec* spec = specAtOffset(slot.kind, field.offset))
        writeParam(patch, *spec, field.raw);
      else if (field.byteWidth >= 2)
        patch.setWordAt(field.offset, field.raw);
      else
        patch.setByteAt(field.offset, static_cast<std::uint8_t>(field.raw));
    }
  }

  const auto globals = allGlobalParams();
  if (chart.masterVolume && !globals.empty())
    writeParam(patch, globals[0], *chart.masterVolume);
  if (chart.outputChannel && globals.size() >= 2)
    writeParam(patch, globals[1], *chart.outputChannel);

  writeName(patch, chart.name);

  std::vector<std::uint8_t> copy(patch.rawData().begin(), patch.rawData().end());
  patch.parse(copy, index);
  return patch;
}

namespace {

const ParamSpec* chartSpec(EffectKind kind, int offset)
{
  return specAtOffset(kind, offset);
}

std::string formatHzValue(double hz)
{
  std::ostringstream os;
  os << std::fixed;
  if (hz >= 1000.0)
    os << std::setprecision(2) << (hz / 1000.0) << " kHz";
  else
    os << std::setprecision(0) << hz << " Hz";
  return os.str();
}

std::string formatChartValue(const ParamSpec& spec, int raw)
{
  if (spec.type == ParamType::Combo && spec.comboItems && spec.comboCount > 0) {
    const int i = std::clamp(raw, spec.min, spec.max);
    if (i >= 0 && i < spec.comboCount)
      return spec.comboItems[i];
  }
  if (spec.type == ParamType::Checkbox)
    return raw ? "ON" : "OFF";
  if ((spec.offset == 0x4F || spec.offset == 0x53) && raw >= spec.max)
    return "THRU";
  if (spec.offset == 0x51) {
    const double sec = raw <= 45 ? 0.5 + static_cast<double>(raw) * 0.1
                                 : 5.5 + static_cast<double>(raw - 46) * 0.5;
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << sec << " sec";
    return os.str();
  }
  if (spec.offset == 0x4F || spec.offset == 0x53) {
    ParamSpec freq = spec;
    freq.min = 0;
    freq.max = 199;
    freq.transform = DisplayTransform::FreqLog;
    freq.displayMin = 500;
    freq.displayMax = 8000;
    return formatHzValue(rawToDisplay(freq, raw));
  }

  const double display = rawToDisplay(spec, raw);
  std::ostringstream os;
  os << std::fixed;
  switch (spec.transform) {
    case DisplayTransform::LevelDb:
      os << std::setprecision(1);
      if (display == 0.0)
        os << "0.0 dB";
      else
        os << std::showpos << display << " dB";
      break;
    case DisplayTransform::QValue:
      os << std::setprecision(1) << display;
      break;
    case DisplayTransform::FreqLinear:
    case DisplayTransform::FreqLog:
      return formatHzValue(display);
    case DisplayTransform::Offset12: {
      const int n = static_cast<int>(std::lround(display));
      if (n > 0)
        os << '+' << n;
      else
        os << n;
      break;
    }
    default: {
      const int n = static_cast<int>(std::lround(display));
      os << n;
      if (spec.suffix && std::string_view(spec.suffix).find("ms") != std::string_view::npos)
        os << " msec";
      break;
    }
  }
  return os.str();
}

std::string padLabel(std::string_view label)
{
  constexpr int kWidth = 11;
  std::string s(label);
  if (static_cast<int>(s.size()) >= kWidth)
    return s + " ";
  return std::string(static_cast<std::size_t>(kWidth - static_cast<int>(s.size())), ' ') + s + " ";
}

void emitKv(std::ostringstream& os, std::string_view label, std::string_view value)
{
  os << padLabel(label) << value << "\n";
}

void emitParam(std::ostringstream& os, std::string_view label, EffectKind kind, int offset,
               const Patch& patch)
{
  const ParamSpec* spec = chartSpec(kind, offset);
  if (!spec)
    return;
  emitKv(os, label, formatChartValue(*spec, readParam(patch, *spec)));
}

void emitSep(std::ostringstream& os)
{
  os << " ----------------------------------\n";
}

std::string sequenceDigits(const std::array<int, 6>& order)
{
  std::string s(6, '1');
  for (int i = 0; i < 6; ++i) {
    const int id = order[static_cast<std::size_t>(i)];
    const int local = id < 6 ? id : id - 6;
    s[static_cast<std::size_t>(i)] = static_cast<char>('1' + local);
  }
  return s;
}

std::string onOffDigits(const std::array<int, 6>& order, const Patch& patch)
{
  std::string s(6, '*');
  for (int i = 0; i < 6; ++i) {
    const int id = order[static_cast<std::size_t>(i)];
    if (!patch.isEffectEnabled(id))
      continue;
    const int local = id < 6 ? id : id - 6;
    s[static_cast<std::size_t>(i)] = static_cast<char>('1' + local);
  }
  return s;
}

std::string reverbModeLine(int raw)
{
  const auto& spec = specFor(EffectKind::Reverb);
  const ParamSpec* mode = nullptr;
  for (const auto& p : spec.params) {
    if (p.offset == 0x52)
      mode = &p;
  }
  if (!mode || !mode->comboItems || raw < 0 || raw >= mode->comboCount)
    return {};
  std::string item = mode->comboItems[raw];
  for (char& c : item)
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return "MODE: " + item;
}

void emitEffect(std::ostringstream& os, const Patch& patch, int identity)
{
  if (!patch.isEffectEnabled(identity))
    return;
  const EffectKind kind = kindForSlot(identity, patch.blockB2Mode(), patch.isDistortion());
  emitSep(os);

  switch (identity) {
    case 0:
      os << " A-1 COMPRESSOR\n\n";
      emitParam(os, "TONE", kind, 0x0F, patch);
      emitParam(os, "ATTACK", kind, 0x10, patch);
      emitParam(os, "SUSTAIN", kind, 0x11, patch);
      emitParam(os, "LEVEL", kind, 0x12, patch);
      break;
    case 1:
      if (patch.isDistortion()) {
        os << " A-2  a. DISTORTION\n\n";
        emitParam(os, "TONE", kind, 0x13, patch);
        emitParam(os, "DISTORTION", kind, 0x14, patch);
        emitParam(os, "LEVEL", kind, 0x15, patch);
      } else {
        os << " A-2  b. OVERDRIVE\n\n";
        emitParam(os, "TONE", kind, 0x16, patch);
        emitParam(os, "DRIVE", kind, 0x17, patch);
        os << padLabel("TURBO:") << (patch.byteAt(0x18) ? "ON" : "OFF") << "\n";
        emitParam(os, "LEVEL", kind, 0x19, patch);
      }
      break;
    case 2:
      os << " A-3 PICKING FILTER\n\n";
      emitParam(os, "SENS", kind, 0x1A, patch);
      emitParam(os, "CUTOFF FREQ", kind, 0x1B, patch);
      emitParam(os, "Q", kind, 0x1C, patch);
      emitParam(os, "UP/DOWN", kind, 0x1D, patch);
      break;
    case 3:
      os << " A-4 STEP PHASER\n\n";
      emitParam(os, "RATE", kind, 0x1E, patch);
      emitParam(os, "DEPTH", kind, 0x1F, patch);
      emitParam(os, "MANUAL", kind, 0x20, patch);
      emitParam(os, "RESONANCE", kind, 0x21, patch);
      emitParam(os, "LFO STEP", kind, 0x22, patch);
      break;
    case 4:
      os << " A-5 PARAMETRIC EQ\n\n";
      emitParam(os, "HI FREQ", kind, 0x23, patch);
      emitParam(os, "HI LEVEL", kind, 0x24, patch);
      os << "\n";
      emitParam(os, "H.M.FREQ", kind, 0x25, patch);
      emitParam(os, "H.M. Q", kind, 0x26, patch);
      emitParam(os, "H.M. LEVEL", kind, 0x27, patch);
      os << "\n";
      emitParam(os, "L.M. FREQ", kind, 0x28, patch);
      emitParam(os, "L.M. Q", kind, 0x29, patch);
      emitParam(os, "L.M. LEVEL", kind, 0x2A, patch);
      os << "\n";
      emitParam(os, "LO FREQ", kind, 0x2B, patch);
      emitParam(os, "LO LEVEL", kind, 0x2C, patch);
      os << "\n";
      emitParam(os, "OUT LEVEL", kind, 0x2D, patch);
      break;
    case 5:
      os << " A-6 NOISE SUPPRESSOR\n\n";
      emitParam(os, "SENS", kind, 0x2E, patch);
      emitParam(os, "RELEASE", kind, 0x2F, patch);
      emitParam(os, "LEVEL", kind, 0x30, patch);
      break;
    case 6:
      os << " B-1 SHORT DELAY\n\n";
      emitParam(os, "D. TIME", kind, 0x31, patch);
      emitParam(os, "E. LEVEL", kind, 0x32, patch);
      break;
    case 7:
      switch (patch.blockB2Mode()) {
        case 1:
          os << " B-2  b. FLANGER\n\n";
          emitParam(os, "RATE", kind, 0x37, patch);
          emitParam(os, "DEPTH", kind, 0x38, patch);
          emitParam(os, "MANUAL", kind, 0x39, patch);
          emitParam(os, "RESONANCE", kind, 0x3A, patch);
          break;
        case 2: {
          os << " B-2  c. PITCH SHIFTER\n\n";
          const int bal = patch.wordAt(0x3B);
          emitKv(os, "BAL. E.", std::to_string(bal / 2));
          emitKv(os, "BAL. D.", std::to_string(100 - bal / 2));
          emitParam(os, "CHROMATIC", kind, 0x3D, patch);
          emitParam(os, "FINE", kind, 0x3E, patch);
          emitParam(os, "F.BACK", kind, 0x3F, patch);
          emitParam(os, "P.DELAY", kind, 0x40, patch);
          break;
        }
        case 3:
          os << " B-2  d. SPACE-D\n\n";
          emitParam(os, "MODE", kind, 0x41, patch);
          break;
        default:
          os << " B-2  a. CHORUS\n\n";
          emitParam(os, "P. DELAY", kind, 0x33, patch);
          emitParam(os, "RATE", kind, 0x34, patch);
          emitParam(os, "DEPTH", kind, 0x35, patch);
          emitParam(os, "E. LEVEL", kind, 0x36, patch);
          break;
      }
      break;
    case 8:
      os << " B-3 AUTO PANPOT\n\n";
      emitParam(os, "RATE", kind, 0x42, patch);
      emitParam(os, "DEPTH", kind, 0x43, patch);
      emitParam(os, "MODE", kind, 0x44, patch);
      break;
    case 9:
      os << " B-4 TAP DELAY\n\n";
      emitParam(os, "C. TAP", kind, 0x45, patch);
      emitParam(os, "L. TAP", kind, 0x47, patch);
      emitParam(os, "R. TAP", kind, 0x49, patch);
      emitParam(os, "C. LEVEL", kind, 0x4B, patch);
      emitParam(os, "L. LEVEL", kind, 0x4C, patch);
      emitParam(os, "R. LEVEL", kind, 0x4D, patch);
      emitParam(os, "FEEDBACK", kind, 0x4E, patch);
      emitParam(os, "CUTOFF", kind, 0x4F, patch);
      break;
    case 10:
      os << " B-5 REVERB\n\n";
      emitParam(os, "DECAY", kind, 0x51, patch);
      os << "\n";
      os << " " << reverbModeLine(static_cast<int>(patch.byteAt(0x52))) << "\n";
      os << "\n";
      emitParam(os, "CUTOFF", kind, 0x53, patch);
      emitParam(os, "PRE DELAY", kind, 0x55, patch);
      emitParam(os, "E. LEVEL", kind, 0x56, patch);
      break;
    case 11:
      os << " B-6 LINEOUT FILTER\n\n";
      emitParam(os, "PRESENCE", kind, 0x57, patch);
      emitParam(os, "TREBLE", kind, 0x58, patch);
      emitParam(os, "MIDDLE", kind, 0x59, patch);
      emitParam(os, "BASS", kind, 0x5A, patch);
      break;
    default:
      break;
  }
  os << "\n";
}

} // namespace

std::string toChartText(const Patch& patch, const ChartMetadata& meta)
{
  std::ostringstream os;
  os << " **********************************************\n";
  os << " ********* Roland GP-16 Patch Chart ***********\n";
  os << " **********************************************\n\n";
  os << " Patch Name: " << patch.name() << "\n";
  os << "     Author: " << meta.author << "\n";
  os << "   Comments: " << meta.comments << "\n\n";
  os << " PROGRAM CHANGE NO.";
  if (meta.programChangeGroup && meta.programChangeNumber)
    os << " " << *meta.programChangeGroup << ": " << *meta.programChangeNumber;
  os << "\n\n";
  os << " SEQUENCE BLOCK A\t" << sequenceDigits(patch.blockAOrder()) << "\n";
  os << " SEQUENCE BLOCK B\t" << sequenceDigits(patch.blockBOrder()) << "\n\n";
  os << " BLOCK A - ON/OFF\t" << onOffDigits(patch.blockAOrder(), patch) << "\n";
  os << " BLOCK B - ON/OFF\t" << onOffDigits(patch.blockBOrder(), patch) << "\n\n";

  for (int id = 0; id < Patch::kEffectCount; ++id)
    emitEffect(os, patch, id);

  emitSep(os);
  os << " MASTER VOLUME " << static_cast<int>(patch.byteAt(0x5B)) << "\n\n";

  emitSep(os);
  os << " EXPRESSION PEDAL\n\n";
  os << " ASSIGN OFF\n\n";

  emitSep(os);
  os << " OUTPUT SELECT\n\n";
  const auto globals = allGlobalParams();
  if (globals.size() >= 2)
    os << " CHANNEL  " << formatChartValue(globals[1], static_cast<int>(patch.byteAt(0x63)))
       << "\n";
  else
    os << " CHANNEL  1\n";

  return os.str();
}

bool writePatchChartFile(const std::filesystem::path& path, const Patch& patch,
                         const ChartMetadata& meta, std::string& error)
{
  error.clear();
  std::ofstream out(path);
  if (!out) {
    error = "could not write " + path.string();
    return false;
  }
  out << toChartText(patch, meta);
  if (!out) {
    error = "failed writing " + path.string();
    return false;
  }
  return true;
}
