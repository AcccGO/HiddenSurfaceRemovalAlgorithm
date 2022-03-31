#include "Model.h"

#include <time.h>

#include <fstream>
#include <gtc/matrix_transform.hpp>
#include <iostream>

#include "AnalyzeObjFile.h"
#include "Parameter.h"

namespace model
{
    using namespace std;
    using namespace analyze_obj_file;

    // user model choice
    Model::Model(int choice, float scale)
    {
        all_obj_name.push_back("bunny.obj");
        all_obj_name.push_back("teapot.obj");
        all_obj_name.push_back("ball.obj");
        all_obj_name.push_back("deer.obj");
        all_obj_name.push_back("special.obj");

        choice           = choice < all_obj_name.size() ? choice : 0;
        choice           = choice >= 0 ? choice : 0;
        current_obj_name = all_obj_name[choice];
        current_obj_path = "models/";

        this->scale = scale;

        LoadModel();
    }

    Model::Model(int choice)
    {
        all_obj_name.push_back("bunny.obj");
        all_obj_name.push_back("teapot.obj");
        all_obj_name.push_back("ball.obj");
        all_obj_name.push_back("deer.obj");
        all_obj_name.push_back("special.obj");

        choice           = choice < all_obj_name.size() ? choice : 0;
        choice           = choice >= 0 ? choice : 0;
        current_obj_name = all_obj_name[choice];
        current_obj_path = "models/";

        scale = 1.0;

        LoadModel();
    }

    Model::Model()
    {
        all_obj_name.push_back("bunny.obj");
        all_obj_name.push_back("teapot.obj");
        all_obj_name.push_back("ball.obj");
        all_obj_name.push_back("deer.obj");
        all_obj_name.push_back("special.obj");

        current_obj_name = all_obj_name[0];
        current_obj_path = "models/";

        scale = 1.0;

        LoadModel();
    }

    void Model::LoadModel()
    {
        ifstream fin(current_obj_path + current_obj_name);
        string fileString((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
        vector<string> lines = Split(fileString, "\n");

        MeshGroup* current_mesh_group = NULL;

        string line;
        AnalyzeObjFile obj_line;
        const unsigned long int line_size = lines.size();
        unsigned long int index           = 0;
        while (index < line_size) {
            line = lines[index++];
            obj_line.init(line);
            string command = obj_line.GetWord();
            if (command.size() == 0)
                continue;

            // Comments.
            if (command == "#") {
                continue;
            }
            // Mesh group.
            else if (command == "o" || command == "g") {
                current_mesh_group = CreatNewMeshGroup(obj_line.GetWord());
                continue;
            }
            // Vertex.
            else if (command == "v") {
                float x = obj_line.GetFloat() * scale;
                float y = obj_line.GetFloat() * scale;
                float z = obj_line.GetFloat() * scale;
                Vertex new_vertex(x, y, z);
                obj_vectexs.push_back(new_vertex);
                continue;
            }

            // Normal.
            else if (command == "vn") {
                float x = obj_line.GetFloat();
                float y = obj_line.GetFloat();
                float z = obj_line.GetFloat();
                Normal new_normal(x, y, z);
                obj_normals.push_back(new_normal);
                continue;
            }
            // All facets under the current group.
            else if (command == "f") {
                // If there is no group name.
                if (current_mesh_group == NULL) {
                    current_mesh_group = CreatNewMeshGroup();
                }

                vector<Face> current_faces = GetMeshGroupFaces(obj_line);
                for (int i = 0; i < current_faces.size(); i++) {
                    current_mesh_group->mesh_group_faces.push_back(current_faces[i]);
                }
            }
        }

        SplitFaces();
    }

    MeshGroup* Model::CreatNewMeshGroup(string name)
    {
        MeshGroup new_mesh_group(name);
        this->all_mesh_groups.push_back(new_mesh_group);
        return &all_mesh_groups.back();
    }

    vector<Face> Model::GetMeshGroupFaces(AnalyzeObjFile obj_line)
    {
        float vertex_index[4];
        float normal_index[4];

        unsigned long int index = 0;
        while (true) {
            string word = obj_line.GetWord();
            if (word == "")
                break;
            vector<string> subWords = Split(word, "/");
            if (subWords.size() >= 1) {
                int vi              = atoi(subWords[0].c_str()) - 1;  // The subscripts start at 1 when storing.
                vertex_index[index] = vi;
            }
            if (subWords.size() == 3 || subWords.size() == 2)  // Vertex/texture coordinates/normals.
            {
                int ni              = atoi(subWords[subWords.size() - 1].c_str()) - 1;
                normal_index[index] = ni;
            } else if (subWords.size() < 1) {
                normal_index[index] = -1;
            }
            index += 1;
        }

        vector<Face> faces;

        // Triangular sheet.
        if (index == 3) {
            Face face;

            for (int i = 0; i < 3; i++) {
                face.normal_index[i] = normal_index[i];
                face.vertex_index[i] = vertex_index[i];
            }

            faces.push_back(face);
        }
        // Triangulate the quadrilateral.
        else if (index == 4) {
            Face face;

            for (int i = 0; i < 3; i++) {
                face.normal_index[i] = normal_index[i];
                face.vertex_index[i] = vertex_index[i];
            }

            faces.push_back(face);

            face.normal_index[0] = normal_index[2];
            face.vertex_index[0] = vertex_index[2];
            face.normal_index[1] = normal_index[3];
            face.vertex_index[1] = vertex_index[3];
            face.normal_index[2] = normal_index[0];
            face.vertex_index[2] = vertex_index[0];

            faces.push_back(face);
        }
        return faces;
    }

    void Model::TransformModel()
    {
        glm::mat4 R(1.0);
        R = glm::rotate(R, rotate_angle_x, glm::vec3(1, 0, 0));
        R = glm::rotate(R, rotate_angle_y, glm::vec3(0, 1, 0));

        glm::mat4 M(1.0);
        M = glm::translate(M, obj_pos);
        M = glm::rotate(M, rotate_angle_x, glm::vec3(1, 0, 0));
        M = glm::rotate(M, rotate_angle_y, glm::vec3(0, 1, 0));

        // Compute the vector after model transformation.
        // Compute the normal rotation.
        trans_normal.clear();
        if (!obj_normals.empty()) {
            for (int i = 0; i < obj_normals.size(); i++) {
                Normal normal = obj_normals[i];

                glm::vec4 n_ori(normal.x, normal.y, normal.z, 1);
                glm::vec4 n_rot = R * n_ori;

                glm::vec3 norm_dir = glm::normalize(glm::vec3(n_rot));

                Normal new_normal{norm_dir.x, norm_dir.y, norm_dir.z};
                trans_normal.push_back(new_normal);
            }
        }

        trans_vertexs.clear();
        for (unsigned long int i = 0; i < obj_vectexs.size(); i++) {
            Vertex v_ori = obj_vectexs[i];
            glm::vec4 v_glm(v_ori.x, v_ori.y, v_ori.z, 1);
            glm::vec4 v_trans = M * v_glm;

            Vertex new_vertex{v_trans.x, v_trans.y, v_trans.z};
            trans_vertexs.push_back(new_vertex);
        }

        // Calculate the light color of the face.
        shading_color.clear();
        for (int i = 0; i < obj_faces.size(); i++) {
            Color new_color;
            for (int j = 0; j < 3; j++) {
                int vid = obj_faces[i].vertex_index[j];
                int nid = obj_faces[i].normal_index[j];

                glm::vec3 vertex = glm::vec3(trans_vertexs[vid].x, trans_vertexs[vid].y, trans_vertexs[vid].z);

                // if (!trans_normal.empty())
                //{
                //	glm::vec3 normal = glm::vec3(trans_normal[nid].x, trans_normal[nid].y, trans_normal[nid].z);

                //	glm::vec3 light_dir = glm::vec3(0.0, 0.0, 1.0);  // glm::normalize(OpenGLUtils::light_pos - vertex);
                //	double cos = glm::dot(normal, light_dir);

                //	if (cos < 0)
                //		cos = 0;
                //	new_color.face_color[j] = glm::vec3{ std::fabs(cos),std::fabs(cos),std::fabs(cos) };
                //}
                // else
                //{
                //	//new_color.face_color[j] = 1.0f/255 * glm::vec3{ std::rand()%256,std::rand() % 256,std::rand() % 256 };
                //	//new_color.face_color[j] = glm::vec3(1.0f*i / obj_faces.size(), 1.0f*(i%5) / obj_faces.size(), 1.0f*(i%3) / obj_faces.size());
                //	new_color.face_color[j] = glm::vec3(1);
                //}
                new_color.face_color[j] = model_color;
            }
            shading_color.push_back(new_color);
        }

        glm::mat4 V   = glm::lookAt(camera_pos, camera_lookat, glm::vec3(0, 1, 0));
        glm::mat4 P   = glm::ortho(-1.0, 1.0, -1.0, 1.0, 0.001, 5.0);
        glm::mat4 MVP = P * V * M;

        // MVP.
        for (int i = 0; i < obj_vectexs.size(); i++) {
            Vertex v_ori = obj_vectexs[i];
            glm::vec4 v_glm(v_ori.x, v_ori.y, v_ori.z, 1);
            glm::vec4 v_trans = MVP * v_glm;
            trans_vertexs[i]  = Vertex(v_trans.x / v_trans.w, v_trans.y / v_trans.w, -v_trans.z / v_trans.w);
        }
    }

    void Model::SplitFaces()
    {
        obj_faces.clear();

        for (auto mg : all_mesh_groups) {
            for (auto face : mg.mesh_group_faces) {
                obj_faces.push_back(face);
            }
        }
    }
}