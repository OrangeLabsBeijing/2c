/*-----------------------------------------------------------
 * 2c - Cross Platform 3D Application Framework
 *-----------------------------------------------------------
 * Copyright © 2010 – 2011 France Telecom
 * This software is distributed under the Apache 2.0 license,
 * see the "license.txt" file for more details.
 *-----------------------------------------------------------
 * File Name   : CCRenderer.cpp
 *-----------------------------------------------------------
 */

#include "CCDefines.h"


CCShader::CCShader(const char *name)
{
    this->name = name;
}


void CCShader::use()
{
#if defined PROFILEON
    CCProfiler profile( "CCShader::use()" );
#endif

#ifdef QT
    program->bind();
#else
	glUseProgram( program );
#endif
    
    DEBUG_OPENGL();
    
    CCRefreshRenderAttributes();
}


void CCShader::enableAttributeArray(const uint index)
{
#ifdef QT       
    program->enableAttributeArray( index );
#else           
    glEnableVertexAttribArray( index );
#endif
}


void CCShader::disableAttributeArray(const uint index)
{
#ifdef QT       
    program->disableAttributeArray( index );
#else           
    glDisableVertexAttribArray( index );
#endif
}



void CCRenderer::setup(const bool clear)
{
    if( !createContext() || !loadShaders() )
    {
        return;
    }
    
    frameBufferManager.setup();

    // Screen dimensions
    setupScreenSizeParams();

    clearScreenRequired = clear;
}


CCRenderer::~CCRenderer()
{
}


void CCRenderer::setupScreenSizeParams()
{
    refreshScreenSize();
    inverseScreenSize.width = 1.0f / screenSize.width;
    inverseScreenSize.height = 1.0f / screenSize.height;
    aspectRatio = screenSize.width / screenSize.height;
}


CCShader* CCRenderer::loadShader(const char *name)
{
    CCShader *shader = new CCShader( name );
    
    if( loadShader( shader ) )
    {
        currentShader = shader;
        // Get uniform locations
        shader->uniforms[UNIFORM_MODELVIEWPROJECTIONMATRIX] = getShaderUniformLocation( "u_modelViewProjectionMatrix" );
        shader->uniforms[UNIFORM_MODELVIEWMATRIX] = getShaderUniformLocation( "u_modelViewMatrix" );
        shader->uniforms[UNIFORM_MODELCOLOUR] = getShaderUniformLocation( "u_modelColour" );
        
        shader->uniforms[UNIFORM_MODELNORMALMATRIX] = getShaderUniformLocation( "u_modelNormalMatrix" );
        shader->uniforms[UNIFORM_LIGHTPOSITION] = getShaderUniformLocation( "u_lightPosition" );
        shader->uniforms[UNIFORM_LIGHTDIFFUSE] = getShaderUniformLocation( "u_lightDiffuse" );
        
        shader->uniforms[UNIFORM_CAMERAPOSITION] = getShaderUniformLocation( "u_cameraPosition" );
        
        shader->uniforms[TEXTURE_DIFFUSE] = getShaderUniformLocation( "s_diffuseTexture" );
        shader->uniforms[TEXTURE_ENV] = getShaderUniformLocation( "s_envTexture" );
        
        shaders.add( shader );
        return shader;
    }
    delete shader;
    ASSERT( false );
    return NULL;
}


const bool CCRenderer::loadShaders()
{
    loadShader( "basic" );
    loadShader( "basic_vc" );
    loadShader( "alphacolour" );
    loadShader( "phong" );
    loadShader( "phongenv" );
    return true;
}


void CCRenderer::clear()
{
    if( clearScreenRequired )
    {
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }
    else
    {
        glClear( GL_DEPTH_BUFFER_BIT );
    }
}


void CCRenderer::setupOpenGL()
{
    // Use shader program
    currentShader = shaders.list[0];
    currentShader->use();
    
    currentShader->enableAttributeArray( ATTRIB_VERTEX );
    currentShader->enableAttributeArray( ATTRIB_TEXCOORD );
	DEBUG_OPENGL();

    //glClearColor( 0.85f, 0.85f, 0.875f, 1.0f );
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    DEBUG_OPENGL();

    GLEnableDepth();
    glDepthMask( GL_TRUE );
    glDepthFunc( GL_LEQUAL );
    DEBUG_OPENGL();
	
    // GL_TEXTURE_2D is not a valid argument to glEnable in OpenGL ES 2.0
    //glEnable( GL_TEXTURE_2D );

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_BLEND );
    DEBUG_OPENGL();

#ifndef Q_OS_SYMBIAN
    glClearDepthf( 1.0f );
    DEBUG_OPENGL();
#endif

    glLineWidth( LINE_WIDTH );
    DEBUG_OPENGL();

    // Enable back face culling
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );
}


const bool CCRenderer::setShader(const char *name, const bool useVertexColours, const bool useVertexNormals)
{
#if defined PROFILEON
    CCProfiler profile( "CCRenderer::setShader()" );
#endif

    // Hash map optimzation?
    if( CCText::Equals( currentShader->name, name ) == false )
    {
        for( int i=0; i<shaders.length; ++i )
        {
            CCShader *shader = shaders.list[i];
            if( CCText::Equals( shader->name, name ) )
            {
                currentShader = shader;
                currentShader->use();
                
                static bool usingVertexColours = false, usingVertexNormals = false;
                if( useVertexColours )
                {
                    if( usingVertexColours == false )
                    {
                        currentShader->enableAttributeArray( ATTRIB_COLOUR );
                        usingVertexColours = true;
                    }
                }
                else if( usingVertexColours )
                {
                    currentShader->disableAttributeArray( ATTRIB_COLOUR );
                    usingVertexColours = false;
                }
                
                if( useVertexNormals )
                {
                    if( usingVertexNormals == false )
                    {
                        currentShader->enableAttributeArray( ATTRIB_NORMAL );
                        usingVertexNormals = true;
                    }
                }
                else if( usingVertexNormals )
                {
                    currentShader->disableAttributeArray( ATTRIB_NORMAL );
                    usingVertexNormals = false;
                }
                
                if( currentShader->uniforms[UNIFORM_CAMERAPOSITION] != -1 )
                {
                    const CCVector3 &position = CCCameraBase::currentCamera->getRotatedPosition();
                    CCSetUniformVector3( UNIFORM_CAMERAPOSITION, position.x, position.y, position.z );
                }
                
//                    if( currentShader->uniforms[TEXTURE_DIFFUSE] != -1 )
//                    {
//                        glUniform1i( currentShader->uniforms[TEXTURE_DIFFUSE], 0 );
//                    }
                
#ifndef QT
                if( currentShader->uniforms[TEXTURE_ENV] != -1 )
                {
                    glUniform1i( currentShader->uniforms[TEXTURE_ENV], 1 );
                }
#endif
                
                //currentShader->enableAttributeArray( ATTRIB_COLOUR );
                //currentShader->enableAttributeArray( ATTRIB_NORMAL );
                
                return true;
            }
        }
        
        ASSERT( false );
        return false;
    }
    
    return false;
}



static bool blendEnabled = false;
void GLEnableBlend()
{
    if( blendEnabled == false )
    {
        blendEnabled = true;
        glEnable( GL_BLEND );
    }
}


void GLDisableBlend()
{
    if( blendEnabled )
    {
        blendEnabled = false;
        glDisable( GL_BLEND );
    }
}


const bool GLBlendState()
{
    return blendEnabled;
}


static bool depthEnabled = false;
void GLEnableDepth()
{
    if( depthEnabled == false )
    {
        depthEnabled = true;
        glEnable( GL_DEPTH_TEST );   
    }
}


void GLDisableDepth()
{
    if( depthEnabled )
    {
        depthEnabled = false;
        glDisable( GL_DEPTH_TEST );
    }
}


const bool GLDepthState()
{
    return depthEnabled;
}


// Attempt to simulate OpenGL 1.1 interface to shaders
void GLVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    CCSetVertexAttribute( ATTRIB_VERTEX, size, type, stride, pointer, false );
}


void GLTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    CCSetVertexAttribute( ATTRIB_TEXCOORD, size, type, stride, pointer, false );
}


void GLColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    CCSetUniformVector4( UNIFORM_MODELCOLOUR, r, g, b, a );
    
//    What if we wanted to modify the vertex colours?
//    const int verts = 1024;
//    static GLfloat colours[verts*4];
//    for( int i=0; i<verts; ++i )
//    {
//        colours[i*4+0] = r;
//        colours[i*4+1] = g;
//        colours[i*4+2] = b;
//        colours[i*4+3] = a;
//    }
//    GLVertexAttribPointer( ATTRIB_COLOUR, 4, GL_FLOAT, true, 0, colours );
}

void CCSetVertexAttribute(const uint attribute, 
                          GLint size, GLenum type, GLsizei stride, 
                          const GLvoid *pointer, const bool normalized)
{
    GLVertexAttribPointer( attribute, size, type, normalized, stride, pointer );
}

void CCSetUniformVector3(const uint uniform, 
                         const float x, const float y, const float z)
{
    const GLint uniformLocation = gEngine->renderer->getShader()->uniforms[uniform];
    if( uniformLocation != -1 )
    {
        static GLfloat floats[3];
        floats[0] = x;
        floats[1] = y;
        floats[2] = z;
        
        GLUniform3fv( uniformLocation, 1, floats );
    }
}


void CCSetUniformVector4(const uint uniform, 
                         const float x, const float y, const float z, const float w)
{
    const GLint uniformLocation = gEngine->renderer->getShader()->uniforms[uniform];
    if( uniformLocation != -1 )
    {
        static GLfloat floats[4];
        floats[0] = x;
        floats[1] = y;
        floats[2] = z;
        floats[3] = w;
        
        GLUniform4fv( uniformLocation, 1, floats );
    }
}
