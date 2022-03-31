#pragma once
#include <common.hpp>
#include <geometric.hpp>
#include <glm.hpp>
#include <vec3.hpp>
#include <vector>

#include "AnalyzeObjFile.h"

namespace model
{
    using namespace std;
    using namespace glm;
    using namespace analyze_obj_file;

    class Vector3
    {
    public:
        float x, y, z;

    public:
        Vector3()
        {
            x = 0;
            y = 0;
            z = 0;
        }
        Vector3(float x)
        {
            this->x = x;
            this->y = x;
            this->z = x;
        }
        Vector3(float x, float y, float z)
        {
            this->x = x;
            this->y = y;
            this->z = z;
        }

        explicit Vector3(glm::vec3 x)
        {
            this->x = x.x;
            this->y = x.y;
            this->z = x.z;
        }

        auto operator+() const { return Vector3{x, y, z}; }
        auto operator-() const { return Vector3{-x, -y, -z}; }

        auto& operator+=(Vector3 const& a)
        {
            x += a.x;
            y += a.y;
            z += a.z;
            return *this;
        }
        auto& operator-=(Vector3 const& a)
        {
            x -= a.x;
            y -= a.y;
            z -= a.z;
            return *this;
        }
        auto& operator*=(Vector3 const& a)
        {
            x *= a.x;
            y *= a.y;
            z *= a.z;
            return *this;
        }
        auto& operator/=(Vector3 const& a)
        {
            x /= a.x;
            y /= a.y;
            z /= a.z;
            return *this;
        }

        auto& operator[](int i)
        {
            if (i == 0) return x;
            if (i == 1) return y;
            if (i == 2) return z;
        }
    };

    class Vertex : public Vector3
    {
    public:
        Vertex()
            : Vector3(){};
        Vertex(float x, float y, float z)
            : Vector3(x, y, z){};
    };

    class Normal : public Vector3
    {
    public:
        Normal()
            : Vector3(){};
        Normal(float x, float y, float z)
            : Vector3(x, y, z){};
    };

    class Face
    {
    public:
        Vector3 vertex_index;
        Vector3 normal_index;
        Face() {}
    };

    class Color
    {
    public:
        glm::vec3 face_color[3];
    };

    class MeshGroup
    {
    public:
        MeshGroup(){};
        ~MeshGroup(){};
        MeshGroup(string name) { mesh_group_name = name; };
        vector<Face> mesh_group_faces;
        string mesh_group_name;
    };

    class Model
    {
    public:
        vector<string> all_obj_name;
        string current_obj_name;
        string current_obj_path;

        vector<MeshGroup> all_mesh_groups;
        vector<Normal> obj_normals;
        vector<Vertex> obj_vectexs;
        vector<Face> obj_faces;

        float scale;

        // MVP
        vector<Normal> trans_normal;
        vector<Vertex> trans_vertexs;
        vector<Color> shading_color;

    public:
        Model();
        Model(int choice);
        Model(int choice, float scale);
        ~Model(){};

        void LoadModel();
        MeshGroup* CreatNewMeshGroup(string name = "default group");
        vector<Face> GetMeshGroupFaces(AnalyzeObjFile obj_line);

        void SplitFaces();
        void TransformModel();
    };
}
