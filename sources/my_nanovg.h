#pragma once
#include <QtGlobal>
#include <nanovg.h>

#if defined(Q_OS_ANDROID)

#define NANOVG_GLES2 1
#include <nanovg_gl.h>

static constexpr auto &nvgCreateGL = nvgCreateGLES2;
static constexpr auto &nvgDeleteGL = nvgDeleteGLES2;
static constexpr auto &nvglCreateImageFromHandleGL = nvglCreateImageFromHandleGLES2;
static constexpr auto &nvglImageHandleGL = nvglImageHandleGLES2;

#else

#define NANOVG_GL2 1
#include <nanovg_gl.h>

static constexpr auto &nvgCreateGL = nvgCreateGL2;
static constexpr auto &nvgDeleteGL = nvgDeleteGL2;
static constexpr auto &nvglCreateImageFromHandleGL = nvglCreateImageFromHandleGL2;
static constexpr auto &nvglImageHandleGL = nvglImageHandleGL2;

#endif
