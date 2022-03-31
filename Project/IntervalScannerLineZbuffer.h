#pragma once
#include <Windows.h>
#include <time.h>

#include "ScannerLineCommon.h"
using namespace std;

namespace scanner_line
{
    namespace interval_scanner_line_zbuffer
    {

        class IntervalScannerLineZbuffer
        {
        public:
            Model* model_to_scan = nullptr;

            vector<ClassifiedEdgeTable*> classified_edge_table;
            vector<ClassifiedPolygonTable*> classified_polygon_table;
            ActiveEdgeTable* active_edge_table       = nullptr;
            ActivePolygonTable* active_polygon_table = nullptr;

            int framebuffer_width, framebuffer_height;
            glm::vec3 bg_color          = glm::vec3(0.0f);
            unsigned char* frame_buffer = nullptr;

            IntervalScannerLineZbuffer(int size);
            IntervalScannerLineZbuffer(Model* current_model, int size);
            ~IntervalScannerLineZbuffer();

            void InitData();
            int ScanModel(int width, int height);
            void MainAlgorithm();
            void FillLineColor(int y_id, int start, int end, glm::vec3 color, glm::vec3 color_per_col);
            void FillAreaColor(int y_id, ActivePolygonTable* ipl, int start, int end);
            ActivePolygonTable* UpdateIPL(ActivePolygonTable* IPL, ActivePolygonTable* current_apt, ActiveEdgeTable* current_aet);
            void SetWindowSize(int width, int height);
            void SetClearColor(glm::vec3 color);
            void ResetModel(Model* new_model);
        };
    }
}
