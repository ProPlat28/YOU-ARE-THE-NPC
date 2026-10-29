#pragma once

#include "raylib.h"
#include "raymath.h"

#include <string>
#include <vector>

enum class JobKind {
  Office = 0,
  Cafe,
  Shop,
  Police,
  Unemployed
};

enum class EndingKind {
  None = 0,
  NormalLife,
  Investigator,
  Hunter,
  HeroEnding,
  Escape,
  Reset
};

enum class EventKind {
  Accident = 0,
  Robbery,
  Fight,
  Fire,
  Disappearance,
  Chase,
  Theft
};

enum class CellKind {
  Road = 0,
  Building,
  Park,
  Plaza,
  Restricted
};

struct MemoryFact {
  std::string text;
  int day;
  bool aboutHero;
};

struct Agent {
  int id;
  std::string name;
  Vector3 position;
  float heading;
  float speed;
  Color body;
  Color shirt;
  int homeX;
  int homeZ;
  int workX;
  int workZ;
  JobKind job;
  std::vector<MemoryFact> memory;
  float trust;
  float suspicionHero;
  bool missing;
  std::vector<Vector3> path;
  std::size_t pathIndex;
  float talkCooldown;
  std::string lastLine;
  float waitTimer;
};

struct Car {
  Vector3 position;
  float heading;
  Color color;
  bool stolen;
};

struct WorldEvent {
  EventKind kind;
  Vector3 position;
  float age;
  float lifetime;
  std::string description;
  bool active;
};

struct PlayerState {
  Vector3 position;
  float heading;
  float speed;
  float stamina;
  int homeX;
  int homeZ;
  int workX;
  int workZ;
  JobKind job;
  std::vector<MemoryFact> journal;
  float trustCity;
  int crimesWitnessed;
  int npcsWarned;
  int reportsFiled;
  int timesFollowedHero;
  int timesHelpedHero;
  int daysSurvived;
  bool enteredRestricted;
  bool foundSeam;
  bool discoveredHero;
  bool skippedWork;
};

struct HeroState {
  Vector3 position;
  float heading;
  Color shirt;
  float actionTimer;
  int scheme;
  bool inRestricted;
  bool visible;
  float vanishTimer;
  int crimesCommitted;
  bool robberyDone;
};

inline Vector3 xz(float x, float z, float y = 0.0f) {
  return Vector3{x, y, z};
}
