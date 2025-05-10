#include <cstdlib>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <memory>
#include <array>

namespace fs = std::filesystem;

// Clase para imprimir simultáneamente en consola y archivo
class TeeStream {
    std::ostream& console;
    std::ofstream file;

public:
    TeeStream(std::ostream& c, const std::string& filename) : console(c), file(filename) {}

    template<typename T>
    TeeStream& operator<<(const T& value) {
        console << value;
        file << value;
        return *this;
    }

    // Para manipuladores como std::endl
    TeeStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
        console << manip;
        file << manip;
        return *this;
    }
};

// Ejecuta un comando y muestra errores si falla
bool run_command(const std::string& cmd, TeeStream& out) {
    out << "Ejecutando: " << cmd << std::endl;
    
    // Open process using popen to capture stdout
    std::array<char, 128> buffer;
    std::string result;
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) {
        out << "Error al ejecutar: " << cmd << std::endl;
        return false;
    }

    // Read the command output line by line
    while (fgets(buffer.data(), buffer.size(), fp) != nullptr) {
        result += buffer.data();
    }

    // Close the process
    int ret = pclose(fp);
    
    if (ret != 0) {
        out << "Error al ejecutar: " << cmd << std::endl;
        return false;
    }

    out << result << std::endl;

    return true;
}

int main() {
    const size_t MB = 1024 * 1024;
    const size_t A = 48;
    const std::string input_file = "input.bin";
    const std::string output_file = "output.bin";

    TeeStream out(std::cout, "experimentacion.txt");

    for (int m = 4; m <= 60; m += 4) {
        size_t N = m * 50 * MB / sizeof(int64_t);
        out << "\n==== Tamaño: " << m * 50 << " MB ====\n";

        for (int trial = 1; trial <= 5; ++trial) {
            out << "\n-- Prueba #" << trial << " con N = " << N << " B --\n";

            out << ">> generate.exe " << N << "\n";
            if (!run_command("generate.exe " + input_file + " " + std::to_string(N), out))
                 continue;

            out << "-> MergeSort\n";
            out << ">> MergeSort.exe " << input_file << " " << output_file << "\n";
            if (!run_command("MergeSort.exe " + input_file + " " + output_file + " " + std::to_string(A), out))
                continue;

            out << ">> check.exe " << output_file << "\n";
            if (!run_command("check.exe " + output_file, out))
                continue;

            out << ">> Borrar " << output_file << "\n";
            fs::remove(output_file);

            out << "-> QuickSort\n";
            out << ">> QuickSort.exe " << input_file << " " << output_file << "\n";
            if (!run_command("QuickSort.exe " + input_file + " " + output_file + " " + std::to_string(A) + " " + std::to_string(N), out))
                continue;

            out << ">> check.exe " << output_file << "\n";
            if (!run_command("check.exe " + output_file, out))
                continue;

            out << ">> Borrar " << output_file << "\n";
            fs::remove(output_file);

            out << ">> Borrar " << input_file << "\n";
            fs::remove(input_file);
        }
    }

    out << "\nSimulación completada.\n";
    return 0;
}
