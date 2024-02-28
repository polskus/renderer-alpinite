#ifndef GLTFREADER_H
#define GLTFREADER_H

#include "tile_scheduler/tile_types.h"
#include <QObject>

namespace nucleus::tile_scheduler {

class GLTFReader : public QObject {
    Q_OBJECT
public:
    explicit GLTFReader(QObject* parent = nullptr);

public slots:
    void deliver_tile(const tile_types::TileLayer& tile);
signals:
    void tile_read(const tile_types::LayeredTile& tile);

private:
    tile_types::LayeredTile load_tile_from_gltf(const tile_types::TileLayer& tile);
};

} // namespace nucleus::tile_scheduler
#endif // GLTFREADER_H
