#include <iostream>
#include <fstream>
#include <string>

int main() {
    const std::string filename = "testfile.txt";
    const size_t fileSize = 100 * 1024 * 1024; // 100 MB

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Failed to create file " << filename << std::endl;
        return 1;
    }

    const std::string dataPerLine = "This line demonstrates a large file transfer using sendfile. ";
    size_t bytesWritten = 0;

    while (bytesWritten < fileSize) {
        file.write(dataPerLine.c_str(), dataPerLine.size());
        if (!file) {
            std::cerr << "Error: Failed to write to file at " << bytesWritten << " bytes." << std::endl;
            return 1;
        }
        bytesWritten += dataPerLine.size();
    }

    file.flush();
    if (!file) {
        std::cerr << "Error: Flushing buffer failed." << std::endl;
        return 1;
    }

    file.close();
    std::cout << "Generated file: " << filename << " (" << bytesWritten << " bytes)" << std::endl;
    return 0;
}