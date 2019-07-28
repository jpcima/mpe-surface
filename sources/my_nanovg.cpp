#include <QtGlobal>

#define GL_GLEXT_PROTOTYPES 1
#if defined(Q_OS_ANDROID)
# define NANOVG_GLES2_IMPLEMENTATION 1
# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>
#else
# define NANOVG_GL2_IMPLEMENTATION 1
# include <GL/gl.h>
# include <GL/glext.h>
#endif

#include <nanovg.h>
#include <nanovg_gl.h>
