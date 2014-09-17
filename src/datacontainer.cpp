#include <QThread>
#include <QOpenGLContext>
#include "datacontainer.h"
#include "qsgfloattexture.h"

#include <math.h>

class DataTexture : public QSGFloatTexture
{
public:
    DataTexture(DataContainer* datacontainer) : QSGFloatTexture(), m_datacontainer(datacontainer) {}
    virtual ~DataTexture() {}

    virtual bool updateTexture() {
        if (m_datacontainer->m_new_data) {
            setDataSource(m_datacontainer->m_data,
                          m_datacontainer->m_width,
                          m_datacontainer->m_height,
                          1);
            return QSGFloatTexture::updateTexture();
        } else  {
            return false;
        }
    }
    DataContainer* m_datacontainer;
};


DataContainer::DataContainer(QQuickItem *parent) :
    QQuickItem(parent), m_data(nullptr), m_width(0), m_height(0), m_new_data(false),
    m_provider(nullptr), m_test_data_buffer()
{
}

DataContainer::~DataContainer()
{
    if (m_provider) {
        m_provider->deleteLater();
    }
}

bool DataContainer::isTextureProvider() const
{
    return true;
}

bool DataContainer::setDataFloat32(void *data, int width, int height)
{
    m_data = (float*) (data);
    m_width = width;
    m_height = height;
    m_new_data = true;
    emit dataChanged();
    return true;
}

bool DataContainer::setTestData()
{
    int w = 512, h = 512;
    int num_bytes = w * h * sizeof(float);
    if (m_test_data_buffer.size() != num_bytes) {
        m_test_data_buffer.resize(num_bytes);
    }
    float* d = reinterpret_cast<float*>(m_test_data_buffer.data());

    // gauss + noise
    for (int iy = 0; iy < w; ++iy) {
        for (int ix = 0; ix < h; ++ix) {
            double x = -1. + 2.*(ix * (1./w));
            double y = -1. + 2.*(iy * (1./h));
            double r = rand() * (1./RAND_MAX);
            d[iy*w + ix] = exp(-(x*x+y*y)*2.) + .2 * (r-.5);
        }
    }
    // linear strip
    for (int iy = h-30; iy < h; ++iy) {
        for (int ix = 0; ix < w; ++ix) {
            double x = (ix * (1./w));
            d[iy*w + ix] = x;
        }
    }

    setDataFloat32(d, w, h);
    return true;
}

QSGTextureProvider *DataContainer::textureProvider()
{
    if (!QOpenGLContext::currentContext()) {
        qWarning("DataContainer::textureProvider needs OpenGL context");
        return 0;
    }
    if (!m_provider) {
        m_provider = new DataTextureProvider(this);
        m_provider->m_datatexture->updateTexture();
    }
    return m_provider;
}

DataTextureProvider::DataTextureProvider(DataContainer *datacontainer) :
    QSGTextureProvider()
{
    m_datatexture = new DataTexture(datacontainer);
}

DataTextureProvider::~DataTextureProvider()
{
    delete m_datatexture;
}

QSGTexture *DataTextureProvider::texture() const
{
    return m_datatexture;
}