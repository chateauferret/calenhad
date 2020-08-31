//
// Created by martin on 07/10/2019.
//

#ifndef CALENHAD_RASTERFILEMODULE_H
#define CALENHAD_RASTERFILEMODULE_H

#include "StructuredGrid.h"

namespace calenhad {
    namespace module {


        class RasterModule : public Module {

        public:

            explicit RasterModule (const QString& type = CalenhadServices::preferences() -> calenhad_module_raster);

            void fileDialogRequested();

            QString glsl() override;
            void openFile (const QString& filename);
            bool isComplete() override;
            calenhad::grid::CubicSphere* raster();


        protected slots:

            void inflate (const QDomElement& element) override;

            void serialize (QDomElement& element) override;

        protected:

            QString _filename;
            calenhad::grid::CubicSphere* _cube;
            QWidget* _rasterContent;
            QFormLayout* _rasterLayout{};

            QLabel* _filenameLabel;

            void assimilateRaster (QImage* image);
        };

    }
}


#endif //CALENHAD_RASTERFILEMODULE_H
