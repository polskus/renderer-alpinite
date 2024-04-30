#ifndef READER_H
#define READER_H
#include <QDebug>
#include <QFile>
#include <extern/tl_expected/include/tl/expected.hpp>
#include <glm/glm.hpp>
#include <mapbox/vector_tile.hpp>
#include <string>
#include <vector>
// Create a class with minimal functionality to get regions from a vector tile

struct EawsRegion
{
    std::string id="";                                      // The id is the name of the region e.g. "AT-05-18"
    std::string id_alt ="";                                 // ???
    std::string start_date="";                              // The day the regions was defined
    std::string end_date="";                                // Only for outdated regions: The day the regions expired
    std::vector<glm::ivec2> verticesInLocalCoordinates; // the vertices of the region's bounding polygon
};

namespace vector_tile::reader {
/* Reads all EAWS regions stored in a provided vector tile
 * The mvt file is expected to contain three layers: "micro-regions", "micro-regions_elevation", "outline". The first one is relevant for this class.
 * The layer "micro-regions" contains several features, each feature representing one micro region.
 * Every feature contains at least one poperty and one geometry, the latter one holding the vertices of the region's boundary polygon.
 * Every Property has an id. There must be at least one poperty with its id holding the name-string of the region (e.g. "FR-64").
 * Further properties can exist with ids (all string) "alt-id", "start_date", "end_date".
 * If a region has the property with id "end_date" this region is outdated and was substituted by other regions.
 * This old regions is kept in the collection to allow processing of older bulletins that were using this region
 * @param inputFilePath : Path to a mvt file
 * @param nameOfLayerWithEawsRegions : name of the layer that contains the EAWS micro regions and the vertices of their boundary polygon
 */
tl::expected<std::vector<EawsRegion>, std::string> eaws_regions(const QByteArray& inputData, const std::string& nameOfLayerWithEawsRegions = "micro-regions");
} // namespace vector_tile::reader
#endif // READER_H
