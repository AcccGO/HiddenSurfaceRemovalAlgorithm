#pragma once
#include "Model.h"
namespace scanner_line
{
    using namespace model;

    // �������α�
    class ClassifiedPolygonTable
    {
    public:
        int y_max;  // ���������y����

        float a, b, c, d;  // ���������ƽ��ķ���ϵ��
        int   id;          // ����εı��
        int   dy;          // ����ο�Խɨ������Ŀ

        ClassifiedPolygonTable* next_p = nullptr;

        ClassifiedPolygonTable(){};
        ~ClassifiedPolygonTable()
        {
            next_p = nullptr;
        }
    };

    // ����߱�
    class ClassifiedEdgeTable
    {
    public:
        float x;   // �ߵ��϶˵��x����
        float dx;  // ��������ɨ���߽����x�����dx(-1/k)
        int   dy;  // �߿�Խ��ɨ������Ŀ
        int   id;  // ����������εı��

        glm::vec3 color_top;
        glm::vec3 color_per_line;
        glm::vec3 color_per_col;  // ����ɨ������Ҫ

        ClassifiedEdgeTable* next_e = nullptr;

        ClassifiedEdgeTable(){};
        ~ClassifiedEdgeTable()
        {
            next_e = nullptr;
        }
    };

    // �����α�
    class ActiveEdgeTable;
    class ActivePolygonTable
    {
    public:
        float a, b, c, d;
        int   id;  // ����εı��
        int   dy;  // ����ο�Խ��ʣ��ɨ������Ŀ

        ActivePolygonTable* next_ap;

        ActivePolygonTable(ClassifiedPolygonTable* cpt);

        ActivePolygonTable(){};
        ~ActivePolygonTable()
        {
            next_ap = nullptr;
        }

        // ����ɨ������Ҫ
        bool      in_flag = false;
        float     x;
        float     z;
        float     dz_x;
        glm::vec3 color;
        glm::vec3 color_per_col;
        ActivePolygonTable(ActivePolygonTable* apt, ActiveEdgeTable* aet);
    };

    // ��߱�:���ͶӰ����α߽��ɨ�����ཻ�ı߶�
    class ActiveEdgeTable
    {
    public:
        // ����ɨ���߶���left�ߵ�����
        float xl;   // �󽻵��x����
        float dxl;  //���󽻵���ϣ�������ɨ���߽����x����֮��
        float dyl;  // �Ժ��󽻵����ڱ��ཻ��ɨ������Ϊ��ֵ���Ժ�����ÿ����һ��ɨ���߼�1

        float xr;  // �ҽ��㣬ͬ��
        float dxr;
        float dyr;

        float zl;   // �󽻵㴦���������ƽ������ֵ
        float dzx;  // ��ɨ���������߹�һ������ʱ�����������ƽ����������������ƽ�淽�̣�dzx=-a/c(c!=0)
        float dzy;  // ��y���������ƹ�һ��ɨ����ʱ�����������ƽ����������������ƽ�淽�̣�dzy=b/c(c!=0)
        int   id;   // ��������ڵĶ���εı��

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