#include "CalenhadMapWidget.h"
#include <QIcon>
#include "../icosphere/Bounds.h"
#include "../graph/graph.h"
#include "../CalenhadServices.h"
#include "../preferences/preferences.h"
#include "projection/ProjectionService.h"
#include "projection/Projection.h"
#include <QWindow>
#include <QtXml/QtXml>
#include <QtGui/QPainter>

using namespace calenhad;
using namespace geoutils;
using namespace matrices;
using namespace icosphere;
using namespace calenhad::graph;
using namespace calenhad::mapping;
using namespace calenhad::mapping::projection;

CalenhadMapWidget::CalenhadMapWidget (QWidget* parent) : QOpenGLWidget (parent),
    m_vertexBuffer (nullptr),
    m_computeShader (nullptr),
    m_computeProgram (nullptr),
    m_vertexShader (nullptr),
    m_fragmentShader (nullptr),
    m_texture (nullptr),
    m_renderProgram (nullptr),
    m_indexBuffer (nullptr),
    _parentMap (nullptr),
    _altitudeMapBuffer (nullptr),
    _projection (CalenhadServices::projections() -> fetch ("Equirectangular")),
    _scale (1.0),
    _shader (""),
    _graticule (true),
    _inset (false) {


    QSurfaceFormat format;
    format.setSamples(8);
    format.setVersion(4, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
    setContextMenuPolicy(Qt::CustomContextMenu);

}

CalenhadMapWidget::~CalenhadMapWidget() {
    makeCurrent();
    if (m_computeShader) { delete m_computeShader; }
    if (m_vertexShader) { delete m_vertexShader; }
    if (m_fragmentShader) { delete m_fragmentShader; }
    if (m_texture) { delete m_texture; }
    if (m_renderProgram) { delete m_renderProgram; }
    if (m_computeProgram) { delete m_computeProgram; }
    if (m_indexBuffer)  { delete m_indexBuffer; }
    if (m_vertexBuffer) { delete m_vertexBuffer; }
    if (_altitudeMapBuffer) { delete _altitudeMapBuffer; }
}

void CalenhadMapWidget::initializeGL() {
    initializeOpenGLFunctions();
    //glEnable (GL_MULTISAMPLE);

    glClearColor(0, 0, 1, 1);
    _colorMapBuffer = _graph -> colorMapBuffer();
    _altitudeMapBuffer = _graph -> altitudeMapBuffer();

    // create and allocate any required shared altitudeMapBuffer on the GPU and copy the contents across to them.

    GLuint altitudeMap = 0;
    glGenBuffers (1, &altitudeMap);
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, altitudeMap);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) * _graph -> altitudeMapBufferSize(), _altitudeMapBuffer, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, altitudeMap);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

    // create and allocate the colorMapBuffer on the GPU and copy the contents across to them.

    GLuint colorMap = 1;
    glGenBuffers (1, &colorMap);
    glBindBuffer (GL_SHADER_STORAGE_BUFFER, colorMap);
    glBufferData (GL_SHADER_STORAGE_BUFFER, sizeof (float) *  _graph -> colorMapBufferSize(), _colorMapBuffer, GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, colorMap);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 1); // unbind

    m_vao.create();
    if (m_vao.isCreated()){
        m_vao.bind();
        qDebug() << "VAO created!";
    }

    static const GLfloat g_vertex_buffer_data[] = {
        -1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f
    };
    static const GLushort g_element_buffer_data[] = { 0, 1, 2, 3 };

    m_vertexBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vertexBuffer->create();
    m_vertexBuffer->setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_vertexBuffer->bind();
    m_vertexBuffer->allocate(g_vertex_buffer_data,sizeof(g_vertex_buffer_data));

    m_indexBuffer = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    m_indexBuffer->create();
    m_indexBuffer->setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_indexBuffer->bind();
    m_indexBuffer->allocate(g_element_buffer_data,sizeof(g_element_buffer_data));

    glActiveTexture(GL_TEXTURE0);
    m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_texture->create();
    m_texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
    m_texture->setSize (2048, 1024);
    m_texture->setMinificationFilter(QOpenGLTexture::Linear);
    m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_texture->allocateStorage();
    m_texture->bind();

    glBindImageTexture(0, m_texture->textureId(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    qDebug() << m_texture->width() << m_texture->height();
    m_computeShader = new QOpenGLShader(QOpenGLShader::Compute);
    //m_computeShader -> compileSourceFile (":/shaders/map_cs.glsl");
    m_vertexShader = new QOpenGLShader(QOpenGLShader::Vertex);
    m_vertexShader->compileSourceFile(":/shaders/map_vs.glsl");
    m_fragmentShader = new QOpenGLShader(QOpenGLShader::Fragment);
    m_fragmentShader -> compileSourceFile (":/shaders/map_fs.glsl");
    m_computeProgram = new QOpenGLShaderProgram();

    if (! m_computeShader) {
        m_computeShader = new QOpenGLShader(QOpenGLShader::Compute);
    }

    m_computeShader -> compileSourceCode (_shader);
    m_computeProgram -> removeAllShaders();
    m_computeProgram -> addShader (m_computeShader);
    m_computeProgram -> link();
    m_computeProgram->bind();

    m_renderProgram = new QOpenGLShaderProgram();
    m_renderProgram->addShader(m_vertexShader);
    m_renderProgram->addShader(m_fragmentShader);
    m_renderProgram->link();
    m_renderProgram->bind();

    GLint posPtr = glGetAttribLocation(m_renderProgram->programId(), "pos");
    glVertexAttribPointer (posPtr, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray (posPtr);

    m_vao.release();
}

void CalenhadMapWidget::paintGL() {
    if (_parentMap) { _parentMap -> update(); }
    QPainter p (this);
    p.beginNativePainting();

    glUseProgram (m_computeProgram -> programId());
    static GLint srcLoc= glGetUniformLocation(m_renderProgram->programId(),"srcTex");
    static GLint destLoc=glGetUniformLocation(m_computeProgram->programId(),"destTex");
    static GLint ambsLoc = glGetUniformLocation (m_computeProgram -> programId(), "altitudeMapBufferSize");
    static GLint cmbsLoc = glGetUniformLocation (m_computeProgram -> programId(), "colorMapBufferSize");
    static GLint resolutionLoc = glGetUniformLocation (m_computeProgram -> programId(), "resolution");
    static GLint projectionLoc = glGetUniformLocation (m_computeProgram -> programId(), "projection");
    //static GLint modelMatrixLoc = glGetUniformLocation (m_computeProgram -> programId(), "modelMatrix");
    static GLint scaleLoc = glGetUniformLocation (m_computeProgram -> programId (), "scale");
    static GLint datumLoc = glGetUniformLocation (m_computeProgram -> programId(), "datum");
    static GLint insetHeightLoc = glGetUniformLocation (m_computeProgram -> programId(), "insetHeight");

    m_vao.bind();
    m_computeProgram->bind();
    m_texture->bind();
    GLubyte c = 0;
    std::vector<GLubyte> emptyData (m_texture -> width() * m_texture -> height() * 4, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_texture -> width(), m_texture -> height(), GL_BGRA, GL_UNSIGNED_BYTE, &emptyData[0]);
    // prepare the model matrix for the shader
    /*
    Mat4 M = modelMatrix();
    GLfloat a [16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            a [i * 4 + j] = M.value (i, j);
        }
    }
    */
    glUniform1i (destLoc, 0);
    glUniform2f (datumLoc, (GLfloat) _rotation.longitude(), (GLfloat) -_rotation.latitude());
    glUniform1i (projectionLoc, _projection -> id ());
    glUniform1f (scaleLoc, (GLfloat) _scale);
    glUniform1i (insetHeightLoc, _inset ? 192 : 0);
    glUniform1i (resolutionLoc, m_texture -> height());
    //glUniformMatrix4fv (modelMatrixLoc, 1, GL_TRUE, a);

    glUniform1i (ambsLoc, 2048);
    glUniform1i (cmbsLoc, 2048);


    glDispatchCompute (m_texture -> width() / 16, m_texture -> height() / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // draw
    m_renderProgram->bind();
    //glClear(GL_COLOR_BUFFER_BIT);
    m_texture->bind();
    glUniform1i(srcLoc, 0);
    glDrawElements (GL_TRIANGLE_STRIP,4,GL_UNSIGNED_SHORT,0);
    m_vao.release();

    p.endNativePainting();

    if (_graticule) {
        drawGraticule (p);
    }
}

void CalenhadMapWidget::resizeGL(int width, int height) {
    glViewport(0, 0, width, height);
}

// Insert the given code into the compute shader to realise the noise pipeline
void CalenhadMapWidget::setGraph (Graph* g) {
    if (g != _graph) {
        // read template compute shader file
        QFile file (":/shaders/map_cs.glsl");
        file.open (QIODevice::ReadOnly);
        QTextStream textStream (&file);
        _shader = textStream.readAll ();
        QString code = g->glsl ();
        _shader.replace ("// inserted code //", code);
        _shader.replace ("// inserted projections //", CalenhadServices::projections() -> glsl());
        _graph = g;
        std::cout << _shader.toStdString () << "\n";
        if (m_computeShader) {
            m_computeShader->compileSourceCode (_shader);
            m_computeProgram->removeAllShaders ();
            m_computeProgram->addShader (m_computeShader);
            m_computeProgram->link ();
            m_computeProgram->bind ();
        }
    }
}

void CalenhadMapWidget::showEvent (QShowEvent* e) {
    update();
}


void CalenhadMapWidget::setScale (const double& scale) {
    _scale = scale;
}

double CalenhadMapWidget::scale () {
    return _scale;
}

void CalenhadMapWidget::setProjection (const QString& projection) {
    _projection = CalenhadServices::projections () -> fetch (projection);
    update();
}

Projection* CalenhadMapWidget::projection() {
    return _projection;
}

void CalenhadMapWidget::rotate (const Geolocation& rotation) {
    _rotation = rotation;
    update();
    std::cout << "Rotation " << qRadiansToDegrees (_rotation.longitude()) << " " << qRadiansToDegrees (_rotation.latitude()) << "\n";
}

Geolocation CalenhadMapWidget::rotation() {
    return _rotation;
}

QPointF CalenhadMapWidget::translation() {
    return _translation;
}

void CalenhadMapWidget::setTranslation (const QPointF& translation) {
    _translation = translation;
}

matrices::Mat4 CalenhadMapWidget::modelMatrix () {
    //Mat4 T = Mat4::translationMatrix (_translation.x(), _translation.y(), 0.0);
    //Mat4 S = Mat4::scalingMatrix (_scale, _scale, _scale);

    //Versor vLat = Versor::fromParameters (_rotation.latitude(), 0.0, 1.0, 0.0);
    //Versor vLon = Versor::fromParameters (_rotation.longitude(), 1.0, 0.0, 0.0);
    //Versor vTilt = Versor::fromParameters (qDegreesToRadians (23.5), 0.0, 0.0, 0.1);
    //Versor v = vLat * vLon;
    //Mat4 R = v.toRotationMatrix();

    //return T * R ;//* S;

}

void CalenhadMapWidget::setParentMap (CalenhadMapWidget* parentMap) {
    _parentMap = parentMap;
}

Bounds CalenhadMapWidget::bounds() {

}

void CalenhadMapWidget::drawGraticule (QPainter& p) {
    p.setPen(Qt::red);

}

void CalenhadMapWidget::setInset (bool inset) {
    _inset = inset;
}