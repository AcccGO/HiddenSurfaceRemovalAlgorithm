#include "IntervalScannerLineZbuffer.h"

#include <iostream>

#include "Parameter.h"

using namespace std;

namespace scanner_line
{
    namespace interval_scanner_line_zbuffer
    {
        void IntervalScannerLineZbuffer::SetClearColor(glm::vec3 color)
        {
            bg_color = color;
        }

        void IntervalScannerLineZbuffer::ResetModel(Model* new_model)
        {
            model_to_scan = new_model;
        }

        IntervalScannerLineZbuffer::IntervalScannerLineZbuffer(int size)
        {
            this->model_to_scan = nullptr;
            framebuffer_width   = size;
            framebuffer_height  = size;

            classified_polygon_table.resize(framebuffer_height);
            classified_edge_table.resize(framebuffer_height);
            // Create the biggest.
            frame_buffer = new unsigned char[1920 * 1024 * 3];
        }
        IntervalScannerLineZbuffer::IntervalScannerLineZbuffer(Model* current_model, int size)
        {
            this->model_to_scan = current_model;
            framebuffer_width   = size;
            framebuffer_height  = size;

            classified_polygon_table.resize(framebuffer_height);
            classified_edge_table.resize(framebuffer_height);

            frame_buffer = new unsigned char[1920 * 1024 * 3];
        }
        IntervalScannerLineZbuffer::~IntervalScannerLineZbuffer()
        {
            this->model_to_scan = nullptr;
        }

        void IntervalScannerLineZbuffer::InitData()
        {
            int face_id    = 0;
            int polygon_id = 0;

            for (face_id = 0; face_id < model_to_scan->obj_faces.size(); face_id++) {
                Face* face = &model_to_scan->obj_faces[face_id];
                float vertex[3][3];
                float color[3][3];
                float minmax[3][2];

                // Find the maximum and minimum value of XYZ in a face.
                for (int i = 0; i < 3; ++i) {
                    if (i == 0) {
                        for (int j = 0; j < 3; j++) {
                            // FIXED: Index by vertex id in the face instead of i.
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

                // Determine whether the triangle is in the boundary.
                // The projection coordinates are between -1 and 1.
                bool if_in_area = false;
                for (int i = 0; i < 3; ++i) {
                    if (minmax[i][0] > 1 || minmax[i][1] < -1) {
                        if_in_area = true;
                        break;
                    }
                }
                if (if_in_area)
                    continue;

                // Handles points where the x-coordinate is outside the range -1--1.
                if (vertex[0][0] > 1 || vertex[0][0] < -1) std::cout << face_id << vertex[0][0] << std::endl;
                if (vertex[1][0] > 1 || vertex[1][0] < -1) std::cout << face_id << vertex[1][0] << std::endl;
                if (vertex[2][0] > 1 || vertex[2][0] < -1) std::cout << face_id << vertex[2][0] << std::endl;

                // Convert coordinates to integers between [0--width/height].
                int vertex_int[3][3];
                for (int i = 0; i < 3; ++i) {
                    vertex_int[i][0] = ceil((vertex[i][0] + 1) * (framebuffer_width - 1) / 2);
                    vertex_int[i][1] = ceil((vertex[i][1] + 1) * (framebuffer_height - 1) / 2);
                    vertex_int[i][2] = ceil((vertex[i][2] + 1) * (framebuffer_width - 1) / 2);
                }

                // Generate classified polygon table
                glm::vec3 ab, ac, abc_normal, A;

                // FIXED: Here GLM ::vec3 is best found with.x, direct [0] may be problematic.
                ab = glm::vec3(float(vertex_int[1][0] - vertex_int[0][0]), float(vertex_int[1][1] - vertex_int[0][1]), float(vertex_int[1][2] - vertex_int[0][2]));
                ac = glm::vec3{float(vertex_int[2][0] - vertex_int[0][0]), float(vertex_int[2][1] - vertex_int[0][1]), float(vertex_int[2][2] - vertex_int[0][2])};

                A = glm::vec3{vertex_int[0][0], vertex_int[0][1], vertex_int[0][2]};

                abc_normal = glm::cross(ab, ac);
                abc_normal = glm::normalize(abc_normal);

                if (abc_normal.z == 0 || isnan(abc_normal.z))
                    continue;

                int face_y_min = ceil((minmax[1][0] + 1) * (framebuffer_height - 1) / 2);
                int face_y_max = ceil((minmax[1][1] + 1) * (framebuffer_height - 1) / 2);
                if (face_y_min < 0)
                    face_y_min = 0;
                if (face_y_max > framebuffer_height - 1)
                    face_y_max = framebuffer_height - 1;

                if (face_y_max <= face_y_min)
                    continue;

                ClassifiedPolygonTable* new_polygon = new ClassifiedPolygonTable();
                new_polygon->id                     = polygon_id;
                new_polygon->dy                     = face_y_max - face_y_min;
                new_polygon->a                      = abc_normal.x;
                new_polygon->b                      = abc_normal.y;
                new_polygon->c                      = abc_normal.z;
                new_polygon->d                      = -glm::dot(abc_normal, A);  // D in the plane equation.
                new_polygon->y_max                  = face_y_max;

                new_polygon->next_p                  = classified_polygon_table[face_y_max];
                classified_polygon_table[face_y_max] = new_polygon;
                new_polygon                          = nullptr;

                // TODO: A problem with interpolation.
                /*glm::vec3 color_ab = model_to_scan->shading_color[face_id].face_color[1] - model_to_scan->shading_color[face_id].face_color[0];
                glm::vec3 color_ac = model_to_scan->shading_color[face_id].face_color[2] - model_to_scan->shading_color[face_id].face_color[0];

                glm::vec3 y2c1 = color_ab * ac.y;
                glm::vec3 y1c2 = color_ac * ab.y;
                glm::vec3 x1c2 = color_ac * ab.x;
                glm::vec3 x2c1 = color_ab * ac.x;
                glm::vec3 color_per_line = (x1c2 - x2c1)* 1.0f / (ab.x*ac.y - ab.y*ac.x);;
                glm::vec3 color_per_col = (y2c1 - y1c2)* 1.0f / (ab.x*ac.y - ab.y*ac.x);*/

                // Generate the classification side table.
                for (int edge_id = 0; edge_id < 3; edge_id++) {
                    glm::vec3 a, b;
                    // glm::vec3 color_a, color_b;

                    // The coordinates of the a and b endpoints of the edge.
                    a = glm::vec3{vertex_int[edge_id][0], vertex_int[edge_id][1], vertex_int[edge_id][2]};
                    b = glm::vec3{vertex_int[(edge_id + 1) % 3][0], vertex_int[(edge_id + 1) % 3][1], vertex_int[(edge_id + 1) % 3][2]};

                    // The color of the a and b endpoints of the edge.
                    /*color_a.r = model_to_scan->shading_color[face_id].face_color[edge_id][0];
                    color_a.g = model_to_scan->shading_color[face_id].face_color[edge_id][1];
                    color_a.b = model_to_scan->shading_color[face_id].face_color[edge_id][2];
                    color_b.r = model_to_scan->shading_color[face_id].face_color[(edge_id + 1) % 3][0];
                    color_b.g = model_to_scan->shading_color[face_id].face_color[(edge_id + 1) % 3][1];
                    color_b.b = model_to_scan->shading_color[face_id].face_color[(edge_id + 1) % 3][2];*/

                    int top_y, bottom_y;
                    glm::vec3 top_color, bottom_color, line_color;

                    if (a.y > b.y) {
                        top_y    = a.y;
                        bottom_y = b.y;
                        // top_color = color_a;
                        // bottom_color = color_b;
                    } else {
                        top_y    = b.y;
                        bottom_y = a.y;
                        // top_color = color_b;
                        // bottom_color = color_a;
                    }

                    int edge_y_min = bottom_y;
                    int edge_y_max = top_y;
                    // If the two are equal, there is no need to scan this line and it is not included in the classification side table.
                    if (edge_y_max <= edge_y_min)
                        continue;

                    // The number of scan lines crossed by an edge.
                    int dy = edge_y_max - edge_y_min;

                    // If one end of the edge is out of screen range, the color should be recalculated.
                    // line_color.r = (bottom_color.r - top_color.r) / dy;
                    // line_color.g = (bottom_color.g - top_color.g) / dy;
                    // line_color.b = (bottom_color.b - top_color.b) / dy;
                    if (edge_y_max >= framebuffer_height - 1) {
                        int cy = edge_y_max - framebuffer_height + 1;
                        // top_color.r = top_color.r + line_color.r * cy;
                        // top_color.g = top_color.g + line_color.g * cy;
                        // top_color.b = top_color.b + line_color.b * cy;
                        edge_y_max = framebuffer_height - 1;
                    }

                    // If the edge is not the highest point of the triangle, there is another edge above it, which needs to be separated by a pixel.
                    if (edge_y_max != face_y_max) {
                        // top_color.r = top_color.r + line_color.r;
                        // top_color.g = top_color.g + line_color.g;
                        // top_color.b = top_color.b + line_color.b;
                        edge_y_max -= 1;
                    }
                    if (edge_y_min < 0) {
                        edge_y_min = 0;
                    }

                    ClassifiedEdgeTable* new_edge = new ClassifiedEdgeTable();
                    new_edge->id                  = polygon_id;  // id
                    new_edge->x                   = a.x + (b.x - a.x) * (edge_y_max - a.y) / (b.y - a.y);
                    new_edge->dx                  = -(a.x - b.x) / (a.y - b.y);  // dx
                    new_edge->dy                  = edge_y_max - edge_y_min;     // dy
                    // new_edge->color_top = top_color;
                    // new_edge->color_per_line = line_color;

                    new_edge->next_e                  = classified_edge_table[edge_y_max];
                    classified_edge_table[edge_y_max] = new_edge;
                    new_edge                          = nullptr;
                }

                polygon_id++;
            }
        }

        void IntervalScannerLineZbuffer::FillLineColor(int y_id, int start = 0, int end = -1, glm::vec3 color = glm::vec3{-1}, glm::vec3 color_per_col = glm::vec3{0})
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

        void IntervalScannerLineZbuffer::FillAreaColor(int y_id, ActivePolygonTable* ipl, int start, int end)
        {
            pair<int, int> current_area_xl_xr = pair<int, int>(start, end);

            float z_l, z_r;
            z_l                                   = ipl->z + ipl->dz_x * (start - ipl->x);
            z_r                                   = ipl->z + ipl->dz_x * (end - ipl->x);
            pair<float, float> current_area_zl_zr = pair<float, float>(z_l, z_r);

            // glm::vec3 current_color_l = ipl->color_per_col*(start - ipl->x) + ipl->color;
            // glm::vec3 curren_color_d = ipl->color_per_col;

            ActivePolygonTable* ipl_head = ipl->next_ap;
            while (ipl_head != nullptr) {
                float new_z_l, new_z_r;
                new_z_l = ipl_head->z + ipl_head->dz_x * (start - ipl_head->x);
                new_z_r = ipl_head->z + ipl_head->dz_x * (end - ipl_head->x);

                // Without penetration.
                if (new_z_l + new_z_r > current_area_zl_zr.first + current_area_zl_zr.second) {
                    current_area_zl_zr.first  = new_z_l;
                    current_area_zl_zr.second = new_z_r;
                    // current_color_l = ipl_head->color + (start - ipl_head->x)*ipl_head->color_per_col;
                    // curren_color_d = ipl_head->color_per_col;
                }

                ipl_head = ipl_head->next_ap;
            }

            FillLineColor(y_id, start, end, model_color);
        }

        ActivePolygonTable* IntervalScannerLineZbuffer::UpdateIPL(ActivePolygonTable* IPL, ActivePolygonTable* current_apt, ActiveEdgeTable* current_aet)
        {
            ActivePolygonTable* ipl = IPL;

            ActivePolygonTable* ipl_start = new ActivePolygonTable();
            ipl_start->next_ap            = ipl;

            ActivePolygonTable* ipl_last    = ipl_start;
            ActivePolygonTable* ipl_current = ipl;
            while (ipl_current != nullptr) {
                // If an edge has been paired with it before, delete it.
                if (ipl_current->id == current_apt->id) {
                    ipl_last->next_ap = ipl_current->next_ap;
                    ipl               = ipl_start->next_ap;
                    delete ipl_start;
                    return ipl;
                }
                ipl_last    = ipl_last->next_ap;
                ipl_current = ipl_current->next_ap;
            }

            ActivePolygonTable* new_ipl = new ActivePolygonTable(current_apt, current_aet);
            new_ipl->next_ap            = ipl;
            ipl                         = new_ipl;

            return ipl;
        }

        void IntervalScannerLineZbuffer::SetWindowSize(int width, int height)
        {
            framebuffer_width  = width;
            framebuffer_height = height;

            // TODO: Wouldn't it be a good idea to cut each frame frequently?
            for (auto p : classified_polygon_table) {
                ClassifiedPolygonTable* temp = nullptr;
                if (p != nullptr) {
                    while (p->next_p != nullptr) {
                        temp = p;
                        p    = p->next_p;
                        delete temp;
                    }
                }
                delete p;
            }
            for (auto p : classified_edge_table) {
                ClassifiedEdgeTable* temp = nullptr;
                if (p != nullptr) {
                    while (p->next_e != nullptr) {
                        temp = p;
                        p    = p->next_e;
                        delete temp;
                    }
                }
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

        int IntervalScannerLineZbuffer::ScanModel(int width, int height)
        {
            SetWindowSize(width, height);
            model_to_scan->TransformModel();
            InitData();

            // Algorithm
            DWORD t1, t2;
            t1 = GetTickCount();
            MainAlgorithm();
            t2 = GetTickCount();

            return t2 - t1;
        }

        void IntervalScannerLineZbuffer::MainAlgorithm()
        {
            int y_id;

            // Start a line by line scan
            for (y_id = framebuffer_height - 1; y_id >= 0; y_id--) {
                ActivePolygonTable* apt_head = active_polygon_table;
                ActiveEdgeTable* aet_head    = active_edge_table;

                // Check for new polygons to be added************************************
                apt_head = active_polygon_table;
                if (classified_polygon_table[y_id] != nullptr) {
                    ClassifiedPolygonTable* cpt_head = classified_polygon_table[y_id];
                    if (apt_head != nullptr)
                        while (apt_head->next_ap != nullptr) {
                            apt_head = apt_head->next_ap;
                        }
                    // Add polygons to APT in turn.
                    while (cpt_head != nullptr) {
                        ActivePolygonTable* new_polygon = new ActivePolygonTable(cpt_head);
                        if (apt_head == nullptr) {
                            active_polygon_table = new_polygon;
                            apt_head             = active_polygon_table;
                        } else {
                            apt_head->next_ap = new_polygon;
                            apt_head          = apt_head->next_ap;
                        }
                        cpt_head = cpt_head->next_p;  // Find the next CPT.
                    }
                }

                // Check for new classification edges to add************************************
                aet_head = active_edge_table;
                while (aet_head && aet_head->next_aet)  // Refers to the end of aet for easy addition.
                {
                    aet_head = aet_head->next_aet;
                }
                ClassifiedEdgeTable* cet_head = classified_edge_table[y_id];
                while (cet_head != nullptr) {
                    apt_head = active_polygon_table;  // Refers to head
                    while (apt_head) {
                        if (apt_head->id == cet_head->id) {
                            ActiveEdgeTable* p_aE = new ActiveEdgeTable(*cet_head, *apt_head, y_id);

                            if (active_edge_table == nullptr) {
                                active_edge_table = p_aE;
                                aet_head          = active_edge_table;
                            } else {
                                aet_head->next_aet = p_aE;
                                aet_head           = p_aE;
                            }
                            // Break the APT lookup loop.
                            break;
                        } else
                            apt_head = apt_head->next_ap;
                    }
                    cet_head = cet_head->next_e;
                }
                // Sort by x size from left to right
                aet_head = active_edge_table;
                if (aet_head != nullptr && aet_head->next_aet != nullptr) {
                    ActiveEdgeTable* current_aet = aet_head->next_aet;
                    ActiveEdgeTable* start_aet   = new ActiveEdgeTable();
                    start_aet->next_aet          = aet_head;
                    ActiveEdgeTable* end_aet     = aet_head;

                    while (current_aet) {
                        ActiveEdgeTable* temp     = start_aet->next_aet;
                        ActiveEdgeTable* last_aet = start_aet;

                        while (temp != current_aet && temp->xl <= current_aet->xl) {
                            temp     = temp->next_aet;
                            last_aet = last_aet->next_aet;
                        }
                        if (temp == current_aet) {
                            end_aet = current_aet;
                        } else {
                            end_aet->next_aet     = current_aet->next_aet;
                            current_aet->next_aet = temp;
                            last_aet->next_aet    = current_aet;
                        }
                        current_aet = end_aet->next_aet;
                    }

                    active_edge_table = start_aet->next_aet;  // Points to a reordered linked list.
                    delete start_aet;
                }

                // Update the frame Buffer contents************************************
                aet_head = active_edge_table;
                if (aet_head == nullptr) {
                    // Fills the current scan line with a background color.
                    FillLineColor(y_id);
                } else {
                    ActivePolygonTable* IPL   = nullptr;
                    aet_head                  = active_edge_table;
                    ActiveEdgeTable* aet_next = aet_head->next_aet;
                    // Fill the background color with the current scan line to the leftmost polygon.
                    FillLineColor(y_id, 0, aet_head->xl);
                    while (aet_next != nullptr) {
                        // For new AET, update Polygon in.
                        apt_head = active_polygon_table;
                        while (apt_head != nullptr) {
                            if (apt_head->id == aet_head->id) {
                                // Update polygon in.
                                IPL = UpdateIPL(IPL, apt_head, aet_head);
                                break;
                            } else {
                                apt_head = apt_head->next_ap;
                            }
                        }
                        if (aet_head->xl == aet_next->xl)  // X is the same, no padding required.
                        {
                            aet_head = aet_head->next_aet;
                            aet_next = aet_next->next_aet;
                            continue;
                        }
                        // Filling scan interval.
                        if (IPL == nullptr)
                            FillLineColor(y_id, aet_head->xl, aet_next->xl);
                        else
                            FillAreaColor(y_id, IPL, aet_head->xl, aet_next->xl);

                        // Update AET and AET_next.
                        aet_head = aet_head->next_aet;
                        aet_next = aet_next->next_aet;
                    }
                    FillLineColor(y_id, aet_head->xl);

                    // Delete IPL.
                    while (IPL != nullptr) {
                        ActivePolygonTable* temp = IPL;
                        IPL                      = IPL->next_ap;
                        delete temp;
                    }
                }

                // Update the content of the activated polygon table and the activated side table.
                apt_head                     = active_polygon_table;
                ActivePolygonTable* apt_last = apt_head;
                aet_head                     = active_edge_table;
                ActiveEdgeTable* aet_last    = aet_head;
                // Update apt.
                while (apt_head != nullptr) {
                    apt_head->dy -= 1;
                    if (apt_head->dy < 0) {
                        if (apt_head == active_polygon_table) {
                            active_polygon_table = apt_head->next_ap;
                            free(apt_head);
                            apt_head = active_polygon_table;
                            apt_last = apt_head;
                        } else {
                            ActivePolygonTable* temp = apt_head;
                            apt_last->next_ap        = apt_head->next_ap;
                            apt_head                 = apt_last->next_ap;
                            delete temp;
                        }
                    } else {
                        apt_last = apt_head;
                        apt_head = apt_head->next_ap;
                    }
                }
                // Update aet.
                while (aet_head != nullptr) {
                    aet_head->xl += aet_head->dxl;
                    aet_head->zl += aet_head->dzx * aet_head->dxl + aet_head->dzy;

                    aet_head->dyl -= 1;
                    if (aet_head->dyl < 0) {
                        if (aet_head == active_edge_table) {
                            active_edge_table = aet_head->next_aet;
                            free(aet_head);
                            aet_head = active_edge_table;
                            aet_last = aet_head;
                        } else {
                            ActiveEdgeTable* temp = aet_head;
                            aet_last->next_aet    = aet_head->next_aet;
                            aet_head              = aet_last->next_aet;
                            delete temp;
                        }
                    } else {
                        aet_last = aet_head;
                        aet_head = aet_head->next_aet;
                    }
                }
            }
        }
    }
}