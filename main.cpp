#include <cstdlib>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>

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
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        out << "Error al ejecutar: " << cmd << std::endl;
        return false;
    }
    return true;
}

int main() {
    const size_t MB = 1024 * 1024;
    const std::string input_file = "input.bin";
    const std::string output_file = "salidas.bin";

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
            // if (!run_command("MergeSort.exe " + input_file + " " + output_file, out))
            //     continue;

            out << ">> check.exe " << output_file << "\n";
            // if (!run_command("check.exe " + output_file, out))
            //     continue;

            out << ">> Borrar " << output_file << "\n";
            // fs::remove(output_file);

            out << "-> QuickSort\n";
            out << ">> QuickSort.exe " << input_file << " " << output_file << "\n";
            // if (!run_command("QuickSort.exe " + input_file + " " + output_file, out))
            //     continue;

            out << ">> check.exe " << output_file << "\n";
            // if (!run_command("check.exe " + output_file, out))
            //     continue;

            out << ">> Borrar " << output_file << "\n";
            // fs::remove(output_file);

            out << ">> Borrar " << input_file << "\n";
            fs::remove(input_file);
        }
    }

    out << "\nSimulación completada.\n";
    return 0;
}
