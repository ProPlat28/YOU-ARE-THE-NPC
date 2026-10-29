#pragma once

#include "Types.hpp"

#include <cstdint>
#include <vector>

class City {
public:
  static constexpr int kSize = 24;
  static constexpr float kCell = 8.0f;

  City();

  void draw(const PlayerState& player, const HeroState& hero, const std::vector<Agent>& agents,
            const std::vector<Car>& cars, const std::vector<WorldEvent>& events) const;

  CellKind cell(int x, int z) const;
  bool walkable(int x, int z) const;
  bool inBounds(int x, int z) const;
  Vector3 cellCenter(int x, int z, float y = 0.0f) const;
  void worldToCell(Vector3 p, int& x, int& z) const;
  Vector3 clampWorld(Vector3 p) const;
  float heightAt(int x, int z) const;
  Color buildingColor(int x, int z) const;
  Vector3 findNearestWalkable(Vector3 p) const;

  int plazaX() const { return plazaX_; }
  int plazaZ() const { return plazaZ_; }
  int bankX() const { return bankX_; }
  int bankZ() const { return bankZ_; }
  int policeX() const { return policeX_; }
  int policeZ() const { return policeZ_; }
  int cafeX() const { return cafeX_; }
  int cafeZ() const { return cafeZ_; }
  int shopX() const { return shopX_; }
  int shopZ() const { return shopZ_; }
  int officeX() const { return officeX_; }
  int officeZ() const { return officeZ_; }
  int restrictedX() const { return restrictedX_; }
  int restrictedZ() const { return restrictedZ_; }

private:
  std::vector<std::uint8_t> grid_;
  std::vector<float> heights_;
  std::vector<Color> colors_;
  int plazaX_;
  int plazaZ_;
  int bankX_;
  int bankZ_;
  int policeX_;
  int policeZ_;
  int cafeX_;
  int cafeZ_;
  int shopX_;
  int shopZ_;
  int officeX_;
  int officeZ_;
  int restrictedX_;
  int restrictedZ_;

  int index(int x, int z) const { return z * kSize + x; }
  void setCell(int x, int z, CellKind kind, float height, Color color);
};
