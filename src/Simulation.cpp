#include "Simulation.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <sstream>

namespace {
constexpr float kDayLength = 180.0f;
constexpr int kNpcCount = 28;
constexpr int kCarCount = 10;

std::string jobName(JobKind job) {
  switch (job) {
    case JobKind::Office:
      return "office clerk";
    case JobKind::Cafe:
      return "barista";
    case JobKind::Shop:
      return "shop clerk";
    case JobKind::Police:
      return "officer";
    default:
      return "unemployed";
  }
}

const char* kFirst[] = {"Alex", "Sam", "Jordan", "Riley", "Casey", "Quinn", "Avery", "Morgan",
                        "Jamie", "Taylor", "Reese", "Drew", "Parker", "Skyler", "Cameron", "Harper"};
const char* kLast[] = {"Chen", "Ortiz", "Nguyen", "Patel", "Brooks", "Ibarra", "Klein", "Sato",
                       "Diaz", "Walsh", "Khan", "Meyer", "Costa", "Berg", "Young", "Frost"};
}  // namespace

Simulation::Simulation(City& city)
    : city_(city),
      dayTime_(8.0f * kDayLength / 24.0f),
      day_(1),
      eventCooldown_(12.0f),
      followTimer_(0.0f),
      seed_(0xC0FFEEu) {
  player_.position = city_.cellCenter(city_.plazaX(), city_.plazaZ() + 2);
  player_.heading = 0.0f;
  player_.speed = 7.5f;
  player_.stamina = 1.0f;
  player_.homeX = 4;
  player_.homeZ = 8;
  player_.workX = city_.officeX();
  player_.workZ = city_.officeZ();
  player_.job = JobKind::Office;
  player_.trustCity = 0.55f;
  player_.crimesWitnessed = 0;
  player_.npcsWarned = 0;
  player_.reportsFiled = 0;
  player_.timesFollowedHero = 0;
  player_.timesHelpedHero = 0;
  player_.daysSurvived = 0;
  player_.enteredRestricted = false;
  player_.foundSeam = false;
  player_.discoveredHero = false;
  player_.skippedWork = false;

  hero_.position = city_.cellCenter(city_.bankX(), city_.bankZ() + 1);
  hero_.heading = 1.2f;
  hero_.shirt = Color{200, 40, 50, 255};
  hero_.actionTimer = 4.0f;
  hero_.scheme = 0;
  hero_.inRestricted = false;
  hero_.visible = true;
  hero_.vanishTimer = 0.0f;
  hero_.crimesCommitted = 0;
  hero_.robberyDone = false;

  spawnAgents();
  spawnCars();
  rememberPlayer("First day on the job. The city feels ordinary.", false);
}

unsigned Simulation::nextRand() {
  seed_ = seed_ * 1664525u + 1013904223u;
  return seed_;
}

float Simulation::nextFloat() {
  return static_cast<float>(nextRand() & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu);
}

int Simulation::nextInt(int lo, int hi) {
  if (hi <= lo) {
    return lo;
  }
  return lo + static_cast<int>(nextRand() % static_cast<unsigned>(hi - lo + 1));
}

int Simulation::hour() const {
  return static_cast<int>((dayTime_ / kDayLength) * 24.0f) % 24;
}

std::string Simulation::clockText() const {
  const int h = hour();
  const float frac = (dayTime_ / kDayLength) * 24.0f - static_cast<float>(h);
  const int m = static_cast<int>(frac * 60.0f);
  std::ostringstream oss;
  oss << "Day " << day_ << "  ";
  if (h < 10) {
    oss << '0';
  }
  oss << h << ':';
  if (m < 10) {
    oss << '0';
  }
  oss << m;
  return oss.str();
}

std::string Simulation::weatherText() const {
  const int h = hour();
  if (h < 6) {
    return "Night";
  }
  if (h < 11) {
    return "Morning";
  }
  if (h < 17) {
    return "Afternoon";
  }
  if (h < 20) {
    return "Evening";
  }
  return "Night";
}

void Simulation::spawnAgents() {
  agents_.clear();
  agents_.reserve(static_cast<std::size_t>(kNpcCount));
  for (int i = 0; i < kNpcCount; ++i) {
    Agent a{};
    a.id = i + 1;
    a.name = std::string(kFirst[static_cast<std::size_t>(i % 16)]) + " " +
             kLast[static_cast<std::size_t>((i * 3) % 16)];
    a.homeX = 0;
    a.homeZ = 0;
    int tries = 0;
    do {
      a.homeX = nextInt(1, City::kSize - 2);
      a.homeZ = nextInt(1, City::kSize - 2);
      ++tries;
    } while (!city_.walkable(a.homeX, a.homeZ) && tries < 40);
    if (!city_.walkable(a.homeX, a.homeZ)) {
      a.homeX = city_.plazaX();
      a.homeZ = city_.plazaZ();
    }
    const int jobRoll = i % 5;
    if (jobRoll == 0) {
      a.job = JobKind::Office;
      a.workX = city_.officeX();
      a.workZ = city_.officeZ();
      a.shirt = Color{70, 90, 130, 255};
    } else if (jobRoll == 1) {
      a.job = JobKind::Cafe;
      a.workX = city_.cafeX();
      a.workZ = city_.cafeZ();
      a.shirt = Color{160, 90, 70, 255};
    } else if (jobRoll == 2) {
      a.job = JobKind::Shop;
      a.workX = city_.shopX();
      a.workZ = city_.shopZ();
      a.shirt = Color{70, 130, 90, 255};
    } else if (jobRoll == 3) {
      a.job = JobKind::Police;
      a.workX = city_.policeX();
      a.workZ = city_.policeZ();
      a.shirt = Color{50, 70, 150, 255};
    } else {
      a.job = JobKind::Unemployed;
      a.workX = city_.plazaX();
      a.workZ = city_.plazaZ();
      a.shirt = Color{120, 110, 90, 255};
    }
    a.position = city_.cellCenter(a.homeX, a.homeZ);
    a.heading = nextFloat() * 6.28f;
    a.speed = 3.2f + nextFloat() * 1.4f;
    a.body = Color{static_cast<unsigned char>(180 + nextInt(0, 50)),
                   static_cast<unsigned char>(150 + nextInt(0, 40)),
                   static_cast<unsigned char>(130 + nextInt(0, 30)), 255};
    a.trust = 0.4f + nextFloat() * 0.4f;
    a.suspicionHero = 0.0f;
    a.missing = false;
    a.pathIndex = 0;
    a.talkCooldown = 0.0f;
    a.lastLine = "Just another day.";
    a.waitTimer = 0.0f;
    agents_.push_back(a);
  }
}

void Simulation::spawnCars() {
  cars_.clear();
  for (int i = 0; i < kCarCount; ++i) {
    Car c{};
    int x = 0;
    int z = 0;
    int tries = 0;
    do {
      x = nextInt(0, City::kSize - 1);
      z = nextInt(0, City::kSize - 1);
      ++tries;
    } while (city_.cell(x, z) != CellKind::Road && tries < 30);
    c.position = city_.cellCenter(x, z, 0.0f);
    c.heading = (i % 2 == 0) ? 0.0f : 1.57f;
    c.color = Color{static_cast<unsigned char>(40 + nextInt(0, 180)),
                    static_cast<unsigned char>(40 + nextInt(0, 180)),
                    static_cast<unsigned char>(40 + nextInt(0, 180)), 255};
    c.stolen = false;
    cars_.push_back(c);
  }
}

void Simulation::assignPath(Agent& a, int tx, int tz) {
  int sx = 0;
  int sz = 0;
  city_.worldToCell(a.position, sx, sz);
  if (!city_.inBounds(sx, sz)) {
    a.path.clear();
    a.pathIndex = 0;
    return;
  }
  if (!city_.walkable(sx, sz)) {
    a.position = city_.findNearestWalkable(a.position);
    city_.worldToCell(a.position, sx, sz);
  }
  if (!city_.walkable(tx, tz)) {
    Vector3 t = city_.findNearestWalkable(city_.cellCenter(tx, tz));
    city_.worldToCell(t, tx, tz);
  }
  if (sx == tx && sz == tz) {
    a.path.clear();
    a.pathIndex = 0;
    return;
  }

  const int w = City::kSize;
  std::vector<int> prev(static_cast<std::size_t>(w * w), -1);
  std::vector<char> seen(static_cast<std::size_t>(w * w), 0);
  std::queue<int> q;
  const int start = sz * w + sx;
  const int goal = tz * w + tx;
  q.push(start);
  seen[static_cast<std::size_t>(start)] = 1;

  const int dx[4] = {1, -1, 0, 0};
  const int dz[4] = {0, 0, 1, -1};
  bool found = false;
  while (!q.empty()) {
    const int cur = q.front();
    q.pop();
    if (cur == goal) {
      found = true;
      break;
    }
    const int cx = cur % w;
    const int cz = cur / w;
    for (int i = 0; i < 4; ++i) {
      const int nx = cx + dx[i];
      const int nz = cz + dz[i];
      if (!city_.walkable(nx, nz)) {
        continue;
      }
      const int ni = nz * w + nx;
      if (seen[static_cast<std::size_t>(ni)]) {
        continue;
      }
      seen[static_cast<std::size_t>(ni)] = 1;
      prev[static_cast<std::size_t>(ni)] = cur;
      q.push(ni);
    }
  }

  a.path.clear();
  a.pathIndex = 0;
  if (!found) {
    a.path.push_back(city_.cellCenter(tx, tz));
    return;
  }
  std::vector<int> rev;
  for (int at = goal; at != -1; at = prev[static_cast<std::size_t>(at)]) {
    rev.push_back(at);
    if (at == start) {
      break;
    }
  }
  std::reverse(rev.begin(), rev.end());
  for (int node : rev) {
    const int x = node % w;
    const int z = node / w;
    a.path.push_back(city_.cellCenter(x, z));
  }
}

void Simulation::stepAlongPath(Agent& a, float dt) {
  if (a.path.empty() || a.pathIndex >= a.path.size()) {
    a.waitTimer = std::max(0.0f, a.waitTimer - dt);
    return;
  }
  const Vector3 target = a.path[a.pathIndex];
  Vector3 d = Vector3Subtract(target, a.position);
  d.y = 0.0f;
  const float dist = Vector3Length(d);
  if (dist < 0.35f) {
    ++a.pathIndex;
    return;
  }
  const Vector3 dir = Vector3Scale(d, 1.0f / dist);
  a.heading = std::atan2(dir.z, dir.x);
  a.position = Vector3Add(a.position, Vector3Scale(dir, a.speed * dt));
  a.position = city_.clampWorld(a.position);
}

Vector3 Simulation::desiredTarget(const Agent& a) const {
  const int h = hour();
  if (a.job == JobKind::Police && !events_.empty()) {
    for (const WorldEvent& e : events_) {
      if (e.active) {
        return e.position;
      }
    }
  }
  if (h >= 8 && h < 17 && a.job != JobKind::Unemployed) {
    return city_.cellCenter(a.workX, a.workZ);
  }
  if (h >= 17 && h < 20) {
    if ((a.id % 3) == 0) {
      return city_.cellCenter(city_.cafeX(), city_.cafeZ());
    }
    if ((a.id % 3) == 1) {
      return city_.cellCenter(city_.shopX(), city_.shopZ());
    }
    return city_.cellCenter(city_.plazaX(), city_.plazaZ());
  }
  if (h >= 20 || h < 6) {
    return city_.cellCenter(a.homeX, a.homeZ);
  }
  return city_.cellCenter(city_.plazaX(), city_.plazaZ());
}

void Simulation::remember(Agent& a, const std::string& text, bool aboutHero) {
  if (a.memory.size() > 12) {
    a.memory.erase(a.memory.begin());
  }
  a.memory.push_back(MemoryFact{text, day_, aboutHero});
  a.lastLine = text;
  if (aboutHero) {
    a.suspicionHero = std::min(1.0f, a.suspicionHero + 0.18f);
  }
}

void Simulation::rememberPlayer(const std::string& text, bool aboutHero) {
  if (player_.journal.size() > 24) {
    player_.journal.erase(player_.journal.begin());
  }
  player_.journal.push_back(MemoryFact{text, day_, aboutHero});
  if (aboutHero) {
    player_.discoveredHero = true;
  }
}

void Simulation::spawnEvent() {
  WorldEvent e{};
  const int roll = nextInt(0, 6);
  e.kind = static_cast<EventKind>(roll);
  int x = nextInt(1, City::kSize - 2);
  int z = nextInt(1, City::kSize - 2);
  if (!city_.walkable(x, z)) {
    Vector3 p = city_.findNearestWalkable(city_.cellCenter(x, z));
    city_.worldToCell(p, x, z);
  }
  e.position = city_.cellCenter(x, z, 0.0f);
  e.age = 0.0f;
  e.lifetime = 18.0f + nextFloat() * 10.0f;
  e.active = true;
  switch (e.kind) {
    case EventKind::Accident:
      e.description = "A car accident blocks the street.";
      break;
    case EventKind::Robbery:
      e.description = "Someone is shouting about a robbery.";
      e.position = city_.cellCenter(city_.bankX(), city_.bankZ() + 1);
      hero_.scheme = 1;
      hero_.robberyDone = true;
      ++hero_.crimesCommitted;
      break;
    case EventKind::Fight:
      e.description = "A street fight draws a crowd.";
      break;
    case EventKind::Fire:
      e.description = "Smoke is rising from a side street.";
      break;
    case EventKind::Disappearance:
      e.description = "Someone did not show up for work.";
      if (!agents_.empty()) {
        Agent& victim = agents_[static_cast<std::size_t>(nextInt(0, static_cast<int>(agents_.size()) - 1))];
        if (!victim.missing) {
          victim.missing = true;
          e.description = victim.name + " disappeared.";
          rememberPlayer(victim.name + " vanished without a word.", true);
          for (Agent& a : agents_) {
            if (!a.missing && a.id != victim.id && nextFloat() < 0.45f) {
              remember(a, "I heard " + victim.name + " is missing.", true);
            }
          }
        }
      }
      break;
    case EventKind::Chase:
      e.description = "Sirens. A chase cuts across the plaza.";
      hero_.scheme = 2;
      ++hero_.crimesCommitted;
      break;
    case EventKind::Theft:
      e.description = "A red car is missing from the curb.";
      if (!cars_.empty()) {
        cars_[static_cast<std::size_t>(nextInt(0, static_cast<int>(cars_.size()) - 1))].stolen = true;
      }
      ++hero_.crimesCommitted;
      hero_.scheme = 3;
      break;
  }
  events_.push_back(e);
}

void Simulation::updateAgents(float dt) {
  for (Agent& a : agents_) {
    if (a.missing) {
      continue;
    }
    a.talkCooldown = std::max(0.0f, a.talkCooldown - dt);
    if (a.waitTimer > 0.0f) {
      a.waitTimer -= dt;
      continue;
    }
    const Vector3 want = desiredTarget(a);
    int tx = 0;
    int tz = 0;
    city_.worldToCell(want, tx, tz);
    if (a.path.empty() || a.pathIndex >= a.path.size()) {
      assignPath(a, tx, tz);
      a.waitTimer = 0.4f + nextFloat() * 0.8f;
    }
    stepAlongPath(a, dt);

    if (hero_.visible) {
      const float d = Vector3Distance(a.position, hero_.position);
      if (d < 6.0f && nextFloat() < 0.01f) {
        remember(a, "That person in red is not following any schedule.", true);
      }
    }
    for (const WorldEvent& e : events_) {
      if (!e.active) {
        continue;
      }
      if (Vector3Distance(a.position, e.position) < 8.0f && nextFloat() < 0.02f) {
        remember(a, e.description, e.kind == EventKind::Robbery || e.kind == EventKind::Theft ||
                                       e.kind == EventKind::Chase);
      }
    }
  }
}

void Simulation::updateHero(float dt) {
  if (hero_.vanishTimer > 0.0f) {
    hero_.vanishTimer -= dt;
    hero_.visible = hero_.vanishTimer < 0.4f;
  } else {
    hero_.visible = true;
  }

  hero_.actionTimer -= dt;
  Vector3 target = city_.cellCenter(city_.plazaX(), city_.plazaZ());
  switch (hero_.scheme % 5) {
    case 0:
      target = city_.cellCenter(city_.bankX(), city_.bankZ() + 1);
      break;
    case 1:
      target = city_.cellCenter(city_.restrictedX(), city_.restrictedZ() + 1);
      hero_.inRestricted = true;
      break;
    case 2:
      target = city_.cellCenter(city_.policeX() + 2, city_.policeZ());
      break;
    case 3:
      if (!cars_.empty()) {
        target = cars_[0].position;
      }
      break;
    default:
      target = city_.cellCenter(nextInt(1, City::kSize - 2), nextInt(1, City::kSize - 2));
      break;
  }

  Vector3 d = Vector3Subtract(target, hero_.position);
  d.y = 0.0f;
  const float dist = Vector3Length(d);
  if (dist > 0.4f) {
    const Vector3 dir = Vector3Scale(d, 1.0f / dist);
    hero_.heading = std::atan2(dir.z, dir.x);
    const float spd = 8.5f;
    hero_.position = Vector3Add(hero_.position, Vector3Scale(dir, spd * dt));
  }
  hero_.position = city_.clampWorld(hero_.position);

  int hx = 0;
  int hz = 0;
  city_.worldToCell(hero_.position, hx, hz);
  if (city_.cell(hx, hz) == CellKind::Restricted) {
    hero_.inRestricted = true;
    if (nextFloat() < 0.02f) {
      hero_.vanishTimer = 2.2f;
    }
  }

  if (hero_.actionTimer <= 0.0f) {
    hero_.actionTimer = 6.0f + nextFloat() * 8.0f;
    hero_.scheme = nextInt(0, 4);
    if (nextFloat() < 0.25f) {
      hero_.vanishTimer = 1.6f;
    }
  }
}

void Simulation::updateCars(float dt) {
  for (std::size_t i = 0; i < cars_.size(); ++i) {
    Car& c = cars_[i];
    if (c.stolen) {
      c.position = Vector3Add(c.position, Vector3{std::cos(c.heading) * 10.0f * dt, 0.0f, std::sin(c.heading) * 10.0f * dt});
      c.position = city_.clampWorld(c.position);
      continue;
    }
    const float spd = 4.0f;
    Vector3 n = Vector3Add(c.position, Vector3{std::cos(c.heading) * spd * dt, 0.0f, std::sin(c.heading) * spd * dt});
    int x = 0;
    int z = 0;
    city_.worldToCell(n, x, z);
    if (city_.cell(x, z) != CellKind::Road) {
      c.heading += 1.5708f;
      if (c.heading > 6.2832f) {
        c.heading -= 6.2832f;
      }
    } else {
      c.position = n;
    }
    c.position = city_.clampWorld(c.position);
  }
}

void Simulation::updateEvents(float dt) {
  for (WorldEvent& e : events_) {
    if (!e.active) {
      continue;
    }
    e.age += dt;
    if (e.age >= e.lifetime) {
      e.active = false;
    }
  }
  events_.erase(std::remove_if(events_.begin(), events_.end(),
                               [](const WorldEvent& e) { return !e.active && e.age > e.lifetime + 1.0f; }),
                events_.end());
}

void Simulation::gossip() {
  if (agents_.size() < 2) {
    return;
  }
  const int i = nextInt(0, static_cast<int>(agents_.size()) - 1);
  int j = nextInt(0, static_cast<int>(agents_.size()) - 1);
  if (i == j) {
    return;
  }
  Agent& a = agents_[static_cast<std::size_t>(i)];
  Agent& b = agents_[static_cast<std::size_t>(j)];
  if (a.missing || b.missing) {
    return;
  }
  if (Vector3Distance(a.position, b.position) > 5.0f) {
    return;
  }
  if (!a.memory.empty() && nextFloat() < 0.4f) {
    remember(b, a.memory.back().text, a.memory.back().aboutHero);
  }
}

void Simulation::update(float dt, bool playerWorking, bool playerSleeping) {
  dayTime_ += dt;
  if (dayTime_ >= kDayLength) {
    dayTime_ -= kDayLength;
    ++day_;
    ++player_.daysSurvived;
    resetDay();
  }

  if (!playerWorking && !playerSleeping) {
    updateAgents(dt);
    updateHero(dt);
    updateCars(dt);
    updateEvents(dt);
    if (nextFloat() < 0.08f * dt) {
      gossip();
    }
  } else {
    updateAgents(dt * 0.35f);
    updateHero(dt * 0.35f);
    updateCars(dt * 0.35f);
    updateEvents(dt);
  }

  eventCooldown_ -= dt;
  if (eventCooldown_ <= 0.0f) {
    spawnEvent();
    eventCooldown_ = 16.0f + nextFloat() * 14.0f;
  }

  observeWorld();
  playerFollowTick(dt);

  int px = 0;
  int pz = 0;
  city_.worldToCell(player_.position, px, pz);
  if (city_.cell(px, pz) == CellKind::Restricted) {
    player_.enteredRestricted = true;
    if (!player_.foundSeam && nextFloat() < 0.15f * dt + 0.002f) {
      player_.foundSeam = true;
      rememberPlayer("The alley wall flickered. The world has a seam.", false);
    }
  }
}

void Simulation::resetDay() {
  for (Agent& a : agents_) {
    if (a.missing) {
      continue;
    }
    a.path.clear();
    a.pathIndex = 0;
  }
  if (day_ == 3 && !player_.discoveredHero) {
    rememberPlayer("Coworkers keep mentioning a stranger in red.", true);
  }
}

void Simulation::observeWorld() {
  if (hero_.visible) {
    const float d = Vector3Distance(player_.position, hero_.position);
    if (d < 9.0f) {
      player_.discoveredHero = true;
      if (d < 5.5f && nextFloat() < 0.02f) {
        rememberPlayer("The person in red walked through a locked gate.", true);
        ++player_.crimesWitnessed;
      }
    }
  }
  for (const WorldEvent& e : events_) {
    if (!e.active) {
      continue;
    }
    if (Vector3Distance(player_.position, e.position) < 8.0f) {
      if (e.kind == EventKind::Robbery || e.kind == EventKind::Theft || e.kind == EventKind::Chase) {
        player_.discoveredHero = true;
      }
    }
  }
}

void Simulation::playerFollowTick(float dt) {
  if (hero_.visible && Vector3Distance(player_.position, hero_.position) < 7.0f) {
    followTimer_ += dt;
    if (followTimer_ > 3.0f) {
      ++player_.timesFollowedHero;
      followTimer_ = 0.0f;
      rememberPlayer("I kept the person in red in sight.", true);
    }
  } else {
    followTimer_ = std::max(0.0f, followTimer_ - dt);
  }
}

Agent* Simulation::nearestAgent(float radius) {
  Agent* best = nullptr;
  float bestD = radius;
  for (Agent& a : agents_) {
    if (a.missing) {
      continue;
    }
    const float d = Vector3Distance(player_.position, a.position);
    if (d < bestD) {
      bestD = d;
      best = &a;
    }
  }
  return best;
}

const Agent* Simulation::nearestAgent(float radius) const {
  const Agent* best = nullptr;
  float bestD = radius;
  for (const Agent& a : agents_) {
    if (a.missing) {
      continue;
    }
    const float d = Vector3Distance(player_.position, a.position);
    if (d < bestD) {
      bestD = d;
      best = &a;
    }
  }
  return best;
}

void Simulation::playerTalkNearby() {
  Agent* a = nearestAgent(4.0f);
  if (a == nullptr) {
    rememberPlayer("Nobody was close enough to talk.", false);
    return;
  }
  a->talkCooldown = 2.0f;
  if (!a->memory.empty()) {
    rememberPlayer(a->name + " said: " + a->memory.back().text, a->memory.back().aboutHero);
    a->lastLine = a->memory.back().text;
  } else if (a->suspicionHero > 0.3f) {
    a->lastLine = "That stranger in red does not belong here.";
    rememberPlayer(a->name + " said: " + a->lastLine, true);
  } else {
    a->lastLine = "I work as a " + jobName(a->job) + ". Same as always.";
    rememberPlayer(a->name + " said: " + a->lastLine, false);
  }
}

void Simulation::playerWarnNearby() {
  Agent* a = nearestAgent(4.5f);
  if (a == nullptr) {
    return;
  }
  ++player_.npcsWarned;
  a->suspicionHero = std::min(1.0f, a->suspicionHero + 0.3f);
  a->lastLine = "You really think someone is controlling them?";
  remember(*a, "Was warned that the stranger is not an NPC.", true);
  rememberPlayer("I warned " + a->name + " about the stranger.", true);
}

void Simulation::playerReport() {
  int px = 0;
  int pz = 0;
  city_.worldToCell(player_.position, px, pz);
  const bool nearStation = (std::abs(px - city_.policeX()) + std::abs(pz - city_.policeZ())) <= 3;
  if (!nearStation) {
    rememberPlayer("I should report this at the police station.", false);
    return;
  }
  ++player_.reportsFiled;
  rememberPlayer("I filed a report about the stranger in red.", true);
  for (Agent& a : agents_) {
    if (a.job == JobKind::Police) {
      remember(a, "A civilian reported a rogue actor in the city.", true);
    }
  }
}

void Simulation::playerHelpHero() {
  if (!hero_.visible || Vector3Distance(player_.position, hero_.position) > 6.0f) {
    rememberPlayer("The stranger was not close enough to help.", false);
    return;
  }
  ++player_.timesHelpedHero;
  hero_.vanishTimer = 2.5f;
  rememberPlayer("I covered for the stranger. They vanished between frames.", true);
}

EndingKind Simulation::evaluateEnding() const {
  if (player_.foundSeam && player_.discoveredHero && player_.daysSurvived >= 2) {
    return EndingKind::Escape;
  }
  if (player_.daysSurvived >= 4 && player_.timesFollowedHero == 0 && player_.reportsFiled == 0 &&
      !player_.discoveredHero) {
    return EndingKind::NormalLife;
  }
  int aware = 0;
  for (const Agent& a : agents_) {
    if (a.suspicionHero > 0.5f) {
      ++aware;
    }
  }
  if (aware >= 8 && player_.npcsWarned >= 4) {
    return EndingKind::HeroEnding;
  }
  if (player_.timesFollowedHero >= 4 && player_.reportsFiled >= 1) {
    return EndingKind::Hunter;
  }
  if (player_.discoveredHero && player_.journal.size() >= 6) {
    return EndingKind::Investigator;
  }
  if (player_.daysSurvived >= 5 && hero_.crimesCommitted >= 4) {
    return EndingKind::Reset;
  }
  return EndingKind::None;
}

std::string Simulation::endingTitle(EndingKind kind) const {
  switch (kind) {
    case EndingKind::NormalLife:
      return "NORMAL LIFE";
    case EndingKind::Investigator:
      return "THE INVESTIGATOR";
    case EndingKind::Hunter:
      return "THE HUNTER";
    case EndingKind::HeroEnding:
      return "THE HERO";
    case EndingKind::Escape:
      return "ESCAPE";
    case EndingKind::Reset:
      return "RESET";
    default:
      return "";
  }
}

std::string Simulation::endingBody(EndingKind kind) const {
  switch (kind) {
    case EndingKind::NormalLife:
      return "You kept your routine. The city never asked more of you.\nYou never discovered what was really happening.";
    case EndingKind::Investigator:
      return "You watched, asked, and pieced it together.\nSomeone in this city is being controlled.";
    case EndingKind::Hunter:
      return "You tracked the stranger and interfered with their story.\nThe hero of this world now has an enemy.";
    case EndingKind::HeroEnding:
      return "The others started listening to you.\nThey no longer see you as just another NPC.";
    case EndingKind::Escape:
      return "You found a seam in the world.\nThe simulation noticed you noticing.";
    case EndingKind::Reset:
      return "The player's story reached its conclusion.\nEverything resets. Everyone forgets. Except you.";
    default:
      return "";
  }
}
