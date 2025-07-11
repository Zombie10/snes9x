/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "glx_context.hpp"

GTKGLXContext::GTKGLXContext()
{
    display = nullptr;
    context = nullptr;

    version_major = -1;
    version_minor = -1;

    gladLoaderLoadGLX(nullptr, 0);
}

GTKGLXContext::~GTKGLXContext()
{
    if (context)
        glXDestroyContext(display, context);
}

bool GTKGLXContext::attach(Display *dpy, Window xid)
{
    this->xid = xid;
    display = dpy;

    XWindowAttributes wa{};
    XGetWindowAttributes(display, xid, &wa);
    screen = XScreenNumberOfScreen(wa.screen);

    glXQueryVersion(display, &version_major, &version_minor);
    if (version_major < 2 && version_minor < 3)
        return false;

    int attribs[] = {
        GLX_DOUBLEBUFFER, True,
        GLX_X_RENDERABLE, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        None
    };
    int num_fbconfigs;
    GLXFBConfig *fbconfigs = glXChooseFBConfig(display, screen, attribs, &num_fbconfigs);

    if (!fbconfigs || num_fbconfigs < 1)
    {
        printf("Couldn't match a GLX framebuffer config.\n");
        return false;
    }

    fbconfig = fbconfigs[0];
    free(fbconfigs);

    return true;
}

bool GTKGLXContext::create_context()
{
    int context_attribs[] = {
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        None
    };

    const char *extensions = glXQueryExtensionsString(display, screen);

    XSetErrorHandler([](Display *dpy, XErrorEvent *event) -> int {
        printf("XError: type: %d code: %d\n", event->type, event->error_code);
        return X_OK;
    });
    if (strstr(extensions, "GLX_ARB_create_context"))
        context = glXCreateContextAttribsARB(display, fbconfig, nullptr, True, context_attribs);
    if (!context)
        context = glXCreateNewContext(display, fbconfig, GLX_RGBA_TYPE, nullptr, True);
    XSetErrorHandler(nullptr);

    if (!context)
    {
        printf("Couldn't create GLX context.\n");
        return false;
    }

    resize();

    return true;
}

void GTKGLXContext::resize()
{
    while (!ready())
        usleep(100);

    unsigned int width;
    unsigned int height;
    glXQueryDrawable(display, xid, GLX_WIDTH, &width);
    glXQueryDrawable(display, xid, GLX_HEIGHT, &height);

    this->width = width;
    this->height = height;
    make_current();
}

void GTKGLXContext::swap_buffers()
{
    glXSwapBuffers(display, xid);
}

bool GTKGLXContext::ready()
{
    return true;
}

void GTKGLXContext::make_current()
{
    glXMakeCurrent(display, xid, context);
}

void GTKGLXContext::swap_interval(int frames)
{
    if (GLAD_GLX_EXT_swap_control)
        glXSwapIntervalEXT(display, xid, frames);
    else if (GLAD_GLX_SGI_swap_control)
        glXSwapIntervalSGI(frames);
#ifdef GLX_MESA_swap_control
    else if (GLAD_GLX_MESA_swap_control)
        glXSwapIntervalMESA(frames);
#endif
}
