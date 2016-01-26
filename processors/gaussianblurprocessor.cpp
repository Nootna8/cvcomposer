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

#include "gaussianblurprocessor.h"

#include <opencv2/imgproc/imgproc.hpp>

#include <QDebug>

#include "cvutils.h"


GaussianBlurProcessor::GaussianBlurProcessor() :
    AbstractProcessor()
{
}

QList<cv::Mat> GaussianBlurProcessor::processImpl(const QList<cv::Mat> &inputs)
{
    cv::Mat blurred = inputs[0].clone();
    cv::GaussianBlur(inputs[0],
                     blurred,
                     getProperty("size").value<cv::Size>(),
                     getProperty("sigmaX").toDouble(),
                     getProperty("sigmaY").toDouble(),
                     getProperty("border").toInt());
    return QList<cv::Mat>() << blurred;
}

