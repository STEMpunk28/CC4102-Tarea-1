#include <cstdlib>
#include <iostream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// Ejecuta un comando y muestra errores si falla
bool run_command(const std::string& cmd) {
    std::cout << "Ejecutando: " << cmd << std::endl;
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Error al ejecutar: " << cmd << std::endl;
        return false;
    }
    return true;
}

int main() {
    const size_t MB = 1024 * 1024;
    const std::string input_file = "input.bin";
    const std::string output_file = "salidas.bin";

    for (int m = 4; m <= 60; m += 4) {
        size_t N = m * 50 * MB / sizeof(int64_t);
        std::cout << "\n==== Tamaño: " << m << "MB ====\n";

        for (int trial = 1; trial <= 5; ++trial) {
            std::cout << "\n-- Prueba #" << trial << " con N = " << N << " --\n";

            if (!run_command("generate.exe " + std::to_string(N)))
                continue;
            std::cout << ">> generate.exe " << N << "\n";

            std::cout << "-> MergeSort\n";
            // std::string merge_cmd = "MergeSort.exe " + input_file + " " + output_file;
            std::cout << ">> MergeSort.exe " << input_file << " " << output_file << "\n";

            // std::string check_merge = "check.exe " + output_file;
            std::cout << ">> check.exe " << output_file << "\n";

            std::cout << ">> Borrar " << output_file << "\n";
            // fs::remove(output_file);

            std::cout << "-> QuickSort\n";
            // std::string quick_cmd = "QuickSort.exe " + input_file + " " + output_file;
            std::cout << ">> QuickSort.exe " << input_file << " " << output_file << "\n";

            // std::string check_quick = "check.exe " + output_file;
            std::cout << ">> check.exe " << output_file << "\n";

            std::cout << ">> Borrar " << output_file << "\n";
            // fs::remove(output_file);

            std::cout << ">> Borrar " << input_file << "\n";
            fs::remove(input_file);
        }
    }

    std::cout << "\nSimulación completada.\n";
    return 0;
}
