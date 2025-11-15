#include "swtor_parser.h"
#include <charconv>
#include <sstream>
#include <iomanip>
#include <cctype>

namespace swtor {

static inline bool starts_with(std::string_view s, std::string_view p) {
  return s.size()>=p.size() && s.substr(0,p.size())==p;
}

uint64_t parse_u64(std::string_view sv, bool& ok) {
  uint64_t v=0;
  ok=false;
  auto first = sv.data();
  auto last  = sv.data()+sv.size();
  auto res = std::from_chars(first, last, v, 10);
  if (res.ec == std::errc() && res.ptr==last) { ok=true; }
  return v;
}

double parse_double(std::string_view sv, bool& ok) {
  // simple strtod fallback (since from_chars(double) wasn't widely implemented pre-C++17 in some libs)
  try {
    std::string s(sv);
    size_t idx=0;
    double v = std::stod(s, &idx);
    ok = (idx==s.size());
    return v;
  } catch (...) {
    ok=false; return 0.0;
  }
}

bool parse_timestamp(std::string_view sv, TimeStamp& ts) {
  // expects "hh:mm:ss.mmm" (without brackets)
  // minimal robust parse
  int hh=0, mm=0, ss=0, ms=0;
  if (sv.size()<12) return false;
  auto to_int = [](std::string_view x)->int{
    int v=0; for(char c: x){ if(!std::isdigit((unsigned char)c)) return -1; v = v*10 + (c-'0'); } return v;
  };
  if (!std::isdigit((unsigned char)sv[0])) return false;
  auto p1 = sv.find(':');
  auto p2 = sv.find(':', p1+1);
  auto p3 = sv.find('.', p2+1);
  if (p1==std::string_view::npos || p2==std::string_view::npos || p3==std::string_view::npos) return false;
  hh = to_int(sv.substr(0, p1));
  mm = to_int(sv.substr(p1+1, p2-(p1+1)));
  ss = to_int(sv.substr(p2+1, p3-(p2+1)));
  ms = to_int(sv.substr(p3+1));
  if (hh<0||mm<0||ss<0||ms<0) return false;
  ts.hh=hh; ts.mm=mm; ts.ss=ss; ts.ms=ms;
  return true;
}

static inline std::string trim_copy(std::string_view sv) {
  size_t a=0,b=sv.size();
  while (a<b && std::isspace((unsigned char)sv[a])) ++a;
  while (b>a && std::isspace((unsigned char)sv[b-1])) --b;
  return std::string(sv.substr(a,b-a));
}

// Extract name and optional #hash from tokens like [@Zag#689392444394070|( ... )]
static void parse_bracket_entity(std::string_view sv, EntityRef& e) {
  // sv will be like: @Zag#689392444394070|(-342.10,148.65,0.13,-9.94)|(472674/472674)
  // We'll stop at first '|' for the name + hash part.
  e = EntityRef{};
  // Detect player/companion by leading sigil in the bracket: [@ ...] == player, [=] self, [+] companion (convention)
  if (!sv.empty() && (sv[0]=='@' || sv[0]=='=' || sv[0]=='+')) {
    e.is_player = (sv[0]=='@' || sv[0]=='=');
    e.is_companion = (sv[0]=='+');
    sv.remove_prefix(1);
  }
  // Now parse "Name#hash" or just "Name"
  auto pipe = sv.find('|');
  std::string_view head = pipe==std::string_view::npos ? sv : sv.substr(0, pipe);
  auto hash_pos = head.find('#');
  if (hash_pos != std::string_view::npos) {
    e.name = std::string(head.substr(0, hash_pos));
    std::string_view h = head.substr(hash_pos+1);
    bool ok=false;
    e.hash = parse_u64(h, ok);
    if (!ok) e.hash = 0;
  } else {
    e.name = std::string(head);
  }
  // Derive static_id convention for printing:
  e.static_id = e.is_player ? 1 : (e.is_companion ? 2 : 3);
  e.instance_id = 0;
}

static void parse_named_with_id(std::string_view token, NamedId& out) {
  // e.g., "Flesh Eating Shriek {4297170614222848}:10545000040696"
  out = NamedId{};
  auto brace_open = token.find('{');
  auto brace_close = token.find('}', brace_open==std::string_view::npos?0:brace_open+1);
  if (brace_open!=std::string_view::npos && brace_close!=std::string_view::npos && brace_close>brace_open) {
    // name before space before {
    std::string_view name_part = token.substr(0, token.find_last_of(" {"));
    out.name = trim_copy(name_part);
    std::string_view id_part = token.substr(brace_open+1, brace_close-brace_open-1);
    bool ok=false; out.id = parse_u64(id_part, ok); if (!ok) out.id=0;
  } else {
    out.name = trim_copy(token);
  }
}

static void parse_event_effect(std::string_view token, EventEffect& ee) {
  // e.g., "ApplyEffect {836045448945477}: Damage {836045448945501}"
  ee = EventEffect{};
  auto colon = token.find(':');
  std::string_view left = colon==std::string_view::npos? token : token.substr(0, colon);
  std::string_view right= colon==std::string_view::npos? std::string_view{} : token.substr(colon+1);
  // left: "ApplyEffect {id}"
  NamedId nid{}; parse_named_with_id(trim_copy(left), *(NamedId*)&nid);
  ee.name = nid.name; ee.id = nid.id;
  // right: " Damage {id2}" -> we'll keep only the name in ee.detail; numeric in ee.id stays as ApplyEffect's id
  if (!right.empty()) {
    // Attempt to strip any trailing {id}
    auto brace_open = right.find('{');
    std::string_view detail = (brace_open==std::string_view::npos) ? right : right.substr(0, brace_open);
    ee.detail = trim_copy(detail);
  }
}

static void parse_value_and_school(std::string_view tail, Value& v) {
  // Example tail: "(442* energy {836045448940874}) <5299.0>"
  v = Value{};
  auto lt = tail.find('<');
  auto gt = tail.find('>', lt==std::string_view::npos?0:lt+1);
  if (lt!=std::string_view::npos && gt!=std::string_view::npos && gt>lt+1) {
    std::string_view amount = tail.substr(lt+1, gt-lt-1);
    bool ok=false; v.amount = parse_double(trim_copy(amount), ok);
    v.has_amount = ok;
  }
  // find a " school " word inside parentheses
  auto lp = tail.find('(');
  auto rp = tail.find(')', lp==std::string_view::npos?0:lp+1);
  if (lp!=std::string_view::npos && rp!=std::string_view::npos && rp>lp) {
    std::string inside = std::string(tail.substr(lp+1, rp-lp-1));
    // naive extract: look for alphabetic token (e.g., "energy")
    std::string word;
    for (char c: inside) {
      if (std::isalpha((unsigned char)c)) { word.push_back(c); }
      else if (!word.empty()) break;
    }
    v.school = word;
  }
}

bool parse_line(std::string_view sv, CombatLine& out) {
  out = CombatLine{};
  out.raw.assign(sv.begin(), sv.end());

  // Expect leading [timestamp]
  if (sv.empty() || sv.front()!='[') return false;
  auto rb = sv.find(']');
  if (rb==std::string_view::npos) return false;
  std::string_view ts_sv = sv.substr(1, rb-1);
  if (!parse_timestamp(ts_sv, out.ts)) return false;
  // Next token: [source ...]
  auto lb2 = sv.find('[', rb+1);
  auto rb2 = sv.find(']', lb2==std::string_view::npos?0:lb2+1);
  if (lb2==std::string_view::npos || rb2==std::string_view::npos) return false;
  std::string_view src_block = sv.substr(lb2+1, rb2-lb2-1);
  parse_bracket_entity(src_block, out.source);
  // Next token: [ability ...]
  auto lb3 = sv.find('[', rb2+1);
  auto rb3 = sv.find(']', lb3==std::string_view::npos?0:lb3+1);
  if (lb3==std::string_view::npos || rb3==std::string_view::npos) return false;
  std::string_view ability_block = sv.substr(lb3+1, rb3-lb3-1);
  // ability_block like: Flesh Eating Shriek {id}:10545000040696|(...)
  // Split before first '|' and parse named id on the left side before optional colon
  {
    auto pipe = ability_block.find('|');
    std::string_view head = pipe==std::string_view::npos? ability_block : ability_block.substr(0, pipe);
    // remove any trailing ":<number>"
    auto colon = head.find(':');
    std::string_view head_named = colon==std::string_view::npos? head : head.substr(0, colon);
    parse_named_with_id(trim_copy(head_named), out.ability);
  }
  // Next token: [aux ...] (optional)
  auto lb4 = sv.find('[', rb3+1);
  auto rb4 = sv.find(']', lb4==std::string_view::npos?0:lb4+1);
  if (lb4!=std::string_view::npos && rb4!=std::string_view::npos) {
    std::string_view aux_block = sv.substr(lb4+1, rb4-lb4-1);
    parse_named_with_id(trim_copy(aux_block), out.aux);
  } else {
    lb4 = rb4 = rb3; // so event search starts after previous block
  }
  // Next token: [event ...]
  auto lb5 = sv.find('[', rb4+1);
  auto rb5 = sv.find(']', lb5==std::string_view::npos?0:lb5+1);
  if (lb5!=std::string_view::npos && rb5!=std::string_view::npos) {
    std::string_view event_block = sv.substr(lb5+1, rb5-lb5-1);
    parse_event_effect(trim_copy(event_block), out.event);
  }
  // Tail: whatever remains (value/school)
  std::string_view tail = (rb5!=std::string_view::npos) ? sv.substr(rb5+1) : sv.substr(rb4+1);
  parse_value_and_school(tail, out.value);

  return true;
}

static std::string quoted(const std::string& s) {
  std::string out; out.reserve(s.size()+2);
  out.push_back('\"'); out.append(s); out.push_back('\"'); return out;
}

std::string to_text(const CombatLine& line, const PrintOptions& opt) {
  // Recreate your desired "src: PLAYER \"Name\" id[static:hash:instance]" text using 64-bit hash
  const char* kind = line.source.is_player ? "PLAYER" : (line.source.is_companion ? "COMPANION" : "NPC");
  std::ostringstream os;
  os << "src: " << kind << ' ' << quoted(line.source.name)
     << " id[" << line.source.static_id << ':' << line.source.hash << ':' << line.source.instance_id << ']';
  return os.str();
}

} // namespace swtor
