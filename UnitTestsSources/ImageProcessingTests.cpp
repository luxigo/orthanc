/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2019 Osimis S.A., Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "PrecompiledHeadersUnitTests.h"
#include "gtest/gtest.h"

#include "../Core/DicomFormat/DicomImageInformation.h"
#include "../Core/Images/Image.h"
#include "../Core/Images/ImageProcessing.h"
#include "../Core/Images/ImageTraits.h"
#include "../Core/OrthancException.h"

#include <memory>

using namespace Orthanc;


TEST(DicomImageInformation, ExtractPixelFormat1)
{
  // Cardiac/MR*
  DicomMap m;
  m.SetValue(DICOM_TAG_ROWS, "24", false);
  m.SetValue(DICOM_TAG_COLUMNS, "16", false);
  m.SetValue(DICOM_TAG_BITS_ALLOCATED, "16", false);
  m.SetValue(DICOM_TAG_SAMPLES_PER_PIXEL, "1", false);
  m.SetValue(DICOM_TAG_BITS_STORED, "12", false);
  m.SetValue(DICOM_TAG_HIGH_BIT, "11", false);
  m.SetValue(DICOM_TAG_PIXEL_REPRESENTATION, "0", false);
  m.SetValue(DICOM_TAG_PHOTOMETRIC_INTERPRETATION, "MONOCHROME2", false);

  DicomImageInformation info(m);
  PixelFormat format;
  ASSERT_TRUE(info.ExtractPixelFormat(format, false));
  ASSERT_EQ(PixelFormat_Grayscale16, format);
}


TEST(DicomImageInformation, ExtractPixelFormat2)
{
  // Delphine CT
  DicomMap m;
  m.SetValue(DICOM_TAG_ROWS, "24", false);
  m.SetValue(DICOM_TAG_COLUMNS, "16", false);
  m.SetValue(DICOM_TAG_BITS_ALLOCATED, "16", false);
  m.SetValue(DICOM_TAG_SAMPLES_PER_PIXEL, "1", false);
  m.SetValue(DICOM_TAG_BITS_STORED, "16", false);
  m.SetValue(DICOM_TAG_HIGH_BIT, "15", false);
  m.SetValue(DICOM_TAG_PIXEL_REPRESENTATION, "1", false);
  m.SetValue(DICOM_TAG_PHOTOMETRIC_INTERPRETATION, "MONOCHROME2", false);

  DicomImageInformation info(m);
  PixelFormat format;
  ASSERT_TRUE(info.ExtractPixelFormat(format, false));
  ASSERT_EQ(PixelFormat_SignedGrayscale16, format);
}



namespace
{
  template <typename T>
  class TestImageTraits : public ::testing::Test
  {
  private:
    std::auto_ptr<Image>  image_;

  protected:
    virtual void SetUp() ORTHANC_OVERRIDE
    {
      image_.reset(new Image(ImageTraits::PixelTraits::GetPixelFormat(), 7, 9, false));
    }

    virtual void TearDown() ORTHANC_OVERRIDE
    {
      image_.reset(NULL);
    }

  public:
    typedef T ImageTraits;
    
    ImageAccessor& GetImage()
    {
      return *image_;
    }
  };

  template <typename T>
  class TestIntegerImageTraits : public TestImageTraits<T>
  {
  };
}


typedef ::testing::Types<
  ImageTraits<PixelFormat_Grayscale8>,
  ImageTraits<PixelFormat_Grayscale16>,
  ImageTraits<PixelFormat_SignedGrayscale16>
  > IntegerFormats;
TYPED_TEST_CASE(TestIntegerImageTraits, IntegerFormats);

typedef ::testing::Types<
  ImageTraits<PixelFormat_Grayscale8>,
  ImageTraits<PixelFormat_Grayscale16>,
  ImageTraits<PixelFormat_SignedGrayscale16>,
  ImageTraits<PixelFormat_RGB24>,
  ImageTraits<PixelFormat_BGRA32>
  > AllFormats;
TYPED_TEST_CASE(TestImageTraits, AllFormats);


TYPED_TEST(TestImageTraits, SetZero)
{
  ImageAccessor& image = this->GetImage();
  
  memset(image.GetBuffer(), 128, image.GetHeight() * image.GetWidth());

  switch (image.GetFormat())
  {
    case PixelFormat_Grayscale8:
    case PixelFormat_Grayscale16:
    case PixelFormat_SignedGrayscale16:
      ImageProcessing::Set(image, 0);
      break;

    case PixelFormat_RGB24:
    case PixelFormat_BGRA32:
      ImageProcessing::Set(image, 0, 0, 0, 0);
      break;

    default:
      ASSERT_TRUE(0);
  }

  typename TestFixture::ImageTraits::PixelType zero, value;
  TestFixture::ImageTraits::PixelTraits::SetZero(zero);

  for (unsigned int y = 0; y < image.GetHeight(); y++)
  {
    for (unsigned int x = 0; x < image.GetWidth(); x++)
    {
      TestFixture::ImageTraits::GetPixel(value, image, x, y);
      ASSERT_TRUE(TestFixture::ImageTraits::PixelTraits::IsEqual(zero, value));
    }
  }
}


TYPED_TEST(TestIntegerImageTraits, SetZeroFloat)
{
  ImageAccessor& image = this->GetImage();
  
  memset(image.GetBuffer(), 128, image.GetHeight() * image.GetWidth());

  float c = 0.0f;
  for (unsigned int y = 0; y < image.GetHeight(); y++)
  {
    for (unsigned int x = 0; x < image.GetWidth(); x++, c++)
    {
      TestFixture::ImageTraits::SetFloatPixel(image, c, x, y);
    }
  }

  c = 0.0f;
  for (unsigned int y = 0; y < image.GetHeight(); y++)
  {
    for (unsigned int x = 0; x < image.GetWidth(); x++, c++)
    {
      ASSERT_FLOAT_EQ(c, TestFixture::ImageTraits::GetFloatPixel(image, x, y));
    }
  }
}

TYPED_TEST(TestIntegerImageTraits, FillPolygon)
{
  ImageAccessor& image = this->GetImage();

  ImageProcessing::Set(image, 128);

  // draw a triangle
  std::vector<ImageProcessing::ImagePoint> points;
  points.push_back(ImageProcessing::ImagePoint(1,1));
  points.push_back(ImageProcessing::ImagePoint(1,5));
  points.push_back(ImageProcessing::ImagePoint(5,5));

  ImageProcessing::FillPolygon(image, points, 255);

  // outside polygon
  ASSERT_FLOAT_EQ(128, TestFixture::ImageTraits::GetFloatPixel(image, 0, 0));
  ASSERT_FLOAT_EQ(128, TestFixture::ImageTraits::GetFloatPixel(image, 0, 6));
  ASSERT_FLOAT_EQ(128, TestFixture::ImageTraits::GetFloatPixel(image, 6, 6));
  ASSERT_FLOAT_EQ(128, TestFixture::ImageTraits::GetFloatPixel(image, 6, 0));

  ASSERT_FLOAT_EQ(255, TestFixture::ImageTraits::GetFloatPixel(image, 1, 1));
  ASSERT_FLOAT_EQ(255, TestFixture::ImageTraits::GetFloatPixel(image, 1, 2));
  ASSERT_FLOAT_EQ(255, TestFixture::ImageTraits::GetFloatPixel(image, 1, 5));
  ASSERT_FLOAT_EQ(255, TestFixture::ImageTraits::GetFloatPixel(image, 2, 4));
  ASSERT_FLOAT_EQ(255, TestFixture::ImageTraits::GetFloatPixel(image, 5, 5));
}

TYPED_TEST(TestIntegerImageTraits, FillPolygonLargerThanImage)
{
  ImageAccessor& image = this->GetImage();

  ImageProcessing::Set(image, 0);

  std::vector<ImageProcessing::ImagePoint> points;
  points.push_back(ImageProcessing::ImagePoint(0, 0));
  points.push_back(ImageProcessing::ImagePoint(image.GetWidth(),0));
  points.push_back(ImageProcessing::ImagePoint(image.GetWidth(),image.GetHeight()));
  points.push_back(ImageProcessing::ImagePoint(0,image.GetHeight()));

  ASSERT_THROW(ImageProcessing::FillPolygon(image, points, 255), OrthancException);
}

TYPED_TEST(TestIntegerImageTraits, FillPolygonFullImage)
{
  ImageAccessor& image = this->GetImage();

  ImageProcessing::Set(image, 0);

  std::vector<ImageProcessing::ImagePoint> points;
  points.push_back(ImageProcessing::ImagePoint(0, 0));
  points.push_back(ImageProcessing::ImagePoint(image.GetWidth() - 1,0));
  points.push_back(ImageProcessing::ImagePoint(image.GetWidth() - 1,image.GetHeight() - 1));
  points.push_back(ImageProcessing::ImagePoint(0,image.GetHeight() - 1));

  ImageProcessing::FillPolygon(image, points, 255);

  ASSERT_FLOAT_EQ(255, TestFixture::ImageTraits::GetFloatPixel(image, 0, 0));
  ASSERT_FLOAT_EQ(255, TestFixture::ImageTraits::GetFloatPixel(image, image.GetWidth() - 1, image.GetHeight() - 1));
}




static void SetGrayscale8Pixel(ImageAccessor& image,
                               unsigned int x,
                               unsigned int y,
                               uint8_t value)
{
  ImageTraits<PixelFormat_Grayscale8>::SetPixel(image, value, x, y);
}

static bool TestGrayscale8Pixel(const ImageAccessor& image,
                                unsigned int x,
                                unsigned int y,
                                uint8_t value)
{
  PixelTraits<PixelFormat_Grayscale8>::PixelType p;
  ImageTraits<PixelFormat_Grayscale8>::GetPixel(p, image, x, y);
  return p == value;
}

static void SetRGB24Pixel(ImageAccessor& image,
                          unsigned int x,
                          unsigned int y,
                          uint8_t red,
                          uint8_t green,
                          uint8_t blue)
{
  PixelTraits<PixelFormat_RGB24>::PixelType p;
  p.red_ = red;
  p.green_ = green;
  p.blue_ = blue;
  ImageTraits<PixelFormat_RGB24>::SetPixel(image, p, x, y);
}

static bool TestRGB24Pixel(const ImageAccessor& image,
                           unsigned int x,
                           unsigned int y,
                           uint8_t red,
                           uint8_t green,
                           uint8_t blue)
{
  PixelTraits<PixelFormat_RGB24>::PixelType p;
  ImageTraits<PixelFormat_RGB24>::GetPixel(p, image, x, y);
  return (p.red_ == red &&
          p.green_ == green &&
          p.blue_ == blue);
}


TEST(ImageProcessing, FlipGrayscale8)
{
  {
    Image image(PixelFormat_Grayscale8, 0, 0, false);
    ImageProcessing::FlipX(image);
    ImageProcessing::FlipY(image);
  }

  {
    Image image(PixelFormat_Grayscale8, 1, 1, false);
    SetGrayscale8Pixel(image, 0, 0, 128);
    ImageProcessing::FlipX(image);
    ImageProcessing::FlipY(image);
    ASSERT_TRUE(TestGrayscale8Pixel(image, 0, 0, 128));
  }

  {
    Image image(PixelFormat_Grayscale8, 3, 2, false);
    SetGrayscale8Pixel(image, 0, 0, 10);
    SetGrayscale8Pixel(image, 1, 0, 20);
    SetGrayscale8Pixel(image, 2, 0, 30);
    SetGrayscale8Pixel(image, 0, 1, 40);
    SetGrayscale8Pixel(image, 1, 1, 50);
    SetGrayscale8Pixel(image, 2, 1, 60);

    ImageProcessing::FlipX(image);
    ASSERT_TRUE(TestGrayscale8Pixel(image, 0, 0, 30));
    ASSERT_TRUE(TestGrayscale8Pixel(image, 1, 0, 20));
    ASSERT_TRUE(TestGrayscale8Pixel(image, 2, 0, 10));
    ASSERT_TRUE(TestGrayscale8Pixel(image, 0, 1, 60));
    ASSERT_TRUE(TestGrayscale8Pixel(image, 1, 1, 50));
    ASSERT_TRUE(TestGrayscale8Pixel(image, 2, 1, 40));

    ImageProcessing::FlipY(image);
    ASSERT_TRUE(TestGrayscale8Pixel(image, 0, 0, 60));
    ASSERT_TRUE(TestGrayscale8Pixel(image, 1, 0, 50));
    ASSERT_TRUE(TestGrayscale8Pixel(image, 2, 0, 40));
    ASSERT_TRUE(TestGrayscale8Pixel(image, 0, 1, 30));
    ASSERT_TRUE(TestGrayscale8Pixel(image, 1, 1, 20));
    ASSERT_TRUE(TestGrayscale8Pixel(image, 2, 1, 10));
  }
}



TEST(ImageProcessing, FlipRGB24)
{
  Image image(PixelFormat_RGB24, 2, 2, false);
  SetRGB24Pixel(image, 0, 0, 10, 100, 110);
  SetRGB24Pixel(image, 1, 0, 20, 100, 110);
  SetRGB24Pixel(image, 0, 1, 30, 100, 110);
  SetRGB24Pixel(image, 1, 1, 40, 100, 110);

  ImageProcessing::FlipX(image);
  ASSERT_TRUE(TestRGB24Pixel(image, 0, 0, 20, 100, 110));
  ASSERT_TRUE(TestRGB24Pixel(image, 1, 0, 10, 100, 110));
  ASSERT_TRUE(TestRGB24Pixel(image, 0, 1, 40, 100, 110));
  ASSERT_TRUE(TestRGB24Pixel(image, 1, 1, 30, 100, 110));

  ImageProcessing::FlipY(image);
  ASSERT_TRUE(TestRGB24Pixel(image, 0, 0, 40, 100, 110));
  ASSERT_TRUE(TestRGB24Pixel(image, 1, 0, 30, 100, 110));
  ASSERT_TRUE(TestRGB24Pixel(image, 0, 1, 20, 100, 110));
  ASSERT_TRUE(TestRGB24Pixel(image, 1, 1, 10, 100, 110));
}


TEST(ImageProcessing, ResizeBasicGrayscale8)
{
  Image source(PixelFormat_Grayscale8, 2, 2, false);
  SetGrayscale8Pixel(source, 0, 0, 10);
  SetGrayscale8Pixel(source, 1, 0, 20);
  SetGrayscale8Pixel(source, 0, 1, 30);
  SetGrayscale8Pixel(source, 1, 1, 40);

  {
    Image target(PixelFormat_Grayscale8, 2, 4, false);
    ImageProcessing::Resize(target, source);
    ASSERT_TRUE(TestGrayscale8Pixel(target, 0, 0, 10));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 1, 0, 20));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 0, 1, 10));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 1, 1, 20));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 0, 2, 30));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 1, 2, 40));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 0, 3, 30));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 1, 3, 40));
  }

  {
    Image target(PixelFormat_Grayscale8, 4, 2, false);
    ImageProcessing::Resize(target, source);
    ASSERT_TRUE(TestGrayscale8Pixel(target, 0, 0, 10));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 1, 0, 10));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 2, 0, 20));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 3, 0, 20));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 0, 1, 30));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 1, 1, 30));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 2, 1, 40));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 3, 1, 40));
  }
}


TEST(ImageProcessing, ResizeBasicRGB24)
{
  Image source(PixelFormat_RGB24, 2, 2, false);
  SetRGB24Pixel(source, 0, 0, 10, 100, 110);
  SetRGB24Pixel(source, 1, 0, 20, 100, 110);
  SetRGB24Pixel(source, 0, 1, 30, 100, 110);
  SetRGB24Pixel(source, 1, 1, 40, 100, 110);

  {
    Image target(PixelFormat_RGB24, 2, 4, false);
    ImageProcessing::Resize(target, source);
    ASSERT_TRUE(TestRGB24Pixel(target, 0, 0, 10, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 1, 0, 20, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 0, 1, 10, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 1, 1, 20, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 0, 2, 30, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 1, 2, 40, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 0, 3, 30, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 1, 3, 40, 100, 110));
  }

  {
    Image target(PixelFormat_RGB24, 4, 2, false);
    ImageProcessing::Resize(target, source);
    ASSERT_TRUE(TestRGB24Pixel(target, 0, 0, 10, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 1, 0, 10, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 2, 0, 20, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 3, 0, 20, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 0, 1, 30, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 1, 1, 30, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 2, 1, 40, 100, 110));
    ASSERT_TRUE(TestRGB24Pixel(target, 3, 1, 40, 100, 110));
  }
}


TEST(ImageProcessing, ResizeEmptyGrayscale8)
{
  {
    Image source(PixelFormat_Grayscale8, 0, 0, false);
    Image target(PixelFormat_Grayscale8, 2, 2, false);
    ImageProcessing::Resize(target, source);
    ASSERT_TRUE(TestGrayscale8Pixel(target, 0, 0, 0));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 1, 0, 0));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 0, 1, 0));
    ASSERT_TRUE(TestGrayscale8Pixel(target, 1, 1, 0));
  }

  {
    Image source(PixelFormat_Grayscale8, 2, 2, false);
    Image target(PixelFormat_Grayscale8, 0, 0, false);
    ImageProcessing::Resize(target, source);
  }
}
