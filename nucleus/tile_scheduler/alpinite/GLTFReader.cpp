#include "GLTFReader.h"
#include "qdebug.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

using namespace nucleus::tile_scheduler;

GLTFReader::GLTFReader(QObject* parent)
    : QObject { parent }
{
}

void GLTFReader::deliver_tile(const tile_types::TileLayer& tile)
{
    tile_types::LayeredTile dummy = { tile.id, tile.network_info, std::make_shared<QByteArray>(), std::make_shared<QByteArray>(),
        std::make_shared<QByteArray>(), std::make_shared<QByteArray>() };

    if (tile.network_info.status == tile_types::NetworkInfo::Status::Good) {
        qDebug() << "RECV TILE: " << tile.id.zoom_level << "/" << tile.id.coords.x << "/" << tile.id.coords.y << " " << (uint64_t)tile.network_info.status
                 << " Size: " << tile.data->size();

        tile_types::LayeredTile loaded_tile = load_tile_from_gltf(tile);

        emit tile_read(loaded_tile);
    } else {
        qDebug() << "Emitting dummy tile";
        emit tile_read(dummy);
    }
}

tile_types::LayeredTile GLTFReader::load_tile_from_gltf(const tile_types::TileLayer& tile)
{
    tile_types::LayeredTile dummy = { tile.id, tile.network_info, std::make_shared<QByteArray>(), std::make_shared<QByteArray>(),
        std::make_shared<QByteArray>(), std::make_shared<QByteArray>() };

    void* buf = (void*)tile.data->constData(); /* Pointer to glb or gltf file data */
    size_t size = tile.data->size(); /* Size of the file data */

    cgltf_options options = {};
    cgltf_data* data_ptr = NULL;
    cgltf_result result = cgltf_parse(&options, buf, size, &data_ptr);
    if (result == cgltf_result_success) {
        qDebug() << "CGLTF READ SUCCESS";
    } else if (result == cgltf_result_invalid_gltf) {
        qDebug() << "CGLTF INVALID GLTF";
        return dummy;
    } else {
        qDebug() << result;
        return dummy;
    }

    cgltf_data& data = *data_ptr;

    // tile_types::LayeredTile gltf_tile = { tile.id, tile.network_info, std::make_shared<QByteArray>(), std::make_shared<QByteArray>() };

    // return gltf_tile;

    qDebug() << "MESHES: " << data.meshes_count;
    assert(data.meshes_count == 1);
    cgltf_mesh& mesh = data.meshes[0];

    qDebug() << "PRIMITIVES COUNT: " << mesh.primitives_count;
    assert(mesh.primitives_count == 1);
    cgltf_primitive& mesh_primitive = mesh.primitives[0];
    assert(mesh_primitive.type == cgltf_primitive_type::cgltf_primitive_type_triangles);

    cgltf_attribute* position_attr = nullptr;
    for (unsigned int i = 0; i < mesh_primitive.attributes_count; i++) {
        cgltf_attribute* attribute = &mesh_primitive.attributes[i];
        if (attribute->type == cgltf_attribute_type_position) {
            position_attr = attribute;
        }
    }

    cgltf_attribute* uv_attr = nullptr;
    for (unsigned int i = 0; i < mesh_primitive.attributes_count; i++) {
        cgltf_attribute* attribute = &mesh_primitive.attributes[i];
        if (attribute->type == cgltf_attribute_type_texcoord) {
            uv_attr = attribute;
        }
    }

    assert(position_attr != nullptr);
    assert(uv_attr != nullptr);

    assert(data.buffers_count > 0);
    cgltf_buffer& first_buffer = data.buffers[0];
    if (first_buffer.data == nullptr) {
        first_buffer.data = const_cast<void*>(data.bin);
        first_buffer.size = data.bin_size;
    }

    cgltf_accessor& index_accessor = *mesh_primitive.indices;
    std::vector<glm::uvec3> indices;
    indices.resize(index_accessor.count / 3);
    cgltf_accessor_unpack_indices(
        &index_accessor, reinterpret_cast<unsigned int*>(indices.data()), cgltf_component_size(index_accessor.component_type), indices.size() * 3);

    cgltf_accessor& position_accessor = *position_attr->data;
    cgltf_accessor& uv_accessor = *uv_attr->data;
    assert(position_accessor.buffer_view == uv_accessor.buffer_view);
    std::vector<glm::vec3> positions;
    positions.resize(position_accessor.count);
    cgltf_accessor_unpack_floats(&position_accessor, reinterpret_cast<float*>(positions.data()), positions.size() * 3);
    std::vector<glm::vec2> uvs;
    uvs.resize(uv_accessor.count);
    cgltf_accessor_unpack_floats(&uv_accessor, reinterpret_cast<float*>(uvs.data()), uvs.size() * 2);

    assert(data.scenes_count == 1);
    assert(data.scenes[0].nodes_count == 1);
    cgltf_node& root_node = *data.scenes[0].nodes[0];
    assert(root_node.has_translation);
    assert(root_node.children_count == 1);
    cgltf_node& intermediate_node = *root_node.children[0];
    assert(intermediate_node.has_translation);
    assert(intermediate_node.children_count == 1);
    cgltf_node& mesh_node = *intermediate_node.children[0];
    assert(mesh_node.has_translation);
    assert(mesh_node.children_count == 0);
    glm::dvec3 offset = glm::dvec3(root_node.translation[0], root_node.translation[1], root_node.translation[2])
        + glm::dvec3(intermediate_node.translation[0], intermediate_node.translation[1], intermediate_node.translation[2])
        + glm::dvec3(mesh_node.translation[0], mesh_node.translation[1], mesh_node.translation[2]);

    cgltf_material& material = *mesh_primitive.material;
    assert(material.has_pbr_metallic_roughness);
    assert(material.pbr_metallic_roughness.base_color_texture.texture != nullptr);
    cgltf_texture& albedo_texture = *material.pbr_metallic_roughness.base_color_texture.texture;
    cgltf_image& albedo_image = *albedo_texture.image;

    std::shared_ptr<QByteArray> qb_raw_texture
        = std::make_shared<QByteArray>((char*)cgltf_buffer_view_data(albedo_image.buffer_view), albedo_image.buffer_view->size);

    // OLD CONVERT FROM FLOAT + OFFSET TO DOUBLE
    // std::vector<glm::dvec3> positionsd;
    // positionsd.resize(positions.size());
    // for (unsigned int i = 0; i < positionsd.size(); i++) {
    //     positionsd[i] = glm::dvec3(positions[i]) + offset;
    // }

    // std::vector<glm::dvec2> uvsd;
    // uvsd.resize(uvs.size());
    // for (unsigned int i = 0; i < uvsd.size(); i++) {
    //     uvsd[i] = glm::dvec2(uvs[i]);
    // }

    // NEW CONVERT FROM FLOAT + OFFSET TO FLOAT, TO KEEP THE FLOAT WORKFLOW IN THE SHADERS
    for (unsigned int i = 0; i < positions.size(); i++) {
        positions[i] = (glm::vec3)(glm::dvec3(positions[i]) + offset);
    }

    unsigned int max_index = positions.size() - 1;
    for (unsigned int i = 0; i < indices.size(); i++) {
        if (indices[i].x > max_index) {
            qDebug() << "Index[" << i << "].x > max_index !!! " << indices[i].x << " > " << max_index;
            assert(false);
        }
        if (indices[i].y > max_index) {
            qDebug() << "Index[" << i << "].y > max_index !!! " << indices[i].y << " > " << max_index;
            assert(false);
        }
        if (indices[i].z > max_index) {
            qDebug() << "Index[" << i << "].z > max_index !!! " << indices[i].z << " > " << max_index;
            assert(false);
        }
    }
    qDebug() << "All " << indices.size() << " indices are correct!";

    // OLD USE OF DOUBLE DATA
    // std::shared_ptr<QByteArray> qb_indices = std::make_shared<QByteArray>(reinterpret_cast<const char*>(indices.data()), indices.size());
    // std::shared_ptr<QByteArray> qb_positions = std::make_shared<QByteArray>(reinterpret_cast<const char*>(positionsd.data()), positionsd.size());
    // std::shared_ptr<QByteArray> qb_uvs = std::make_shared<QByteArray>(reinterpret_cast<const char*>(uvsd.data()), uvsd.size());

    // NEW FLOAT ONLY DATA
    std::shared_ptr<QByteArray> qb_indices = std::make_shared<QByteArray>((char*)indices.data(), indices.size() * sizeof(glm::uvec3));
    std::shared_ptr<QByteArray> qb_positions = std::make_shared<QByteArray>((char*)positions.data(), positions.size() * sizeof(glm::vec3));
    std::shared_ptr<QByteArray> qb_uvs = std::make_shared<QByteArray>((char*)uvs.data(), uvs.size() * sizeof(glm::vec2));

    qDebug() << "Indices Buffer Size: " << qb_indices->size();
    qDebug() << "Positions Buffer Size: " << qb_positions->size();
    qDebug() << "UVs Buffer Size: " << qb_uvs->size();
    qDebug() << "Texture Buffer Size: " << qb_raw_texture->size();

    cgltf_free(data_ptr);

    return { tile.id, tile.network_info, qb_indices, qb_positions, qb_uvs, qb_raw_texture };
}
