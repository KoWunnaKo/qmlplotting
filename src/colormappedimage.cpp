#include "colormappedimage.h"

#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>
#include <QSGMaterialShader>
#include <QSGTexture>
#include <QOpenGLContext>
#include <QStringList>
#include "qsgfloattexture.h"

// ----------------------------------------------------------------------------

static GLfloat cmap_wjet[] = {
    1.000000, 1.000000, 1.000000,
    0.200000, 0.300000, 1.000000,
    0.000000, 0.500000, 1.000000,
    0.000000, 1.000000, 1.000000,
    0.500000, 1.000000, 0.000000,
    1.000000, 1.000000, 0.000000,
    1.000000, 0.300000, 0.000000,
    1.000000, 0.000000, 0.000000
};

static GLfloat cmap_jet[] = {
    0.000000, 0.000000, 0.500000,
    0.000000, 0.000000, 1.000000,
    0.000000, 1.000000, 1.000000,
    0.500000, 1.000000, 0.300000,
    1.000000, 1.000000, 0.000000,
    1.000000, 0.000000, 0.000000
};

static GLfloat cmap_hot[] = {
    0.000000, 0.000000, 0.000000,
    0.800000, 0.000000, 0.000000,
    1.000000, 0.900000, 0.000000,
    1.000000, 1.000000, 1.000000
};

static GLfloat cmap_bwr[] = {
    0.229806, 0.298718, 0.753683,
    0.266234, 0.353095, 0.801467,
    0.303869, 0.406535, 0.844959,
    0.342804, 0.458758, 0.883726,
    0.383013, 0.509419, 0.917388,
    0.424370, 0.558148, 0.945620,
    0.466667, 0.604563, 0.968155,
    0.509635, 0.648281, 0.984788,
    0.552953, 0.688929, 0.995376,
    0.596262, 0.726149, 0.999836,
    0.639176, 0.759600, 0.998151,
    0.681291, 0.788965, 0.990363,
    0.722193, 0.813953, 0.976575,
    0.761465, 0.834303, 0.956945,
    0.798692, 0.849786, 0.931689,
    0.833467, 0.860208, 0.901069,
    0.865395, 0.865410, 0.865396,
    0.897787, 0.848937, 0.820881,
    0.924128, 0.827385, 0.774508,
    0.944469, 0.800927, 0.726736,
    0.958853, 0.769768, 0.678008,
    0.967328, 0.734133, 0.628752,
    0.969954, 0.694267, 0.579375,
    0.966811, 0.650421, 0.530264,
    0.958003, 0.602842, 0.481776,
    0.943661, 0.551751, 0.434244,
    0.923945, 0.497309, 0.387970,
    0.899046, 0.439559, 0.343230,
    0.869187, 0.378313, 0.300267,
    0.834621, 0.312874, 0.259301,
    0.795632, 0.241284, 0.220526,
    0.752535, 0.157246, 0.184115,
    0.705673, 0.015556, 0.150233
};

static GLfloat cmap_gray[] = {
    0., 0., 0.,
    1., 1., 1.,
};

// ----------------------------------------------------------------------------

class QSQColormapMaterial : public QSGMaterial
{
public:
    QSGMaterialType *type() const { static QSGMaterialType type; return &type; }
    QSGMaterialShader *createShader() const;
    QSGTexture* m_texture_image;
    QSGTexture* m_texture_cmap;
    double m_amplitude;
    double m_offset;
};

class QSQColormapShader : public QSGMaterialShader
{
public:
    const char *vertexShader() const {
        return
        "attribute highp vec4 vertex;          \n"
        "attribute highp vec2 texcoord;        \n"
        "uniform highp mat4 matrix;            \n"
        "varying highp vec2 coord;             \n"
        "                                      \n"
        "void main() {                         \n"
        "    coord = texcoord;                 \n"
        "    gl_Position = matrix * vertex;    \n"
        "}";
    }

    const char *fragmentShader() const {
        return
        "uniform sampler2D image;                                \n"
        "uniform sampler2D cmap;                                 \n"
        "uniform highp float amplitude;                          \n"
        "uniform highp float offset;                             \n"
        "varying highp vec2 coord;                               \n"
        "uniform lowp float opacity;                             \n"
        "void main() {                                           \n"
        "    highp float val = texture2D(image, coord.st).r;     \n"
        "    val = amplitude * (val + offset);                   \n"
        "    vec4 color = texture2D(cmap, vec2(val, 1.));        \n"
        "    gl_FragColor = color * opacity;                     \n"
        "}";
    }

    char const *const *attributeNames() const
    {
        static char const *const names[] = { "vertex", "texcoord", 0 };
        return names;
    }

    void initialize()
    {
        QSGMaterialShader::initialize();
        m_id_matrix = program()->uniformLocation("matrix");
        m_id_opacity = program()->uniformLocation("opacity");
        m_id_image = program()->uniformLocation("image");
        m_id_cmap = program()->uniformLocation("cmap");
        m_id_amplitude = program()->uniformLocation("amplitude");
        m_id_offset = program()->uniformLocation("offset");
    }

    void activate() {
    }

    void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *)
    {
        Q_ASSERT(program()->isLinked());
        QSQColormapMaterial* material = static_cast<QSQColormapMaterial*>(newMaterial);

        if (state.isMatrixDirty())
            program()->setUniformValue(m_id_matrix, state.combinedMatrix());
        if (state.isOpacityDirty())
            program()->setUniformValue(m_id_opacity, state.opacity());

        // bind material parameters
        program()->setUniformValue(m_id_amplitude, float(material->m_amplitude));
        program()->setUniformValue(m_id_offset, float(material->m_offset));

        // bind the material textures (image and colormap)
        glActiveTexture(GL_TEXTURE1);
        program()->setUniformValue(m_id_cmap, 1);
        material->m_texture_cmap->bind();
        glActiveTexture(GL_TEXTURE0);
        program()->setUniformValue(m_id_image, 0);
        material->m_texture_image->bind();
    }

    void deactivate() {
        // unbind all textures
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

private:
    int m_id_matrix;
    int m_id_opacity;
    int m_id_image;
    int m_id_cmap;
    int m_id_amplitude;
    int m_id_offset;
};


inline QSGMaterialShader* QSQColormapMaterial::createShader() const { return new QSQColormapShader; }

// ----------------------------------------------------------------------------


ColormappedImage::ColormappedImage(QQuickItem *parent) :
    DataClient(parent),
    m_min_value(0.), m_max_value(1.),
    m_new_colormap(false), m_texture_cmap(nullptr)
{
    setFlag(QQuickItem::ItemHasContents);
}

ColormappedImage::~ColormappedImage()
{
    if (m_texture_cmap) {
        m_texture_cmap->deleteLater();
    }
}

void ColormappedImage::setMinimumValue(double value)
{
    if (value == m_min_value) return;
    m_min_value = value;
    emit minimumValueChanged(value);
    update();
}

void ColormappedImage::setMaximumValue(double value)
{
    if (value == m_max_value) return;
    m_max_value = value;
    emit maximumValueChanged(value);
    update();
}

void ColormappedImage::setColormap(const QString &colormap)
{
    if (colormap == m_colormap) return;
    m_colormap = colormap;
    m_new_colormap = true;
    emit colormapChanged(m_colormap);
    update();
}

static void updateColormapTexture(QSGFloatTexture* t, const QString& colormap) {
    // default colormap
    GLfloat* data = cmap_gray;
    int numpoints = sizeof(cmap_gray) / (3*sizeof(GLfloat));

    // check colormap strings, TODO: build a map for lookup?
    if (colormap == "wjet") {
        data = cmap_wjet;
        numpoints = sizeof(cmap_wjet) / (3*sizeof(GLfloat));
    } else if (colormap == "jet") {
        data = cmap_jet;
        numpoints = sizeof(cmap_jet) / (3*sizeof(GLfloat));
    } else if (colormap == "hot") {
        data = cmap_hot;
        numpoints = sizeof(cmap_hot) / (3*sizeof(GLfloat));
    } else if (colormap == "bwr") {
        data = cmap_bwr;
        numpoints = sizeof(cmap_bwr) / (3*sizeof(GLfloat));
    }

    t->setDataSource(data, numpoints, 1, 3);
    t->updateTexture();
}

QSGNode *ColormappedImage::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *)
{
    QSGGeometryNode* n = static_cast<QSGGeometryNode*>(node);
    QSGGeometry *geometry;
    QSQColormapMaterial *material;

    if (n && !m_datacontainer) {
        // remove the node if there is no data source
        delete n;
        return nullptr;
    }

    if  (!n && m_datacontainer) {
        // create a node if there is a data source
        n = new QSGGeometryNode();
        // inintialize geometry
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);
        geometry->setDrawingMode(GL_TRIANGLE_STRIP);
        n->setGeometry(geometry);
        n->setFlag(QSGNode::OwnsGeometry);
        m_new_geometry = true;
        // initialize colormap
        if (!m_texture_cmap) {
            m_texture_cmap = new QSGFloatTexture();
            m_new_colormap = true;
        }
        // initialize material
        material = new QSQColormapMaterial;
        material->m_texture_cmap = m_texture_cmap;
        material->m_texture_image = m_datacontainer->textureProvider()->texture();
        n->setMaterial(material);
        n->setFlag(QSGNode::OwnsMaterial);
    }

    if (!n) {
        // return null if there is nothing to be drawn
        return nullptr;
    }

    // ** graph node and data source can be considered valid from here on **

    geometry = n->geometry();
    material = static_cast<QSQColormapMaterial*>(n->material());
    QSGNode::DirtyState dirty_state = QSGNode::DirtyMaterial;

    // update geometry in case the width and height changed
    if (m_new_geometry) {
        double w = width();
        double h = height();
        geometry->vertexDataAsTexturedPoint2D()[0].set(0, 0, 0, 0);
        geometry->vertexDataAsTexturedPoint2D()[1].set(0, h, 0, 1);
        geometry->vertexDataAsTexturedPoint2D()[2].set(w, 0, 1, 0);
        geometry->vertexDataAsTexturedPoint2D()[3].set(w, h, 1, 1);
        dirty_state |= QSGNode::DirtyGeometry;
        m_new_geometry = false;
    }

    // update material parameters
    material->m_amplitude = 1. / (m_max_value - m_min_value);
    material->m_offset = -m_min_value;

    // check for data source change
    if (m_new_container) {
        material->m_texture_image = m_datacontainer->textureProvider()->texture();
        m_new_container = false;
    }

    // check if the image texture should be updated
    if (m_new_data) {
        static_cast<QSGDynamicTexture*>(material->m_texture_image)->updateTexture();
        m_new_data = false;
    }

    // check if the cmap texture should be updated
    if (m_new_colormap) {
        QSGFloatTexture* t = static_cast<QSGFloatTexture*>(material->m_texture_cmap);
        updateColormapTexture(t, m_colormap);
        m_new_colormap = false;
    }

    n->markDirty(dirty_state);
    return n;
}