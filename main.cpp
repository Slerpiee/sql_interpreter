#include "ui.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Starting Model SQL Interpreter...\n";
    
    UserInterface ui;
    ui.run();
    
    std::cout << "Goodbye!\n";
    return 0;
}
