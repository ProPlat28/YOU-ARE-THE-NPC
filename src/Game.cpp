#include "Game.hpp"

#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <sstream>

Game::Game()
    : city_(),
      sim_(city_),
      running_(true),
      paused_(false),
      showHelp_(true),
      ending_(EndingKind::None),
      yaw_(0.85f),
      pitch_(0.42f),
      camDist_(18.0f),
      banner_("You are not the hero. You are the NPC."),
      bannerTimer_(5.0f) {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(1280, 720, "YOU ARE THE NPC");
  SetTargetFPS(60);
  SetExitKey(KEY_NULL);

  camera_.position = Vector3{20.0f, 16.0f, 20.0f};
  camera_.target = Vector3{20.0f, 1.0f, 20.0f};
  camera_.up = Vector3{0.0f, 1.0f, 0.0f};
  camera_.fovy = 55.0f;
  camera_.projection = CAMERA_PERSPECTIVE;
}

Game::~Game() {
  if (IsWindowReady()) {
    CloseWindow();
  }
}

void Game::run() {
  while (running_ && !WindowShouldClose()) {
    const float dt = std::min(GetFrameTime(), 0.05f);
    update(dt);
    draw();
  }
}

void Game::setBanner(const std::string& text) {
  banner_ = text;
  bannerTimer_ = 4.0f;
}

void Game::handleInput(float dt) {
  if (IsKeyPressed(KEY_ESCAPE)) {
    if (ending_ != EndingKind::None) {
      running_ = false;
      return;
    }
    paused_ = !paused_;
  }
  if (IsKeyPressed(KEY_F1) || IsKeyPressed(KEY_H)) {
    showHelp_ = !showHelp_;
  }
  if (ending_ != EndingKind::None) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
      running_ = false;
    }
    return;
  }
  if (paused_) {
    return;
  }

  if (IsKeyDown(KEY_Q)) {
    yaw_ -= 1.4f * dt;
  }
  if (IsKeyDown(KEY_E)) {
    yaw_ += 1.4f * dt;
  }
  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    yaw_ += GetMouseDelta().x * 0.008f;
    pitch_ += GetMouseDelta().y * 0.006f;
  }
  pitch_ = std::clamp(pitch_, 0.18f, 1.15f);
  camDist_ -= GetMouseWheelMove() * 1.6f;
  camDist_ = std::clamp(camDist_, 8.0f, 42.0f);

  PlayerState& p = sim_.player();
  Vector3 move{0.0f, 0.0f, 0.0f};
  const float cy = std::cos(yaw_);
  const float sy = std::sin(yaw_);
  Vector3 forward{sy, 0.0f, cy};
  Vector3 right{cy, 0.0f, -sy};
  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
    move = Vector3Add(move, forward);
  }
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
    move = Vector3Subtract(move, forward);
  }
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
    move = Vector3Add(move, right);
  }
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
    move = Vector3Subtract(move, right);
  }
  const float len = Vector3Length(move);
  float spd = p.speed;
  if (IsKeyDown(KEY_LEFT_SHIFT)) {
    spd *= 1.7f;
  }
  if (len > 0.01f) {
    move = Vector3Scale(move, 1.0f / len);
    p.heading = std::atan2(move.z, move.x);
    p.position = Vector3Add(p.position, Vector3Scale(move, spd * dt));
    p.position = city_.clampWorld(p.position);
  }

  if (IsKeyPressed(KEY_F)) {
    sim_.playerTalkNearby();
    const Agent* a = sim_.nearestAgent(4.0f);
    if (a != nullptr) {
      setBanner(a->name + ": " + a->lastLine);
    } else {
      setBanner("Nobody nearby.");
    }
  }
  if (IsKeyPressed(KEY_G)) {
    sim_.playerWarnNearby();
    setBanner("You whispered a warning.");
  }
  if (IsKeyPressed(KEY_R)) {
    sim_.playerReport();
    setBanner("You tried to file a report.");
  }
  if (IsKeyPressed(KEY_T)) {
    sim_.playerHelpHero();
    setBanner("You tried to cover for the stranger.");
  }
  if (IsKeyPressed(KEY_N)) {
    ending_ = sim_.evaluateEnding();
    if (ending_ == EndingKind::None) {
      setBanner("The story is not finished. Keep living.");
    }
  }
}

void Game::update(float dt) {
  handleInput(dt);
  if (paused_ || ending_ != EndingKind::None) {
    return;
  }

  sim_.update(dt, false, false);

  const Vector3 focus = sim_.player().position;
  const Vector3 camPos{
      focus.x - std::sin(yaw_) * camDist_ * std::cos(pitch_),
      focus.y + 2.0f + std::sin(pitch_) * camDist_,
      focus.z - std::cos(yaw_) * camDist_ * std::cos(pitch_)};
  camera_.position = camPos;
  camera_.target = Vector3{focus.x, 1.2f, focus.z};

  if (bannerTimer_ > 0.0f) {
    bannerTimer_ -= dt;
  }

  if (sim_.day() >= 2) {
    const EndingKind maybe = sim_.evaluateEnding();
    if (maybe != EndingKind::None && IsKeyPressed(KEY_N)) {
      ending_ = maybe;
    }
  }
}

void Game::drawHud() {
  const int w = GetScreenWidth();
  DrawRectangle(0, 0, w, 78, Color{8, 10, 14, 210});
  DrawText("YOU ARE THE NPC", 18, 10, 22, Color{210, 220, 230, 255});
  DrawText(sim_.clockText().c_str(), 18, 38, 18, Color{180, 190, 200, 255});
  DrawText(sim_.weatherText().c_str(), 220, 38, 18, Color{160, 170, 190, 255});

  std::ostringstream stats;
  stats << "Witnessed " << sim_.player().crimesWitnessed << "  Warned " << sim_.player().npcsWarned
        << "  Reports " << sim_.player().reportsFiled << "  Followed " << sim_.player().timesFollowedHero;
  DrawText(stats.str().c_str(), 360, 38, 18, Color{150, 170, 160, 255});

  if (bannerTimer_ > 0.0f && !banner_.empty()) {
    const int tw = MeasureText(banner_.c_str(), 20);
    DrawRectangle((w - tw) / 2 - 16, 92, tw + 32, 36, Color{12, 14, 20, 200});
    DrawText(banner_.c_str(), (w - tw) / 2, 100, 20, Color{230, 220, 190, 255});
  }

  const Agent* near = sim_.nearestAgent(5.0f);
  if (near != nullptr) {
    DrawText((near->name + "  [" + near->lastLine + "]").c_str(), 18, 108, 16,
             Color{200, 200, 210, 255});
  }

  DrawRectangle(16, GetScreenHeight() - 168, 420, 152, Color{8, 10, 14, 200});
  DrawText("JOURNAL", 28, GetScreenHeight() - 158, 16, Color{200, 190, 150, 255});
  const auto& journal = sim_.player().journal;
  int y = GetScreenHeight() - 136;
  const int start = journal.size() > 5 ? static_cast<int>(journal.size()) - 5 : 0;
  for (int i = start; i < static_cast<int>(journal.size()); ++i) {
    const MemoryFact& f = journal[static_cast<std::size_t>(i)];
    DrawText(("D" + std::to_string(f.day) + "  " + f.text).c_str(), 28, y, 14,
             f.aboutHero ? Color{220, 140, 120, 255} : Color{180, 185, 195, 255});
    y += 22;
  }

  if (showHelp_) {
    DrawRectangle(w - 360, 90, 344, 210, Color{8, 10, 14, 210});
    DrawText("WASD  walk     Shift sprint", w - 344, 104, 16, RAYWHITE);
    DrawText("Q/E or RMB  orbit camera", w - 344, 126, 16, RAYWHITE);
    DrawText("Wheel  zoom", w - 344, 148, 16, RAYWHITE);
    DrawText("F  talk     G  warn NPC", w - 344, 170, 16, RAYWHITE);
    DrawText("R  report at police", w - 344, 192, 16, RAYWHITE);
    DrawText("T  help the stranger", w - 344, 214, 16, RAYWHITE);
    DrawText("N  try an ending", w - 344, 236, 16, RAYWHITE);
    DrawText("H  toggle help   Esc pause", w - 344, 258, 16, RAYWHITE);
    DrawText("Blue is you. Red mark is them.", w - 344, 280, 16, Color{180, 160, 160, 255});
  }

  if (paused_) {
    DrawRectangle(0, 0, w, GetScreenHeight(), Color{0, 0, 0, 120});
    DrawText("PAUSED", w / 2 - 70, GetScreenHeight() / 2 - 20, 40, RAYWHITE);
  }
}

void Game::drawEnding() {
  DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{6, 7, 10, 230});
  const std::string title = sim_.endingTitle(ending_);
  const std::string body = sim_.endingBody(ending_);
  const int tw = MeasureText(title.c_str(), 42);
  DrawText(title.c_str(), GetScreenWidth() / 2 - tw / 2, 180, 42, Color{230, 210, 170, 255});

  int y = 260;
  std::string line;
  std::istringstream iss(body);
  while (std::getline(iss, line)) {
    const int lw = MeasureText(line.c_str(), 22);
    DrawText(line.c_str(), GetScreenWidth() / 2 - lw / 2, y, 22, Color{200, 200, 210, 255});
    y += 32;
  }
  DrawText("Enter / Space / Esc to leave", GetScreenWidth() / 2 - 160, y + 40, 20,
           Color{140, 145, 155, 255});
}

void Game::draw() {
  BeginDrawing();
  Color sky = Color{18, 24, 38, 255};
  if (sim_.weatherText() == "Morning") {
    sky = Color{90, 120, 160, 255};
  } else if (sim_.weatherText() == "Afternoon") {
    sky = Color{70, 130, 190, 255};
  } else if (sim_.weatherText() == "Evening") {
    sky = Color{160, 90, 70, 255};
  }
  ClearBackground(sky);

  BeginMode3D(camera_);
  city_.draw(sim_.player(), sim_.hero(), sim_.agents(), sim_.cars(), sim_.events());
  EndMode3D();

  if (ending_ != EndingKind::None) {
    drawEnding();
  } else {
    drawHud();
  }
  DrawFPS(GetScreenWidth() - 90, 12);
  EndDrawing();
}
