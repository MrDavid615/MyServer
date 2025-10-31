#include "myServer.hpp"
using namespace std;

int main() {
    MyServer server(4);
    server.InitMyServer(8080, "");
    server.StartMyServer();
    return 0;
}
