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

bool run_command(const std::string& cmd, TeeStream& out) {
    std::string temp_file = "experimentacion.txt";
    std::string full_cmd = cmd + " > " + temp_file + " 2>&1"; // Redirect stdout and stderr

    out << "Ejecutando: " << cmd << std::endl;

    int ret = std::system(full_cmd.c_str());
    std::ifstream in(temp_file);
    std::string line;
    while (std::getline(in, line)) {
        out << line << '\n';
    }
    in.close();
    std::remove(temp_file.c_str());

    if (ret != 0) {
        out << "Error al ejecutar: " << cmd << std::endl;
        return false;
    }

    return true;
}


int main() {
    const size_t MB = 1024 * 1024;
    const size_t A = 96;
    const std::string input_file = "/tmp/input.bin";
    const std::string output_file = "/tmp/output.bin";

    TeeStream out(std::cout, "experimentacion.txt");

    for (int m = 4; m <= 60; m += 4) {
        size_t N_in_bytes = m * 50 * MB;
        size_t N = m * 50 * MB / sizeof(int64_t);
        out << "\n==== Tamaño: " << m * 50 << " MB ====\n";

        for (int trial = 1; trial <= 5; ++trial) {
            out << "\n-- Prueba #" << trial << " con N = " << N_in_bytes << " B --\n";

            out << ">> generate.exe " << N << "\n";
            if (!run_command("./generate " + input_file + " " + std::to_string(N_in_bytes), out))
                 continue;

            out << "-> MergeSort\n";
            if (!run_command("./MergeSort " + input_file + " " + output_file + " " + std::to_string(N_in_bytes) + " " + std::to_string(50*MB) + " " + std::to_string(A), out))
                continue;

            out << ">> check.exe " << output_file << "\n";
            if (!run_command("./check " + output_file, out))
                continue;

            out << ">> Borrar " << output_file << "\n";
            fs::remove(output_file);

            out << "-> QuickSort\n";
            if (!run_command("./QuickSort " + input_file + " " + output_file + " " + std::to_string(A) + " " + std::to_string(N_in_bytes), out))
                continue;

            out << ">> check.exe " << output_file << "\n";
            if (!run_command("./check " + output_file, out))
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
