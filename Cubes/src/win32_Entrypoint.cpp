#ifdef _WIN32

#include <windows.h>
int main();

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowCode)
{
    main();
}

#endif