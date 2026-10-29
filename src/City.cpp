#include "City.hpp"

#include <algorithm>
#include <cmath>

City::City()
    : grid_(static_cast<std::size_t>(kSize * kSize), static_cast<std::uint8_t>(CellKind::Road)),
      heights_(static_cast<std::size_t>(kSize * kSize), 0.0f),
      colors_(static_cast<std::size_t>(kSize * kSize), Color{70, 70, 78, 255}),
      plazaX_(12),
      plazaZ_(12),
      bankX_(16),
      bankZ_(8),
      policeX_(7),
      policeZ_(7),
      cafeX_(10),
      cafeZ_(16),
      shopX_(18),
      shopZ_(16),
      officeX_(8),
      officeZ_(18),
      restrictedX_(20),
      restrictedZ_(4) {
  for (int z = 0; z < kSize; ++z) {
    for (int x = 0; x < kSize; ++x) {
      const bool road = (x % 4 == 0) || (z % 4 == 0);
      if (road) {
        setCell(x, z, CellKind::Road, 0.0f, Color{62, 64, 70, 255});
      } else {
        const float h = 4.0f + static_cast<float>((x * 13 + z * 7) % 18);
        const unsigned hue = static_cast<unsigned>((x * 37 + z * 19) % 40);
        const Color c{
            static_cast<unsigned char>(90 + hue),
            static_cast<unsigned char>(95 + (hue / 2)),
            static_cast<unsigned char>(110 + (hue % 20)),
            255};
        setCell(x, z, CellKind::Building, h, c);
      }
    }
  }

  for (int z = 10; z <= 14; ++z) {
    for (int x = 10; x <= 14; ++x) {
      setCell(x, z, CellKind::Plaza, 0.0f, Color{118, 116, 108, 255});
    }
  }

  for (int z = 2; z <= 5; ++z) {
    for (int x = 2; x <= 5; ++x) {
      setCell(x, z, CellKind::Park, 0.0f, Color{48, 92, 54, 255});
    }
  }

  setCell(bankX_, bankZ_, CellKind::Building, 14.0f, Color{170, 150, 90, 255});
  setCell(policeX_, policeZ_, CellKind::Building, 10.0f, Color{70, 90, 160, 255});
  setCell(cafeX_, cafeZ_, CellKind::Building, 6.0f, Color{150, 90, 70, 255});
  setCell(shopX_, shopZ_, CellKind::Building, 7.0f, Color{90, 130, 90, 255});
  setCell(officeX_, officeZ_, CellKind::Building, 16.0f, Color{80, 95, 120, 255});
  setCell(restrictedX_, restrictedZ_, CellKind::Restricted, 18.0f, Color{40, 30, 50, 255});
  setCell(restrictedX_ + 1, restrictedZ_, CellKind::Restricted, 12.0f, Color{36, 28, 46, 255});
}

void City::setCell(int x, int z, CellKind kind, float height, Color color) {
  if (!inBounds(x, z)) {
    return;
  }
  const int i = index(x, z);
  grid_[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(kind);
  heights_[static_cast<std::size_t>(i)] = height;
  colors_[static_cast<std::size_t>(i)] = color;
}

CellKind City::cell(int x, int z) const {
  if (!inBounds(x, z)) {
    return CellKind::Building;
  }
  return static_cast<CellKind>(grid_[static_cast<std::size_t>(index(x, z))]);
}

bool City::walkable(int x, int z) const {
  const CellKind k = cell(x, z);
  return k == CellKind::Road || k == CellKind::Plaza || k == CellKind::Park;
}

bool City::inBounds(int x, int z) const {
  return x >= 0 && z >= 0 && x < kSize && z < kSize;
}

Vector3 City::cellCenter(int x, int z, float y) const {
  return Vector3{static_cast<float>(x) * kCell + kCell * 0.5f, y,
                 static_cast<float>(z) * kCell + kCell * 0.5f};
}

void City::worldToCell(Vector3 p, int& x, int& z) const {
  x = static_cast<int>(std::floor(p.x / kCell));
  z = static_cast<int>(std::floor(p.z / kCell));
}

Vector3 City::clampWorld(Vector3 p) const {
  const float max = static_cast<float>(kSize) * kCell - 0.5f;
  p.x = std::clamp(p.x, 0.5f, max);
  p.z = std::clamp(p.z, 0.5f, max);
  p.y = 0.0f;
  return p;
}

float City::heightAt(int x, int z) const {
  if (!inBounds(x, z)) {
    return 0.0f;
  }
  return heights_[static_cast<std::size_t>(index(x, z))];
}

Color City::buildingColor(int x, int z) const {
  if (!inBounds(x, z)) {
    return GRAY;
  }
  return colors_[static_cast<std::size_t>(index(x, z))];
}

Vector3 City::findNearestWalkable(Vector3 p) const {
  int cx = 0;
  int cz = 0;
  worldToCell(p, cx, cz);
  if (walkable(cx, cz)) {
    return cellCenter(cx, cz);
  }
  for (int r = 1; r < kSize; ++r) {
    for (int dz = -r; dz <= r; ++dz) {
      for (int dx = -r; dx <= r; ++dx) {
        if (walkable(cx + dx, cz + dz)) {
          return cellCenter(cx + dx, cz + dz);
        }
      }
    }
  }
  return cellCenter(plazaX_, plazaZ_);
}

void City::draw(const PlayerState& player, const HeroState& hero, const std::vector<Agent>& agents,
                const std::vector<Car>& cars, const std::vector<WorldEvent>& events) const {
  const float world = static_cast<float>(kSize) * kCell;
  DrawPlane(Vector3{world * 0.5f, 0.0f, world * 0.5f}, Vector2{world + 40.0f, world + 40.0f},
            Color{28, 32, 38, 255});

  for (int z = 0; z < kSize; ++z) {
    for (int x = 0; x < kSize; ++x) {
      const Vector3 c = cellCenter(x, z);
      const CellKind k = cell(x, z);
      const Color col = buildingColor(x, z);
      if (k == CellKind::Building || k == CellKind::Restricted) {
        const float h = heightAt(x, z);
        DrawCube(Vector3{c.x, h * 0.5f, c.z}, kCell * 0.88f, h, kCell * 0.88f, col);
        DrawCubeWires(Vector3{c.x, h * 0.5f, c.z}, kCell * 0.88f, h, kCell * 0.88f,
                      Color{20, 20, 24, 255});
      } else {
        DrawCube(Vector3{c.x, 0.04f, c.z}, kCell, 0.08f, kCell, col);
        if (k == CellKind::Road && (x % 4 == 0)) {
          DrawCube(Vector3{c.x, 0.06f, c.z}, 0.18f, 0.02f, kCell * 0.6f, Color{210, 210, 180, 255});
        }
        if (k == CellKind::Park) {
          DrawCube(Vector3{c.x, 1.2f, c.z}, 0.4f, 2.4f, 0.4f, Color{70, 48, 30, 255});
          DrawSphere(Vector3{c.x, 2.6f, c.z}, 1.1f, Color{36, 110, 48, 255});
        }
      }
    }
  }

  DrawCube(cellCenter(bankX_, bankZ_, 15.4f), 2.2f, 1.0f, 2.2f, GOLD);
  DrawCube(cellCenter(policeX_, policeZ_, 11.2f), 1.6f, 1.2f, 1.6f, BLUE);
  DrawCube(cellCenter(restrictedX_, restrictedZ_, 19.2f), 2.0f, 1.4f, 2.0f, Color{180, 40, 80, 255});

  for (const Car& car : cars) {
    DrawCube(Vector3{car.position.x, 0.55f, car.position.z}, 2.4f, 0.9f, 1.3f, car.color);
    DrawCube(Vector3{car.position.x, 1.05f, car.position.z}, 1.3f, 0.5f, 1.15f, Color{30, 30, 34, 200});
  }

  for (const WorldEvent& ev : events) {
    if (!ev.active) {
      continue;
    }
    const Color c = (ev.kind == EventKind::Fire)      ? ORANGE
                    : (ev.kind == EventKind::Robbery) ? RED
                    : (ev.kind == EventKind::Fight)   ? MAROON
                    : (ev.kind == EventKind::Chase)   ? BLUE
                                                      : YELLOW;
    DrawCylinder(Vector3{ev.position.x, 0.0f, ev.position.z}, 1.6f, 0.2f, 3.5f, 10, Fade(c, 0.35f));
  }

  auto drawPerson = [](Vector3 p, Color shirt, Color body, float heading, float scale) {
    DrawCube(Vector3{p.x, 0.7f * scale, p.z}, 0.55f * scale, 1.15f * scale, 0.4f * scale, shirt);
    DrawSphere(Vector3{p.x, 1.45f * scale, p.z}, 0.28f * scale, body);
    const Vector3 nose{
        p.x + std::cos(heading) * 0.32f * scale, 1.45f * scale,
        p.z + std::sin(heading) * 0.32f * scale};
    DrawSphere(nose, 0.08f * scale, Color{20, 20, 20, 255});
  };

  for (const Agent& a : agents) {
    if (a.missing) {
      continue;
    }
    drawPerson(a.position, a.shirt, a.body, a.heading, 1.0f);
  }

  if (hero.visible) {
    drawPerson(hero.position, hero.shirt, Color{230, 210, 190, 255}, hero.heading, 1.08f);
    DrawCube(Vector3{hero.position.x, 2.3f, hero.position.z}, 0.18f, 0.18f, 0.18f, RED);
  }

  drawPerson(player.position, Color{70, 140, 190, 255}, Color{220, 190, 160, 255}, player.heading,
             1.05f);
  DrawCube(Vector3{player.position.x, 2.25f, player.position.z}, 0.16f, 0.16f, 0.16f, SKYBLUE);
}
