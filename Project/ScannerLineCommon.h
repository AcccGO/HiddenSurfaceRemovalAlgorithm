#pragma once
#include "Model.h"
namespace scanner_line
{
    using namespace model;

    // 分类多边形表
    class ClassifiedPolygonTable
    {
    public:
        int y_max;  // 多边形最大的y坐标

        float a, b, c, d;  // 多边形所在平面的方程系数
        int   id;          // 多边形的编号
        int   dy;          // 多边形跨越扫描线数目

        ClassifiedPolygonTable* next_p = nullptr;

        ClassifiedPolygonTable(){};
        ~ClassifiedPolygonTable()
        {
            next_p = nullptr;
        }
    };

    // 分类边表
    class ClassifiedEdgeTable
    {
    public:
        float x;   // 边的上端点的x坐标
        float dx;  // 相邻两条扫描线交点的x坐标差dx(-1/k)
        int   dy;  // 边跨越的扫描线数目
        int   id;  // 边所属多边形的编号

        glm::vec3 color_top;
        glm::vec3 color_per_line;
        glm::vec3 color_per_col;  // 区间扫描线需要

        ClassifiedEdgeTable* next_e = nullptr;

        ClassifiedEdgeTable(){};
        ~ClassifiedEdgeTable()
        {
            next_e = nullptr;
        }
    };

    // 活化多边形表
    class ActiveEdgeTable;
    class ActivePolygonTable
    {
    public:
        float a, b, c, d;
        int   id;  // 多边形的编号
        int   dy;  // 多边形跨越的剩余扫描线数目

        ActivePolygonTable* next_ap;

        ActivePolygonTable(ClassifiedPolygonTable* cpt);

        ActivePolygonTable(){};
        ~ActivePolygonTable()
        {
            next_ap = nullptr;
        }

        // 区间扫描线需要
        bool      in_flag = false;
        float     x;
        float     z;
        float     dz_x;
        glm::vec3 color;
        glm::vec3 color_per_col;
        ActivePolygonTable(ActivePolygonTable* apt, ActiveEdgeTable* aet);
    };

    // 活化边表:存放投影多边形边界和扫描线相交的边对
    class ActiveEdgeTable
    {
    public:
        // 区间扫描线都用left边的数据
        float xl;   // 左交点的x坐标
        float dxl;  //（左交点边上）两相邻扫描线交点的x坐标之差
        float dyl;  // 以和左交点所在边相交的扫描线数为初值，以后向下每处理一条扫描线减1

        float xr;  // 右交点，同理
        float dxr;
        float dyr;

        float zl;   // 左交点处多边形所在平面的深度值
        float dzx;  // 沿扫描线向右走过一个像素时，多边形所在平面的深度增量。对于平面方程，dzx=-a/c(c!=0)
        float dzy;  // 沿y方向向下移过一根扫描线时，多边形所在平面的深度增量。对于平面方程，dzy=b/c(c!=0)
        int   id;   // 交点对所在的多边形的编号

        glm::vec3 color_top_l;
        glm::vec3 color_per_line_l;
        glm::vec3 color_per_col_l;

        glm::vec3 color_top_r;
        glm::vec3 color_per_line_r;

        ActiveEdgeTable* next_aet = nullptr;

        ActiveEdgeTable(){};
        ~ActiveEdgeTable()
        {
            next_aet = nullptr;
        }
        ActiveEdgeTable(ClassifiedEdgeTable& left_cet, ClassifiedEdgeTable& right_cet, ActivePolygonTable& current_apt, double y)
        {
            this->xl  = left_cet.x;
            this->dxl = left_cet.dx;
            this->dyl = left_cet.dy;
            this->xr  = right_cet.x;
            this->dxr = right_cet.dx;
            this->dyr = right_cet.dy;

            this->zl = (-current_apt.d - current_apt.a * this->xl - current_apt.b * y) / current_apt.c;
            // this->zl = cal_plane_z(current_apt.a, current_apt.b, current_apt.c, current_apt.d, this->x_l, y);
            this->dzx = -current_apt.a / current_apt.c;
            this->dzy = current_apt.b / current_apt.c;
            this->id  = current_apt.id;

            this->color_per_line_l = left_cet.color_per_line;
            this->color_per_line_r = right_cet.color_per_line;
            this->color_top_l      = left_cet.color_top;
            this->color_top_r      = right_cet.color_top;
        }

        ActiveEdgeTable(ClassifiedEdgeTable& left_cet, ActivePolygonTable& current_apt, double y)
        {
            this->xl  = left_cet.x;
            this->dxl = left_cet.dx;
            this->dyl = left_cet.dy;

            this->zl = (-current_apt.d - current_apt.a * this->xl - current_apt.b * y) / current_apt.c;
            // this->zl = cal_plane_z(current_apt.a, current_apt.b, current_apt.c, current_apt.d, this->x_l, y);
            this->dzx = -current_apt.a / current_apt.c;
            this->dzy = current_apt.b / current_apt.c;
            this->id  = current_apt.id;

            this->color_per_line_l = left_cet.color_per_line;

            this->color_top_l = left_cet.color_top;

            this->color_per_col_l = left_cet.color_per_col;
        }
    };
}