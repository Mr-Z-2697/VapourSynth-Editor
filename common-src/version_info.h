#ifndef VERSION_INFO_H_INCLUDED
#define VERSION_INFO_H_INCLUDED

#define VSE_VERSION_STR "R19-mod-6.8"

#define VS_USE_LATEST_API
#define VSE_VS_API_VER_MAJOR 4
#define VSE_VS_API_VER_MINOR 1

#define VSSCRIPT_USE_LATEST_API
#define VSE_VSS_API_VER_MAJOR 4
#define VSE_VSS_API_VER_MINOR 2

void print_version();

#if defined(Q_OS_WIN)
void hide_tty();
#endif

#endif
