#ifndef SCENE_SNAPSHOT_HPP
#define SCENE_SNAPSHOT_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Kind of shape a scene node carries, as a GL-free enum so the presentation
// layer can describe a node without depending on the OpenGL shape types in
// `src/infrastructure/graphics/`.
enum class SnapshotShapeKind : std::uint8_t {
  kNone,
  kQuadrilateral,
  kRectangles,
  kFont,
};

// Achsenparallele Box in Seitenmillimetern. Eigener Typ statt des rectf aus
// `infrastructure/graphics/`, damit das Lesemodell dort keine Abhängigkeit
// hinterlässt.
struct SnapshotBounds {
  float left{0.0F};
  float right{0.0F};
  float bottom{0.0F};
  float top{0.0F};
};

// Was ein Textknoten über seinen Inhalt hinaus zeigt: der gezeichnete Text und
// die Geviertgrösse, mit der er gesetzt wurde. Kindspezifisch, aber nicht
// knotenspezifisch — jeder Font-Knoten trägt es.
struct SnapshotTextDetail {
  std::string text;
  float size_millimetres{0.0F};
};

// Application-layer read model of the render scene graph: a plain, GL-free
// mirror of the SceneNode hierarchy. It exists so the presentation layer (the
// scene-tree widget) can display the graph structure and per-node detail
// without depending on the OpenGL `SceneNode` type in
// `src/infrastructure/graphics/`. The rendering side builds it; the bus carries
// it; the panel renders it.
//
// `style_id` is the name of the domain ShapeConfiguration the node's appearance
// derives from (empty when none). It is the link the detail pane uses to look
// up the node's colours/line width in the ShapeConfigSet.

// Die Werte eines Knotens ohne seine Kinder. Eigener Typ, weil beides
// verschieden benutzt wird: die Werte wandern einzeln weiter (der Baum hängt
// sie an sein Item), die Hierarchie wird nur durchlaufen. So kopiert niemand
// versehentlich einen ganzen Teilbaum mit.
struct SceneNodeValues {
  std::string name;
  std::string style_id;
  bool has_shape{false};
  SnapshotShapeKind shape_kind{SnapshotShapeKind::kNone};
  int draw_layer{0};
  // Die Box der eigenen Geometrie, im Knotenraum und in Seitenkoordinaten;
  // leer, wenn der Knoten keine Geometrie trägt.
  std::optional<SnapshotBounds> local_bounds;
  std::optional<SnapshotBounds> world_bounds;
  std::optional<SnapshotTextDetail> text_detail;
};

struct SceneNodeSnapshot {
  SceneNodeValues values;
  std::vector<SceneNodeSnapshot> children;
};

#endif  // SCENE_SNAPSHOT_HPP
