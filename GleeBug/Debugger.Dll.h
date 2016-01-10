#ifndef DEBUGGER_DLL_H
#define DEBUGGER_DLL_H

#include "Debugger.Global.h"

namespace GleeBug
{
    /**
    \brief DLL information structure.
    */
    class Dll
    {
    public:
        ptr lpBaseOfDll;
        ptr sizeOfImage;
        ptr entryPoint;

        /**
        \brief Constructor.
        \param lpBaseOfDll The base of DLL.
        \param sizeOfImage Size of the image.
        \param entryPoint The entry point.
        */
        explicit Dll(LPVOID lpBaseOfDll, ptr sizeOfImage, LPVOID entryPoint);
    };
};

#endif //DEBUGGER_DLL_H