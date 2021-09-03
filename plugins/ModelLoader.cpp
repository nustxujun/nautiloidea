#include "ModelLoader.h"
#include "SimpleMath.h"
#include "Renderer.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <regex>
#include <filesystem>

using V = DirectX::SimpleMath::Vector3;
using V2 = DirectX::SimpleMath::Vector2;

//struct Vertex
//{
//    V pos;
//    V2 uv;
//    V norm;
//    V tan;
//    V binorm;
//    UINT color;
//
//    Vertex()
//    {
//    }
//
//    Vertex(V&& p, V2&& tc, V&& n, V&& t, V&& b, UINT c) :
//        pos(p), uv(tc), norm(n), tan(t), binorm(b), color(c)
//    {
//    }
//    Vertex(float x, float y, float z) :
//        pos(x, y, z)
//    {}
//
//    Vertex(const V& v) :
//        pos(v)
//    {}
//};
//
//
//
//ICOSphereCreater::ICOSphereCreater(float radius, int resolution)
//{
//
//
//    auto name = Common::format("icosphere_", radius, "_", resolution);
//    auto mesh = RenderContext::getSingleton()->getObject<Mesh>(name);
//    if (mesh)
//        return ;
//
//    mesh = RenderContext::getSingleton()->createObject<Mesh>(name);
//    
//
//	std::vector<Vertex> vertices(12);
//
//    // create 12 vertices of icosahedron
//    float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
//
//    vertices[0] = {-1, t, 0};
//    vertices[1] = {1, t, 0};
//    vertices[2] = {-1, -t, 0};
//    vertices[3] = {1, -t, 0};
//
//    vertices[4] = {0, -1, t};
//    vertices[5] = {0, 1, t};
//    vertices[6] = {0, -1, -t};
//    vertices[7] = {0, 1, -t};
//
//    vertices[8] = {t, 0, -1};
//    vertices[9] = {t, 0, 1};
//    vertices[10] = {-t, 0, -1};
//    vertices[11] = {-t, 0, 1};
//
//
//    // normalize
//    for (auto& v : vertices)
//    {
//        v.pos.Normalize();
//    }
//
//    //create 20 triangles of icosahedron
//
//    struct Face
//    {
//        UINT v1;
//        UINT v2;
//        UINT v3;
//    };
//
//    std::vector<Face> faces(20);
//
//    // 5 faces around point 0
//    faces[0] = {0, 11, 5};
//    faces[1] = {0, 5, 1};
//    faces[2] = {0, 1, 7};
//    faces[3] = {0, 7, 10};
//    faces[4] = {0, 10, 11};
//
//    // 5 adjacent faces 
//    faces[5] = {1, 5, 9};
//    faces[6] = {5, 11, 4};
//    faces[7] = {11, 10, 2};
//    faces[8] = {10, 7, 6};
//    faces[9] = {7, 1, 8};
//
//    // 5 faces around point 3
//    faces[10] = {3, 9, 4};
//    faces[11] = {3, 4, 2};
//    faces[12] = {3, 2, 6};
//    faces[13] = {3, 6, 8};
//    faces[14] = {3, 8, 9};
//
//    // 5 adjacent faces 
//    faces[15] = {4, 9, 5};
//    faces[16] = {2, 4, 11};
//    faces[17] = {6, 2, 10};
//    faces[18] = {8, 6, 7};
//    faces[19] = {9, 8, 1};
//
//    std::unordered_map<int64_t, UINT> middlePointIndexCache;
//    auto getMiddlePoint = [&](UINT p1, UINT p2)->UINT
//    {
//        // first check if we have it already
//        bool firstIsSmaller = p1 < p2;
//        int64_t smallerIndex = firstIsSmaller ? p1 : p2;
//        int64_t greaterIndex = firstIsSmaller ? p2 : p1;
//        int64_t key = (smallerIndex << 32) + greaterIndex;
//
//        auto ret = middlePointIndexCache.find(key);
//        if (ret != middlePointIndexCache.end())
//        {
//            return ret->second;
//        }
//
//        // not in cache, calculate it
//        auto point1 = vertices[p1];
//        auto point2 = vertices[p2];
//        auto middle = V
//        {
//            (point1.pos.x + point2.pos.x) / 2.0f,
//            (point1.pos.y + point2.pos.y) / 2.0f,
//            (point1.pos.z + point2.pos.z) / 2.0f
//        };
//
//        // add vertex makes sure point is on unit sphere
//        middle.Normalize();
//        vertices.push_back(middle);
//        UINT i = (UINT)vertices.size() - 1;
//
//        // store it, return index
//        middlePointIndexCache[key] = i;
//        return i;
//    };
//
//
//    // refine triangles
//    for (int i = 0; i < resolution; i++)
//    {
//        std::vector<Face> temp;
//        for (auto& f: faces)
//        {
//            // replace triangle by 4 triangles
//            auto a = getMiddlePoint(f.v1, f.v2);
//            auto b = getMiddlePoint(f.v2, f.v3);
//            auto c = getMiddlePoint(f.v3, f.v1);
//
//            temp.push_back(Face{f.v1, a, c});
//            temp.push_back(Face{f.v2, b, a});
//            temp.push_back(Face{f.v3, c, b});
//            temp.push_back(Face{a, b, c});
//        }
//        faces.swap(temp);
//    }
//
//
//    const float pi = 3.14159265368f;
//    for (auto& v : vertices)
//    {
//        v.norm = v.pos;
//        v.norm.Normalize();
//        v.pos = v.norm * radius;
//
//        v.uv.x = (std::atan2(v.norm.x, v.norm.z) + pi) / pi * 0.5f;
//        v.uv.y = (std::acos(v.norm.y) + pi) / pi - 1.0f;
//
//        V axis = V::UnitY;
//        if (v.norm == V::UnitY || v.norm == -V::UnitY)
//            axis = V::UnitX;
//        v.tan = axis.Cross(v.norm);
//        v.binorm = v.tan.Cross(v.norm);
//        v.color = (UINT(v.uv.x * 255) << 8) + (UINT(v.uv.y * 255) << 16);
//    }
//
//    auto renderer = Renderer::getSingleton();
//    UINT stride = sizeof(Vertex);
//    mesh->vertices = renderer->createBuffer((UINT)vertices.size() * stride, stride, false, D3D12_HEAP_TYPE_UPLOAD, vertices.data());
//    mesh->indices = renderer->createBuffer((UINT)faces.size() * 3 * 4, 4, false, D3D12_HEAP_TYPE_UPLOAD, faces.data());
//    mesh->numIndices = faces.size() * 3;
//
//    mesh->submeshes = {{0,0, (UINT)mesh->numIndices}};
//
//	mObject = mesh;
//}
//
//UVSphereCreator::UVSphereCreator(float radius, int resolution)
//{
//    auto name = Common::format("icosphere_", radius, "_", resolution);
//    auto mesh = RenderContext::getSingleton()->getObject<Mesh>(name);
//    if (mesh)
//        return;
//
//    mesh = RenderContext::getSingleton()->createObject<Mesh>(name);
//
//    std::vector<Vertex> vertices;
//
//
//    const float pi = 3.14159265358f;
//    size_t vcount = 0;
//    for (int j = 0; j <= resolution; ++j)
//    {
//        float degy = pi * j / resolution;
//        float siny = sin(degy);
//        float cosy = cos(degy);
//        float y = cosy * radius;
//        float r = siny * radius;
//        //float r = radius;
//        for (int i = 0; i <= resolution; ++i)
//        {
//            float degx = i * 2 * pi / resolution;
//            float sinx = sin(degx);
//            float cosx = cos(degx);
//
//            float z = cosx * r;
//            float x = sinx * r;
//
//            Vertex vert;
//
//            // pos
//            vert.pos = {x,y,z};
//
//            // normal
//            vert.norm = { x,y,z };
//            vert.norm.Normalize();
//
//            //uv
//            vert.uv = {(float)i / (float)resolution, (float)j / (float)resolution};
//
//            // tangent
//            V axis = V::UnitY;
//            if (vert.norm == V::UnitY || vert.norm == -V::UnitY)
//                axis = V::UnitX;
//            vert.tan = axis.Cross(vert.norm);
//            vert.tan.Normalize();
//
//            //bitangent
//            vert.binorm = vert.tan.Cross(vert.norm);
//            vert.binorm.Normalize();
//
//            vertices.emplace_back(vert);
//        }
//    }
//
//    std::vector<unsigned int> indices;
//
//    for (int j = 0; j < resolution; ++j)
//    {
//        for (int i = 0; i < resolution; ++i)
//        {
//            unsigned int p1 = j * (resolution + 1) + i;
//            unsigned int p2 = p1 + (resolution + 1);
//            unsigned int p3 = p1 + 1;
//            unsigned int p4 = p2 + 1;
//            indices.push_back(p1);
//            indices.push_back(p2);
//            indices.push_back(p3);
//
//
//            indices.push_back(p3);
//            indices.push_back(p2);
//            indices.push_back(p4);
//
//        }
//    }
//
//
//    auto renderer = Renderer::getSingleton();
//    UINT stride = sizeof(Vertex);
//    mesh->vertices = renderer->createBuffer((UINT)vertices.size() * stride, stride, false, D3D12_HEAP_TYPE_UPLOAD, vertices.data());
//    mesh->indices = renderer->createBuffer((UINT)indices.size() * 4, 4, false, D3D12_HEAP_TYPE_UPLOAD, indices.data());
//    mesh->numIndices = indices.size() * 3;
//
//    mesh->submeshes = { {0,0, (UINT)mesh->numIndices} };
//
//    mObject = mesh;
//
//}
//
//Model::Ptr ModelLoader::operator()(const std::string& filename)
//{
//    Assimp::Importer importer;
//
//    const aiScene* scene = importer.ReadFile(filename,
//        aiProcess_Triangulate |
//        aiProcess_JoinIdenticalVertices |
//        aiProcess_GenNormals |
//        //aiProcess_GenSmoothNormals|
//        aiProcess_CalcTangentSpace |
//        aiProcess_ConvertToLeftHanded);
//
//    if (!scene)
//    {
//        WARN("cannot load model");
//        return {};
//    }
//
//    auto rc = RenderContext::getSingleton();
//    auto renderer = Renderer::getSingleton();
//
//    auto model = rc->createObject<Model>(filename);
//
//    std::regex reg("^(.+)[/\\\\].+\\..+$");
//    std::smatch match;
//    std::string totalpath;
//    if (std::regex_match(filename, match, reg))
//    {
//        totalpath = std::string(match[1]) + "/";
//    }
//
//    if (scene->HasMaterials())
//    {
//        for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
//        {
//            auto m = scene->mMaterials[i];
//            Material::Ptr mat = rc->createObject<Material>(Common::format(filename, "_material_", i));
//
//            auto getTex = [&](auto type, bool srgb)->Texture::Ptr {
//                if (m->GetTextureCount(type))
//                {
//                    aiString path;
//                    aiTextureMapping mapping;
//                    UINT index;
//                    m->GetTexture(type, 0, &path, &mapping, &index);
//                    std::cout << path.C_Str() << std::endl;
//                    if (path.length == 0)
//                        return {};
//
//                    std::string realpath = totalpath + path.C_Str();
//                    if (!std::filesystem::exists(realpath))
//                        return {};
//
//                    auto tex = rc->createObject<Texture>(realpath);
//                    tex->init(realpath, srgb);
//
//                    return tex;
//                }
//                return {};
//            };
//
//            for (int i = 0; i <= 18; ++i)
//            {
//
//                std::cout << m->GetName().C_Str()<<i << " " << m->GetTextureCount((aiTextureType)i) << std::endl;
//            }
//
//            
//
//            mat->textures["albedo"] = getTex(aiTextureType_DIFFUSE, true);
//            mat->textures["normal"] = getTex(aiTextureType_NORMALS, false);
//            mat->textures["ambient"] = getTex(aiTextureType_AMBIENT, true);
//            mat->textures["height"] = getTex(aiTextureType_HEIGHT, false);
//
//            mat->textures["roughness"] = getTex(aiTextureType_DIFFUSE_ROUGHNESS, false);
//            mat->textures["metallic"] = getTex(aiTextureType_METALNESS, false);
//
//            //mat.shininess = getTex(aiTextureType_SHININESS);
//            aiColor4D d;
//            aiGetMaterialColor(m, AI_MATKEY_COLOR_DIFFUSE, &d);
//            //mat.diffuse = { d.r, d.g, d.b };
//            mat->init("shaders/scene_vs.hlsl", "shaders/scene_ps.hlsl","");
//            
//            model->materials.push_back(mat);
//        }
//    }
//
//    auto mesh = rc->createObject<Mesh>(Common::format(filename, "_mesh"));
//    model->mesh = mesh;
//    std::vector<Vertex> vertices;
//    std::vector<UINT> indices;
//    V pmin = { FLT_MAX,FLT_MAX ,FLT_MAX };
//    V pmax = { FLT_MIN, FLT_MIN ,FLT_MIN };
//    auto process = [&](auto& process, const aiNode* node, const aiMatrix4x4* pm)-> void
//    {
//        aiMatrix4x4 trans = node->mTransformation;
//        if (pm)
//        {
//            trans = *pm * trans;
//        }
//        for (unsigned int i = 0; i < node->mNumChildren; ++i)
//        {
//            process(process, node->mChildren[i], &trans);
//        }
//
//
//        auto normtrans = trans;
//        normtrans.Inverse().Transpose();
//
//        auto meshs = node->mMeshes;
//        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
//        {
//            auto mesh_index = meshs[i];
//            auto m = scene->mMeshes[mesh_index];
//
//            Mesh::SubMesh submesh;
//            submesh.materialIndex = m->mMaterialIndex;
//            submesh.numIndices = m->mNumFaces * 3;
//            submesh.startIndex = indices.size();
//
//            mesh->submeshes.push_back(submesh);
//
//
//
//            if (m->HasFaces())
//            {
//                auto start = vertices.size();
//                for (int j = 0; j < m->mNumFaces; ++j)
//                {
//                    for (int k = 0; k < 3; ++k)
//                        indices.push_back(m->mFaces[j].mIndices[k] + start);
//                }
//            }
//
//            //memcpy(&model->transform, &trans, sizeof(model->transform));
//
//            for (int j = 0; j < m->mNumVertices; ++j)
//            {
//                vertices.emplace_back();
//                Vertex& vert = vertices.back();
//                aiVector3D pos = *(m->mVertices + j);
//                pos = trans * pos;
//                vert.pos = {pos.x, pos.y, pos.z};
//
//                pmin = V::Min(pmin, vert.pos);
//                pmax = V::Max(pmax, vert.pos);
//
//                if (m->HasNormals())
//                {
//                    auto norm = * (m->mNormals + j);
//                    norm = normtrans * norm;
//                    vert.norm = {norm.x, norm.y, norm.z};
//                }
//
//                if (m->HasTextureCoords(0))
//                {
//                    auto uv = *(m->mTextureCoords[0] + j);
//
//                    vert.uv = {uv.x, uv.y};
//                }
//
//                if (m->HasTangentsAndBitangents())
//                {
//                    auto tan =  *(m->mTangents + j);
//                    auto binorm = *(m->mBitangents + j);
//                    tan = normtrans * tan;
//                    binorm = normtrans * binorm;
//
//                    vert.tan = {tan.x, tan.y, tan.z};
//                    vert.binorm = {binorm.x, binorm.y, binorm.z};
//                }
//
//            }
//
//
//        }
//
//    };
//
//    process(process, scene->mRootNode, nullptr);
//
//    mesh->init(vertices.data(), vertices.size() * sizeof(Vertex), indices.data(), indices.size() * sizeof(UINT), sizeof(Vertex), sizeof(UINT), indices.size());
//    memcpy(model->transform.data(), &DirectX::SimpleMath::Matrix::Identity, (4 * 16));
//    memcpy(model->normTransform.data(), model->transform.data(), (4 * 16));
//
//    V center = (pmin + pmax) * 0.5f;
//    V half = (pmax - pmin) * 0.5f;
//    model->aabb.center = {center.x, center.y, center.z};
//    model->aabb.extent = {half.x, half.y, half.z};
//    model->boundingradius = half.Length() * 2;
//    model->init();
//
//
//    return model;
//}
//
//Mesh::Ptr QuadMesh::operator()(float w) const
//{
//    auto mesh = RenderContext::getSingleton()->createObject<Mesh>("Quad");
//    auto renderer = Renderer::getSingleton();
//    UINT stride = sizeof(Vertex);
//
//    auto half = w * 0.5f;
//    std::vector<Vertex> vertices = {
//        {{-half, 0, -half}, {0,0}, {0,1,0}, {1,0,0}, {0,0,1}, 0xffffffff },
//        {{ half, 0, -half}, {1,0}, {0,1,0}, {1,0,0}, {0,0,1}, 0xffffffff },
//        {{ half, 0,  half}, {1,1}, {0,1,0}, {1,0,0}, {0,0,1}, 0xffffffff },
//        {{-half, 0,  half}, {0,1}, {0,1,0}, {1,0,0}, {0,0,1}, 0xffffffff },
//    };
//    std::vector<UINT> indices = {
//        0,1,2,
//        0,2,3,
//    };
//
//    
//
//    mesh->vertices = renderer->createBuffer((UINT)vertices.size() * stride, stride, false, D3D12_HEAP_TYPE_UPLOAD, vertices.data());
//    mesh->indices = renderer->createBuffer((UINT)indices.size() * 4, 4, false, D3D12_HEAP_TYPE_UPLOAD, indices.data());
//    mesh->numIndices = indices.size() * 3;
//
//    mesh->submeshes = { {0,0, (UINT)mesh->numIndices} };
//
//    return mesh;
//}
