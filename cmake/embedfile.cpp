#include <cstdint>
#include <iostream>
#include <fstream>

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        std::cout << "Usage: " << argv[0] << " <source> <destination C file> <variable name>" << std::endl;
        return EXIT_FAILURE;
    }

    const char *source = argv[1];
    const char *destination = argv[2];
    const char *variableName = argv[3];

    std::ifstream sourceFile{source, std::ios::binary};
    if (!sourceFile.is_open())
    {
        std::cerr << "Failed to open source file for read: " << source << std::endl;
        return EXIT_FAILURE;
    }

    std::ofstream destinationFile{destination, std::ios::out | std::ios::trunc | std::ios::binary};
    if (!destinationFile.is_open())
    {
        sourceFile.close();
        std::cerr << "Failed to open destination file for write: " << destination << std::endl;
        return EXIT_FAILURE;
    }

    destinationFile << "#include <stdlib.h>\n\n";
    destinationFile << "const char " << variableName << "[] = {\n";
    destinationFile << std::hex;

    size_t bytesWritten = 0;
    do
    {
        char buffer[256];
        sourceFile.read(buffer, sizeof(buffer));

        for (int i = 0; i < sourceFile.gcount(); i++)
        {
            char outChars[8] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
            sprintf(outChars, "0x%02x, ", buffer[i]);
            destinationFile << outChars;
            if ((++bytesWritten % 8) == 0)
            {
                destinationFile << "\n";
            }
        }
    } while (sourceFile.good());

    if ((bytesWritten % 8) != 0)
    {
        destinationFile << "\n";
    }
    destinationFile << "};\n";
    destinationFile << "const size_t " << variableName << "_size = sizeof(" << variableName << ");\n\n";

    sourceFile.close();
    destinationFile.close();

    return EXIT_SUCCESS;
}
