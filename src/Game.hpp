#pragma once

#include "City.hpp"
#include "Simulation.hpp"

#include "raylib.h"

#include <string>

class Game {
public:
  Game();
  ~Game();

  void run();

private:
  City city_;
  Simulation sim_;
  Camera3D camera_;
  bool running_;
  bool paused_;
  bool showHelp_;
  EndingKind ending_;
  float yaw_;
  float pitch_;
  float camDist_;
  std::string banner_;
  float bannerTimer_;

  void update(float dt);
  void draw();
  void drawHud();
  void drawEnding();
  void handleInput(float dt);
  void setBanner(const std::string& text);
};
