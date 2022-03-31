#include "ScannerLineZbuffer.h"

#include <iostream>

#include "Parameter.h"

namespace scanner_line
{
    namespace scanner_line_zbuffer
    {
        void ScannerLineZbuffer::FillLineColor(int y_id, int start = 0, int end = -1, glm::vec3 color = glm::vec3{-1}, glm::vec3 color_per_col = glm::vec3{0})
        {
            if (color == glm::vec3{-1})
                color = bg_color;
            if (end == -1)
                end = framebuffer_width;

            unsigned char* current_line = frame_buffer + y_id * framebuffer_width * 3;
            for (auto i = start; i < end; i++) {
                current_line[3 * i + 0] = color.x * 255;
                current_line[3 * i + 1] = color.y * 255;
                current_line[3 * i + 2] = color.z * 255;

                color += color_per_col;
            }
        }

        void ScannerLineZbuffer::SetClearColor(glm::vec3 color)
        {
            bg_color = color;
        }

        void ScannerLineZbuffer::ResetModel(Model* new_model)
        {
            model_to_scan = new_model;
        }

        ScannerLineZbuffer::ScannerLineZbuffer(int width, int height)
        {
            this->model_to_scan = nullptr;
            framebuffer_width   = width;
            framebuffer_height  = height;

            classified_polygon_table.resize(framebuffer_height);
            classified_edge_table.resize(framebuffer_height);

            frame_buffer = new unsigned char[1920 * 1024 * 3];  // Make it as big as possible so you don't have to destroy it and reopen it later.
        }
        ScannerLineZbuffer::ScannerLineZbuffer(Model* current_model, int width, int height)
        {
            this->model_to_scan = current_model;
            framebuffer_width   = width;
            framebuffer_height  = height;

            classified_polygon_table.resize(framebuffer_height);
            classified_edge_table.resize(framebuffer_height);

            frame_buffer = new unsigned char[1920 * 1024 * 3];
        }
        ScannerLineZbuffer::~ScannerLineZbuffer()
        {
            this->model_to_scan = nullptr;
        }

        int ScannerLineZbuffer::ScanModel(int width, int height)
        {
            SetWindowSize(width, height);
            model_to_scan->TransformModel();
            InitData();

            // Algorithm.
            DWORD t1, t2;
            t1 = GetTickCount();
            MainAlgorithm();
            t2 = GetTickCount();

            return t2 - t1;
        }

        // Find a pair of edges that belong to the same polygon.
        int ScannerLineZbuffer::FindEdgePair(int y_id, int polygon_id, ClassifiedEdgeTable* edge_pair[2])
        {
            ClassifiedEdgeTable* cet_head      = classified_edge_table[y_id];
            int                  edge_pair_num = 0;

            while (cet_head != nullptr) {
                // Find the polygon to which it belongs.
                if (cet_head->id == polygon_id) {
                    edge_pair[edge_pair_num] = cet_head;
                    edge_pair_num++;
                }
                // We got an edge. We don't have to keep looking.
                if (edge_pair_num == 2) {
                    break;
                }
                cet_head = cet_head->next_e;
            }
            // No two edges found.
            if (edge_pair_num != 2) {
                return edge_pair_num;
            }
            // Correctly found two edges, compare the order in front and rear.
            // If two edges share the same vertex.
            if (abs(edge_pair[0]->x - edge_pair[1]->x) < 0.01) {
                if (edge_pair[0]->dx < edge_pair[1]->dx) {
                    return 2;
                } else if (edge_pair[0]->dx > edge_pair[1]->dx) {
                    return -2;
                } else {
                    return 0;
                }
            }
            // The two edges do not share the same vertex, only compare the slope.
            else if (edge_pair[0]->x < edge_pair[1]->x) {
                return 2;
            } else {
                return -2;
            }
        }

        void ScannerLineZbuffer::MainAlgorithm()
        {
            double* zbuffer = new double[framebuffer_width];

            // Start progressive scanning.
            for (int y_id = framebuffer_height - 1; y_id >= 0; y_id--) {
                // Sets the background color of the current behavior.
                FillLineColor(y_id);

                // Z buffer is set to a large value.
                for (int i = 0; i < framebuffer_width; ++i) {
                    zbuffer[i] = -9999;
                }

                // Remove the scanned polygons.
                ActivePolygonTable* apt_head;
                apt_head = active_polygon_table;
                // Delete the first one first.
                while (apt_head != nullptr && apt_head->dy < 0) {
                    active_polygon_table = apt_head->next_ap;
                    free(apt_head);
                    apt_head = active_polygon_table;
                }
                // Then delete the remaining unsatisfied situations.
                while (apt_head != nullptr && apt_head->next_ap != nullptr) {
                    if (apt_head->next_ap->dy < 0) {
                        ActivePolygonTable* next = apt_head->next_ap;
                        apt_head->next_ap        = next->next_ap;
                        free(next);
                        next = nullptr;
                    } else {
                        apt_head = apt_head->next_ap;
                    }
                }
                apt_head = active_polygon_table;  // Refers to the first element.

                // Remove the side pairs / edges after scanning.
                ActiveEdgeTable* aet_head = active_edge_table;
                ActiveEdgeTable* aet_last = nullptr;
                while (aet_head != nullptr) {
                    apt_head           = active_polygon_table;  // The active polygon header refers to the first element.
                    bool enable_delete = false;
                    // If both sides are scanned,
                    if (aet_head->dyl < 0 && aet_head->dyr < 0) {
                        ClassifiedEdgeTable* ptr_cE[2];
                        // Find out if there are any edges for this activated polygon.
                        int find_num = FindEdgePair(y_id, aet_head->id, ptr_cE);
                        if (find_num == 2) {
                            // Find the active polygon in which the current active edge pair is located.
                            while (apt_head) {
                                if (apt_head->id == aet_head->id) {
                                    break;
                                }
                                apt_head = apt_head->next_ap;
                            }
                            if (apt_head->id != aet_head->id)
                                std::cout << "所找的活化边对没有所属的活化多边形！此处有问题" << endl;
                            else {
                                ActiveEdgeTable* new_aet = new ActiveEdgeTable(*ptr_cE[0], *ptr_cE[1], *apt_head, y_id);
                                // Add a new active edge pair to the active edge table.
                                if (aet_last == nullptr) {
                                    new_aet->next_aet = aet_head->next_aet;
                                    active_edge_table = new_aet;
                                    aet_last          = new_aet;
                                    delete aet_head;
                                    aet_head = new_aet->next_aet;
                                } else {
                                    new_aet->next_aet  = aet_head->next_aet;
                                    aet_last->next_aet = new_aet;
                                    aet_last           = new_aet;
                                    delete aet_head;
                                    aet_head = new_aet->next_aet;
                                }
                                continue;  // Because aet_head has changed, you should skip the later judgment, or you will make a mistake.
                            }
                        } else if (find_num == -2) {
                            while (apt_head) {
                                if (apt_head->id == aet_head->id) {
                                    break;
                                }
                                apt_head = apt_head->next_ap;
                            }
                            if (apt_head->id != aet_head->id)
                                std::cout << "所找的活化边对没有所属的活化多边形！此处有问题，apt_head->id：" << apt_head->id << ", aet_head->id" << aet_head->id << endl;
                            else {
                                ActiveEdgeTable* new_aet = new ActiveEdgeTable(*ptr_cE[1], *ptr_cE[0], *apt_head, y_id);
                                // Add a new active edge pair to the active edge table
                                if (aet_last == nullptr) {
                                    new_aet->next_aet = aet_head->next_aet;
                                    active_edge_table = new_aet;
                                    aet_last          = new_aet;
                                    delete aet_head;
                                    aet_head = new_aet->next_aet;
                                } else {
                                    new_aet->next_aet  = aet_head->next_aet;
                                    aet_last->next_aet = new_aet;
                                    aet_last           = new_aet;
                                    delete aet_head;
                                    aet_head = new_aet->next_aet;
                                }
                                continue;  // Because aet_head has changed, you should skip the later judgment, or you will make a mistake.
                            }
                        }
                        enable_delete = true;
                    }

                    // If the left side is finished scanning, you need to find a new left side.
                    else if (aet_head->dyl < 0) {
                        ClassifiedEdgeTable* ptr_cE[2];
                        int                  find_num = FindEdgePair(y_id, aet_head->id, ptr_cE);
                        // Find a new edge.
                        if (find_num == 1) {
                            while (apt_head) {
                                if (apt_head->id == aet_head->id) {
                                    break;
                                }
                                apt_head = apt_head->next_ap;
                            }
                            if (apt_head->id != aet_head->id)
                                std::cout << "所找的新左扫描边没有所属的活化多边形！此处有问题，apt_head->id：" << apt_head->id << ", aet_head->id" << aet_head->id << endl;
                            else {
                                // Replace the active edge on the left side.
                                aet_head->xl               = ptr_cE[0]->x;
                                aet_head->dxl              = ptr_cE[0]->dx;
                                aet_head->dyl              = ptr_cE[0]->dy;
                                aet_head->zl               = (-apt_head->d - apt_head->a * aet_head->xl - apt_head->b * y_id) / apt_head->c;
                                aet_head->color_per_line_l = ptr_cE[0]->color_per_line;
                                aet_head->color_top_l      = ptr_cE[0]->color_top;
                            }
                        } else {
                            printf("旧的多边形中无法找到新的左扫描边, y_id: %d, pid: %d\n", y_id, aet_head->id);
                            enable_delete = true;
                        }
                    }

                    // If the right side is finished scanning, you need to find a new right side.
                    else if (aet_head->dyr < 0) {
                        ClassifiedEdgeTable* ptr_cE[2];
                        int                  find_num = FindEdgePair(y_id, aet_head->id, ptr_cE);
                        // Find a new edge.
                        if (find_num == 1) {
                            while (apt_head) {
                                if (apt_head->id == aet_head->id) {
                                    break;
                                }
                                apt_head = apt_head->next_ap;
                            }
                            if (apt_head->id != aet_head->id)
                                std::cout << "所找的新右扫描边没有所属的活化多边形！此处有问题，apt_head->id：" << apt_head->id << ", aet_head->id" << aet_head->id << endl;
                            else {
                                aet_head->xr               = ptr_cE[0]->x;
                                aet_head->dxr              = ptr_cE[0]->dx;
                                aet_head->dyr              = ptr_cE[0]->dy;
                                aet_head->color_per_line_r = ptr_cE[0]->color_per_line;
                                aet_head->color_top_r      = ptr_cE[0]->color_top;
                            }
                        } else {
                            printf("旧的多边形中无法找到新的右扫描边, y_id: %d, pid: %d\n", y_id, aet_head->id);
                            enable_delete = true;
                        }
                    }

                    // Under normal circumstances, continue to check the next active edge pair.
                    // Continue directly when a new edge pair is found.
                    // If you delete.
                    if (enable_delete) {
                        if (aet_last == nullptr) {
                            active_edge_table = aet_head->next_aet;
                            delete aet_head;
                            aet_head = active_edge_table;
                        } else {
                            aet_last->next_aet = aet_head->next_aet;
                            delete aet_head;
                            aet_head = aet_last->next_aet;
                        }
                    }
                    // If you don't delete, just go down.
                    else {
                        aet_last = aet_head;
                        aet_head = aet_head->next_aet;
                    }
                }

                // Check for new polygons to add.
                apt_head = active_polygon_table;  // Refers to the first element.
                aet_head = aet_last;              // Point back to the header.
                if (classified_polygon_table[y_id] != nullptr) {
                    ClassifiedPolygonTable* cpt_head = classified_polygon_table[y_id];
                    if (apt_head != nullptr)
                        while (apt_head->next_ap != nullptr)  // Find the end of the chain list.
                        {
                            apt_head = apt_head->next_ap;
                        }
                    // Add polygons to apt in turn.
                    while (cpt_head != nullptr) {
                        ActivePolygonTable* new_polygon = new ActivePolygonTable(cpt_head);
                        if (apt_head == nullptr) {
                            active_polygon_table = new_polygon;
                            apt_head             = active_polygon_table;
                        } else {
                            apt_head->next_ap = new_polygon;
                            apt_head          = apt_head->next_ap;
                        }
                        // Add the active edges corresponding to the activated polygon.
                        ClassifiedEdgeTable* ptr_cE[2];
                        int                  find_num = FindEdgePair(y_id, apt_head->id, ptr_cE);
                        ActiveEdgeTable*     p_aE;
                        if (find_num == 2) {
                            p_aE = new ActiveEdgeTable(*ptr_cE[0], *ptr_cE[1], *apt_head, y_id);
                            if (active_edge_table == nullptr) {
                                active_edge_table = p_aE;
                                aet_head          = active_edge_table;
                            } else {
                                aet_head->next_aet = p_aE;
                                aet_head           = p_aE;
                            }
                        } else if (find_num == -2) {
                            p_aE = new ActiveEdgeTable(*ptr_cE[1], *ptr_cE[0], *apt_head, y_id);
                            if (active_edge_table == nullptr) {
                                active_edge_table = p_aE;
                                aet_head          = active_edge_table;
                            } else {
                                aet_head->next_aet = p_aE;
                                aet_head           = p_aE;
                            }
                        } else if (find_num != 0) {
                            printf("新的活化多边形生成边对错误, y_id: %d, pid: %d\n", y_id, apt_head->id);
                        } else {
                            printf("新的活化多边形生成边对为0, y_id: %d, pid: %d\n", y_id, apt_head->id);
                        }

                        cpt_head = cpt_head->next_p;  // Find next cpt.
                    }
                }

                // Update z buffer content.
                aet_head = active_edge_table;  // Point back to the header.
                while (aet_head != nullptr)    // Scan each side pair.
                {
                    int       x_left      = ceil(aet_head->xl);
                    int       x_right     = floor(aet_head->xr);
                    double    zx          = aet_head->zl + aet_head->dzx * (x_left - aet_head->xl);  //???
                    glm::vec3 color_left  = aet_head->color_top_l;
                    glm::vec3 color_right = aet_head->color_top_r;
                    glm::vec3 dColor;
                    glm::vec3 color;
                    int       dx = x_right - x_left;
                    for (int i = 0; i < 3; i++)
                        dColor[i] = (color_right[i] - color_left[i]) / dx;
                    color = color_left;

                    for (int x = x_left; x <= x_right; x++) {
                        // Compare with the zbuffer.
                        if (x < 0 || x >= framebuffer_width) {
                            std::cout << "边对" << aet_head->id << "中的位置" << x << "不在扫描区间内" << std::endl;
                            continue;
                        }
                        double z_buf = zbuffer[x];
                        if (zx > z_buf) {
                            zbuffer[x] = zx;
                            // Temp white color.
                            for (int j = 0; j < 3; ++j) {
                                if (color[j] < 0)
                                    frame_buffer[y_id * framebuffer_width * 3 + 3 * x + j] = 0;
                                else if (color[j] > 1)
                                    frame_buffer[y_id * framebuffer_width * 3 + 3 * x + j] = 255;
                                else
                                    frame_buffer[y_id * framebuffer_width * 3 + 3 * x + j] = (int)(color[j] * 255);
                            }
                        }
                        zx += aet_head->dzx;
                        color += dColor;
                    }

                    aet_head = aet_head->next_aet;
                }

                // Update the contents of the active polygon table and the active edge table.
                apt_head = active_polygon_table;
                aet_head = active_edge_table;
                while (apt_head != nullptr) {
                    apt_head->dy -= 1;
                    apt_head = apt_head->next_ap;
                }
                while (aet_head != nullptr) {
                    aet_head->xl += aet_head->dxl;
                    aet_head->xr += aet_head->dxr;
                    aet_head->zl += aet_head->dzx * aet_head->dxl + aet_head->dzy;

                    aet_head->dyl -= 1;
                    aet_head->dyr -= 1;

                    aet_head->color_top_l += aet_head->color_per_line_l;
                    aet_head->color_top_r += aet_head->color_per_line_r;

                    aet_head = aet_head->next_aet;
                }
            }

            delete[] zbuffer;
        }

        // Generate initial edge and polygon tables.
        void ScannerLineZbuffer::InitData()
        {
            int face_id    = 0;
            int polygon_id = 0;

            for (face_id = 0; face_id < model_to_scan->obj_faces.size(); face_id++) {
                Face* face = &model_to_scan->obj_faces[face_id];
                float vertex[3][3];
                float color[3][3];
                float minmax[3][2];

                // Find the maximum and minimum value of xyz in a face.
                for (int i = 0; i < 3; ++i) {
                    if (i == 0) {
                        for (int j = 0; j < 3; j++) {
                            float point_float = model_to_scan->trans_vertexs[face->vertex_index[i]][j];
                            vertex[i][j]      = point_float;
                            minmax[j][0]      = point_float;
                            minmax[j][1]      = point_float;
                        }
                    } else {
                        for (int j = 0; j < 3; j++) {
                            float point_float = model_to_scan->trans_vertexs[face->vertex_index[i]][j];
                            vertex[i][j]      = point_float;
                            if (point_float < minmax[j][0]) {
                                minmax[j][0] = point_float;
                            } else if (point_float > minmax[j][1]) {
                                minmax[j][1] = point_float;
                            }
                        }
                    }
                }

                // To judge whether the triangular patch is within the boundary, the projection coordinate is between-1 and 1.
                bool notInBound = false;
                for (int i = 0; i < 3; ++i) {
                    if (minmax[i][0] > 1 || minmax[i][1] < -1) {
                        notInBound = true;
                        break;
                    }
                }
                if (notInBound)
                    continue;

                // TODO: How to deal with points whose x coordinates are beyond the range of-1 murray 1?
                if (vertex[0][0] > 1 || vertex[0][0] < -1) std::cout << "面片id" << face_id << "顶点[0][0]超出范围" << vertex[0][0] << std::endl;
                if (vertex[1][0] > 1 || vertex[1][0] < -1) std::cout << "面片id" << face_id << "顶点[1][0]超出范围" << vertex[1][0] << std::endl;
                if (vertex[2][0] > 1 || vertex[2][0] < -1) std::cout << "面片id" << face_id << "顶点[2][0]超出范围" << vertex[2][0] << std::endl;

                // Transform coordinates to integers, between [0--width/height].
                int vertex_int[3][3];
                for (int i = 0; i < 3; ++i) {
                    vertex_int[i][0] = round((vertex[i][0] + 1) * (framebuffer_width - 1) / 2);
                    vertex_int[i][1] = round((vertex[i][1] + 1) * (framebuffer_height - 1) / 2);
                    vertex_int[i][2] = round((vertex[i][2] + 1) * (framebuffer_width - 1) / 2);
                }

                // Generate classified polygon table.
                glm::vec3 ab, ac, abc_normal, A;

                ab = glm::vec3(float(vertex_int[1][0] - vertex_int[0][0]), float(vertex_int[1][1] - vertex_int[0][1]), float(vertex_int[1][2] - vertex_int[0][2]));
                ac = glm::vec3{float(vertex_int[2][0] - vertex_int[0][0]), float(vertex_int[2][1] - vertex_int[0][1]), float(vertex_int[2][2] - vertex_int[0][2])};

                A = glm::vec3{vertex_int[0][0], vertex_int[0][1], vertex_int[0][2]};

                abc_normal = glm::cross(ab, ac);
                abc_normal = glm::normalize(abc_normal);

                if (abc_normal.z == 0 || isnan(abc_normal.z))  // The plane is perpendicular to the view plane.
                    continue;

                int minY = round((minmax[1][0] + 1) * (framebuffer_height - 1) / 2);
                int maxY = round((minmax[1][1] + 1) * (framebuffer_height - 1) / 2);
                if (minY < 0)
                    minY = 0;
                if (maxY > framebuffer_height - 1)
                    maxY = framebuffer_height - 1;

                if (maxY <= minY)  // Parallel to the x axis.
                    continue;

                ClassifiedPolygonTable* new_polygon = new ClassifiedPolygonTable();
                new_polygon->id                     = polygon_id;
                new_polygon->dy                     = maxY - minY;
                new_polygon->a                      = abc_normal.x;
                new_polygon->b                      = abc_normal.y;
                new_polygon->c                      = abc_normal.z;
                new_polygon->d                      = -glm::dot(abc_normal, A);  // D in plane equation
                new_polygon->y_max                  = maxY;

                new_polygon->next_p            = classified_polygon_table[maxY];
                classified_polygon_table[maxY] = new_polygon;
                new_polygon                    = nullptr;

                // Generate classified edge table.
                for (int edge_id = 0; edge_id < 3; edge_id++) {
                    glm::vec3 a, b;
                    glm::vec3 color_a, color_b;

                    // The coordinates of the two endpoints of the edge ab.
                    a = glm::vec3{vertex_int[edge_id][0], vertex_int[edge_id][1], vertex_int[edge_id][2]};
                    b = glm::vec3{vertex_int[(edge_id + 1) % 3][0], vertex_int[(edge_id + 1) % 3][1], vertex_int[(edge_id + 1) % 3][2]};

                    // The color of the two endpoints of ab.
                    color_a.r = model_to_scan->shading_color[face_id].face_color[edge_id][0];
                    color_a.g = model_to_scan->shading_color[face_id].face_color[edge_id][1];
                    color_a.b = model_to_scan->shading_color[face_id].face_color[edge_id][2];
                    color_b.r = model_to_scan->shading_color[face_id].face_color[(edge_id + 1) % 3][0];
                    color_b.g = model_to_scan->shading_color[face_id].face_color[(edge_id + 1) % 3][1];
                    color_b.b = model_to_scan->shading_color[face_id].face_color[(edge_id + 1) % 3][2];

                    int       top_y, bottom_y;
                    glm::vec3 top_color, bottom_color, line_color;

                    if (a.y > b.y) {
                        top_y        = a.y;
                        bottom_y     = b.y;
                        top_color    = color_a;
                        bottom_color = color_b;
                    } else {
                        top_y        = b.y;
                        bottom_y     = a.y;
                        top_color    = color_b;
                        bottom_color = color_a;
                    }

                    int minY_edge = bottom_y;
                    int maxY_edge = top_y;

                    if (maxY_edge <= minY_edge)
                        continue;  // If the two are equal, there is no need to scan this line and does not include the classified edge table.

                    // The number of scan lines crossed by the edge.
                    int dy = maxY_edge - minY_edge;

                    // If the end point of the edge is outside the screen, the color should be recalculated.
                    line_color.r = (bottom_color.r - top_color.r) / dy;
                    line_color.g = (bottom_color.g - top_color.g) / dy;
                    line_color.b = (bottom_color.b - top_color.b) / dy;
                    if (maxY_edge >= framebuffer_height - 1) {
                        int cy      = maxY_edge - framebuffer_height + 1;
                        top_color.r = top_color.r + line_color.r * cy;
                        top_color.g = top_color.g + line_color.g * cy;
                        top_color.b = top_color.b + line_color.b * cy;
                        maxY_edge   = framebuffer_height - 1;
                    }

                    // If this edge is not the highest point of the triangle, it means that there is an edge on it, and you need to open a pixel with the upper edge.
                    if (maxY_edge != maxY) {
                        top_color.r = top_color.r + line_color.r;
                        top_color.g = top_color.g + line_color.g;
                        top_color.b = top_color.b + line_color.b;
                        maxY_edge -= 1;
                    }
                    if (minY_edge < 0) {
                        minY_edge = 0;
                    }

                    ClassifiedEdgeTable* new_edge = new ClassifiedEdgeTable();
                    new_edge->id                  = polygon_id;  // id
                    new_edge->x                   = a.x + (b.x - a.x) * (maxY_edge - a.y) / (b.y - a.y);
                    new_edge->dx                  = -(a.x - b.x) / (a.y - b.y);  // dx
                    new_edge->dy                  = maxY_edge - minY_edge;       // dy
                    new_edge->color_top           = top_color;
                    new_edge->color_per_line      = line_color;
                    ;

                    new_edge->next_e                 = classified_edge_table[maxY_edge];
                    classified_edge_table[maxY_edge] = new_edge;
                    new_edge                         = nullptr;
                }

                polygon_id++;
            }
        }

        void ScannerLineZbuffer::SetWindowSize(int width, int height)
        {
            framebuffer_width  = width;
            framebuffer_height = height;

            for (auto p : classified_polygon_table) {
                delete p;
            }
            for (auto p : classified_edge_table) {
                delete p;
            }
            classified_polygon_table.clear();
            classified_edge_table.clear();
            auto temp = active_edge_table;
            while (temp != nullptr) {
                auto t = temp;
                temp   = temp->next_aet;
                delete t;
            }
            temp       = nullptr;
            auto temp2 = active_polygon_table;
            while (temp2 != nullptr) {
                auto t = temp2;
                temp2  = temp2->next_ap;
                delete t;
            }
            temp2 = nullptr;

            active_polygon_table = nullptr;
            active_edge_table    = nullptr;
            classified_polygon_table.resize(framebuffer_height);
            classified_edge_table.resize(framebuffer_height);
        }
    }
}