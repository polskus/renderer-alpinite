/*****************************************************************************
 * Alpine Terrain Renderer
 * Copyright (C) 2024 Lucas Dworschak
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#pragma once

#include <QObject>
#include <QVector2D>

#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <nucleus/vector_tiles/VectorTileFeature.h>
#include <radix/tile.h>

namespace nucleus::maplabel {

struct FilterDefinitions {
    Q_GADGET
public:
    bool m_peak_visible = true;
    bool m_cities_visible = true;
    bool m_cottages_visible = true;

    bool m_peak_ele_range_filtered = false;
    QVector2D m_peak_ele_range = QVector2D(0,4000);

    Q_PROPERTY(bool peak_visible MEMBER m_peak_visible)
    Q_PROPERTY(bool cities_visible MEMBER m_cities_visible)
    Q_PROPERTY(bool cottages_visible MEMBER m_cottages_visible)
    Q_PROPERTY(QVector2D peak_ele_range MEMBER m_peak_ele_range)
};

class MapLabelFilter : public QObject {
    Q_OBJECT
public:
    explicit MapLabelFilter(QObject* parent = nullptr);

    std::unordered_map<tile::Id, std::unordered_map<nucleus::vectortile::FeatureType, std::unordered_set<std::shared_ptr<nucleus::vectortile::FeatureTXT>>>, tile::Id::Hasher> filter();

    void add_tile(const tile::Id id, std::unordered_map<nucleus::vectortile::FeatureType, std::unordered_set<std::shared_ptr<nucleus::vectortile::FeatureTXT>>> all_features);
    void remove_tile(const tile::Id id);

public slots:
    void filter_changed();

private:
    std::unordered_map<tile::Id, std::unordered_map<nucleus::vectortile::FeatureType, std::unordered_set<std::shared_ptr<nucleus::vectortile::FeatureTXT>>>, tile::Id::Hasher> m_all_features;

    std::queue<tile::Id> tiles_to_filter;
    std::unordered_set<tile::Id, tile::Id::Hasher> all_tiles;

    FilterDefinitions m_definitions;

    void apply_filter_peaks(std::unordered_set<std::shared_ptr<nucleus::vectortile::FeatureTXT>>& features, std::unordered_map<nucleus::vectortile::FeatureType, std::unordered_set<std::shared_ptr<nucleus::vectortile::FeatureTXT>>>& filtered_features);
    void apply_filter_cities(std::unordered_set<std::shared_ptr<nucleus::vectortile::FeatureTXT>>& features, std::unordered_map<nucleus::vectortile::FeatureType, std::unordered_set<std::shared_ptr<nucleus::vectortile::FeatureTXT>>>& filtered_features);
    void apply_filter_cottages(std::unordered_set<std::shared_ptr<nucleus::vectortile::FeatureTXT>>& features, std::unordered_map<nucleus::vectortile::FeatureType, std::unordered_set<std::shared_ptr<nucleus::vectortile::FeatureTXT>>>& filtered_features);

    template<typename T>
    using FeatureFilter = bool (*)(const FilterDefinitions& definition, const T feature);

    std::vector<FeatureFilter<std::shared_ptr<nucleus::vectortile::FeatureTXTPeak>>> m_filter_peaks;
    std::vector<FeatureFilter<std::shared_ptr<nucleus::vectortile::FeatureTXTCity>>> m_filter_cities;
    std::vector<FeatureFilter<std::shared_ptr<nucleus::vectortile::FeatureTXTCottage>>> m_filter_cottages;

};
} // namespace gl_engine
