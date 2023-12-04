#include <iostream>
#include <chrono>
#include <string>
#include "QhciBase.hpp"

int main(int argc, char *argv[]) {
    if(argc != 2) {
        QhciTest::GetInstance().PringUsage();
        return -1;
    }
    std::string command = argv[1];
    if (command == "all") {
        QhciTest::GetInstance().QhciTestAll();
    } else if (QhciTest::GetInstance().current_map_func()[command] != NULL) {
        QhciTest::GetInstance().QhciTestSingle(command);
    } else {
        std::cout << "unknown command, exit" << std::endl;
        QhciTest::GetInstance().PringUsage();
    }
    return 0;
}
