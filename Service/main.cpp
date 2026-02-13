#include "Service.h"

int main(int argc, TCHAR* argv[]) {
    RemoteDesktopService::Instance().Run();
    return 0;
}
