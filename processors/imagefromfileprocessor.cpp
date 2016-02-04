// Copyright 2016 Erwan MATHIEU <wawanbreton@gmail.com>
//
// This file is part of CvComposer.
//
// CvComposer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// CvComposer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with CvComposer.  If not, see <http://www.gnu.org/licenses/>.

#include "imagefromfileprocessor.h"

#include <opencv2/highgui/highgui.hpp>

#include <QDebug>

#include "cvutils.h"


ImageFromFileProcessor::ImageFromFileProcessor() :
    AbstractProcessor()
{
    addInput("path",   PlugType::ImagePath);
    addOutput("image", PlugType::Image);
}

Properties ImageFromFileProcessor::processImpl(const Properties &inputs)
{
    Q_UNUSED(inputs);

    Properties outputs;
    outputs.insert("image", QVariant::fromValue(cv::imread(inputs["path"].toString().toStdString(),
                                                           CV_LOAD_IMAGE_COLOR)));

    return outputs;
}
