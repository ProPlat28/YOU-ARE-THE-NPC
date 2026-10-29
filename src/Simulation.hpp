#pragma once

#include "City.hpp"
#include "Types.hpp"

#include <string>
#include <vector>

class Simulation {
public:
  explicit Simulation(City& city);

  void update(float dt, bool playerWorking, bool playerSleeping);
  void resetDay();

  PlayerState& player() { return player_; }
  const PlayerState& player() const { return player_; }
  HeroState& hero() { return hero_; }
  const HeroState& hero() const { return hero_; }
  std::vector<Agent>& agents() { return agents_; }
  const std::vector<Agent>& agents() const { return agents_; }
  const std::vector<Car>& cars() const { return cars_; }
  const std::vector<WorldEvent>& events() const { return events_; }

  float dayTime() const { return dayTime_; }
  int day() const { return day_; }
  std::string clockText() const;
  std::string weatherText() const;

  void playerTalkNearby();
  void playerWarnNearby();
  void playerReport();
  void playerHelpHero();
  void playerFollowTick(float dt);
  void observeWorld();
  EndingKind evaluateEnding() const;
  std::string endingTitle(EndingKind kind) const;
  std::string endingBody(EndingKind kind) const;

  Agent* nearestAgent(float radius);
  const Agent* nearestAgent(float radius) const;

private:
  City& city_;
  PlayerState player_;
  HeroState hero_;
  std::vector<Agent> agents_;
  std::vector<Car> cars_;
  std::vector<WorldEvent> events_;
  float dayTime_;
  int day_;
  float eventCooldown_;
  float followTimer_;
  unsigned seed_;

  unsigned nextRand();
  float nextFloat();
  int nextInt(int lo, int hi);
  void spawnAgents();
  void spawnCars();
  void spawnEvent();
  void updateAgents(float dt);
  void updateHero(float dt);
  void updateCars(float dt);
  void updateEvents(float dt);
  void assignPath(Agent& a, int tx, int tz);
  void stepAlongPath(Agent& a, float dt);
  Vector3 desiredTarget(const Agent& a) const;
  void remember(Agent& a, const std::string& text, bool aboutHero);
  void rememberPlayer(const std::string& text, bool aboutHero);
  void gossip();
  int hour() const;
};
