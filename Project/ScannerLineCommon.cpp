#include "ScannerLineCommon.h"
namespace scanner_line
{
    ActivePolygonTable::ActivePolygonTable(ActivePolygonTable* apt, ActiveEdgeTable* aet)
    {
        this->id = apt->id;
        this->a  = apt->a;
        this->b  = apt->b;
        this->c  = apt->c;
        this->d  = apt->d;

        this->z             = aet->zl;
        this->x             = aet->xl;
        this->dz_x          = aet->dzx;
        this->color         = aet->color_top_l;
        this->color_per_col = aet->color_per_col_l;
    }

    ActivePolygonTable::ActivePolygonTable(ClassifiedPolygonTable* cpt)
    {
        b  = cpt->b;
        c  = cpt->c;
        a  = cpt->a;
        d  = cpt->d;
        id = cpt->id;
        dy = cpt->dy;

        next_ap = nullptr;
    }
}
