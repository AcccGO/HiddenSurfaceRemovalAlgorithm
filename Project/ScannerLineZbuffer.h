#pragma once
#include <Windows.h>
#include <time.h>

#include "ScannerLineCommon.h"

namespace scanner_line
{
    namespace scanner_line_zbuffer
    {
        using namespace std;
        class ScannerLineZbuffer
        {
        public:
            Model* model_to_scan = nullptr;

            vector<ClassifiedEdgeTable*>    classified_edge_table;
            vector<ClassifiedPolygonTable*> classified_polygon_table;
            ActiveEdgeTable*                active_edge_table    = nullptr;
            ActivePolygonTable*             active_polygon_table = nullptr;

            int            framebuffer_width, framebuffer_height;
            unsigned char* frame_buffer = nullptr;
            glm::vec3      bg_color;

        public:
            ScannerLineZbuffer(int width, int height);
            ScannerLineZbuffer(Model* current_model, int width, int height);
            ~ScannerLineZbuffer();

            int  ScanModel(int width, int height);
            void MainAlgorithm();
            void InitData();
            void SetWindowSize(int width, int height);
            int  FindEdgePair(int y_id, int polygon_id, ClassifiedEdgeTable* edge_pair[2]);
            void FillLineColor(int y_id, int start, int end, glm::vec3 color, glm::vec3 color_per_col);
            void SetClearColor(glm::vec3 color);
            void ResetModel(Model* new_model);
        };
    }
}