/*
Decade
Copyright (c) 2019-2022 Marco Peyer

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

// #include "../casts.hpp"

#include <sigslot/signal.hpp>

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <glm/vec4.hpp>

#include <array>
// #include <exception>
#include <random>
#include <string>
#include <vector>

class ShapeConfiguration {
public:
  ShapeConfiguration() : name(""), outline_visible(true), fill_visible(false), line_width(1.f)
  {
    this->outline_color = {0.f, 0.f, 0.f, 1.f};
    this->fill_color = {0.f, 0.f, 0.f, 1.f};
  }

  ShapeConfiguration(const std::string &name, bool outline_visible, bool fill_visible,
                     float line_width, const glm::vec4 &outline_color, const glm::vec4 &fill_color)
      : name(name), outline_visible(outline_visible), fill_visible(fill_visible),
        line_width(line_width)
  {
    this->outline_color = {outline_color[0], outline_color[1], outline_color[2], outline_color[3]};
    this->fill_color = {fill_color[0], fill_color[1], fill_color[2], fill_color[3]};
  }

  std::string Name() const { return name; }

  void FillVisible(bool value) { fill_visible = value; }

  void OutlineVisible(bool value) { outline_visible = value; }

  bool FillVisible() const { return fill_visible; }

  bool OutlineVisible() const { return outline_visible; }

  void LineWidth(float value) { line_width = value; }

  void OutlineColor(const glm::vec4 &value)
  {
    outline_color = {value[0], value[1], value[2], value[3]};
  }

  void FillColor(const glm::vec4 &value) { fill_color = {value[0], value[1], value[2], value[3]}; }

  float LineWidth() const
  {
    float return_value = 0.f;

    if (outline_visible == true) {
      return_value = line_width;
    }

    return return_value;
  }

  glm::vec4 OutlineColor() const
  {
    glm::vec4 value = glm::vec4{0.f, 0.f, 0.f, 0.f};

    if (outline_visible == true) {
      value = {outline_color[0], outline_color[1], outline_color[2], outline_color[3]};
    }

    return value;
  }

  glm::vec4 FillColor() const
  {
    glm::vec4 value = glm::vec4{0.f, 0.f, 0.f, 0.f};

    if (fill_visible == true) {
      value = {fill_color[0], fill_color[1], fill_color[2], fill_color[3]};
    }

    return value;
  }

  float LineWidthDisabled() const { return line_width; }

  glm::vec4 OutlineColorDisabled() const
  {
    return {outline_color[0], outline_color[1], outline_color[2], outline_color[3]};
  }

  glm::vec4 FillColorDisabled() const
  {
    return {fill_color[0], fill_color[1], fill_color[2], fill_color[3]};
  }

  void RandomColor()
  {
    const float alpha_channel = 0.5f;

    std::random_device rd;
    std::uniform_real_distribution<float> dist(0.f, 1.f);

    fill_color = std::array<float, 4>{dist(rd), dist(rd), dist(rd), alpha_channel};
    outline_color = fill_color;
  }

  bool operator==(const std::string &compare) const { return Name() == compare; }

private:
  std::string name;
  bool outline_visible;
  bool fill_visible;
  float line_width;
  std::array<float, 4> outline_color;
  std::array<float, 4> fill_color;

  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const unsigned int version)
  {
    ar &BOOST_SERIALIZATION_NVP(name);
    ar &BOOST_SERIALIZATION_NVP(outline_visible);
    ar &BOOST_SERIALIZATION_NVP(fill_visible);
    ar &BOOST_SERIALIZATION_NVP(line_width);
    ar &BOOST_SERIALIZATION_NVP(outline_color);
    ar &BOOST_SERIALIZATION_NVP(fill_color);
  }
};

class ShapeConfigurationStorage {
public:
  ShapeConfigurationStorage()
  {
    shape_configurations.emplace_back("Page Margin", true, false, 0.2f,
                                      glm::vec4(0.f, 0.f, 0.f, 1.f), glm::vec4(1.f, 1.f, 1.f, 0.f));
    shape_configurations.emplace_back("Title Frame", true, false, 0.5f,
                                      glm::vec4(0.1f, 0.1f, 0.1f, 1.f),
                                      glm::vec4(1.f, 1.f, 1.f, 0.f));
    shape_configurations.emplace_back("Calendar Labels", true, false, 0.1f,
                                      glm::vec4(0.75f, 0.75f, 0.75f, 0.25f),
                                      glm::vec4(0.f, 0.f, 0.f, 1.f));
    shape_configurations.emplace_back("Day Shapes", true, true, 0.2f,
                                      glm::vec4(0.85f, 0.85f, 0.85f, 1.f),
                                      glm::vec4(0.f, 0.f, 0.f, 0.f));
    shape_configurations.emplace_back("Sunday Shapes", true, true, 0.2f,
                                      glm::vec4(0.85f, 0.85f, 0.85f, 1.f),
                                      glm::vec4(0.85f, 0.85f, 0.85f, 1.f));
    shape_configurations.emplace_back("Months Shapes", true, false, 0.2f,
                                      glm::vec4(0.4f, 0.4f, 0.4f, 1.0f),
                                      glm::vec4(0.5f, 0.5f, 0.5f, 0.f));
    shape_configurations.emplace_back("Years Shapes", false, false, 0.2f,
                                      glm::vec4(0.25f, 0.25f, 0.25f, 1.f),
                                      glm::vec4(0.25f, 0.25f, 0.25f, 0.f));
    shape_configurations.emplace_back("Years Totals", false, true, 0.2f,
                                      glm::vec4(0.25f, 0.75f, 0.25f, 1.f),
                                      glm::vec4(0.25f, 0.75f, 0.25f, 1.f));

    number_persistent_configurations = shape_configurations.size();
  }

  void
  ReceiveShapeConfigurationStorage(const ShapeConfigurationStorage &shape_configuration_storage)
  {
    *this = shape_configuration_storage;
    SendShapeConfigurationStorage();
  }

  void SendShapeConfigurationStorage() { signal_shape_configuration_storage(*this); }

  ShapeConfigurationStorage &operator=(const ShapeConfigurationStorage &other)
  {
    shape_configurations = other.shape_configurations;
    number_persistent_configurations = other.number_persistent_configurations;
    return *this;
  }

  size_t size() const { return shape_configurations.size(); }

  void resize(const size_t size) { shape_configurations.resize(size); }

  ShapeConfiguration &operator[](const size_t index) { return shape_configurations[index]; }

  ShapeConfiguration GetShapeConfiguration(const std::string &name)
  {
    auto found = std::find(shape_configurations.begin(), shape_configurations.end(), name);

    if (found == shape_configurations.end()) {
      // throw std::runtime_error("Can't find shape configuration!");
      return ShapeConfiguration();
    } else {
      return *found;
    }
  }

  size_t GetNumberPersistentConfigurations() { return number_persistent_configurations; }

  sigslot::signal<const ShapeConfigurationStorage &> signal_shape_configuration_storage;

private:
  std::vector<ShapeConfiguration> shape_configurations;
  size_t number_persistent_configurations;

  friend class boost::serialization::access;
  template <class Archive> void save(Archive &ar, const unsigned int version) const
  {
    ar &BOOST_SERIALIZATION_NVP(shape_configurations);
    ar &BOOST_SERIALIZATION_NVP(number_persistent_configurations);
  }
  template <class Archive> void load(Archive &ar, const unsigned int version)
  {
    ar &BOOST_SERIALIZATION_NVP(shape_configurations);
    ar &BOOST_SERIALIZATION_NVP(number_persistent_configurations);
    SendShapeConfigurationStorage();
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
};
