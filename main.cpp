#include <cstdlib>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <memory>
#include <array>

namespace fs = std::filesystem;

/**
 * Permite imprimir simultáneamente en la consola estándar (`std::cout`) y en un archivo.
 * 
 * Constructor:
 *   TeeStream(std::ostream& c, const std::string& filename)
 *     - c: flujo de salida de consola (por lo general, std::cout).
 *     - filename: nombre del archivo donde también se escribirá la salida.
 * 
 * Métodos:
 *   operator<<: Sobrecarga del operador << para enviar datos tanto al flujo de consola como al archivo.
 *     - value: cualquier dato imprimible por ostream.
 *     - manip: manipuladores de flujo como std::endl.
 */
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

/**
 * Ejecuta un comando del sistema, redirige su salida (stdout y stderr) a un archivo temporal
 * y luego imprime su contenido a través de un objeto TeeStream. Al utilizar Docker, se debio
 * cambiar la forma en la que se guardaba la salida al archivo, por lo que el archivo queda vacio.
 * 
 * Parámetros:
 *   - cmd: comando del sistema a ejecutar.
 *   - out: objeto TeeStream para imprimir salida simultáneamente en consola y archivo.
 * 
 * Retorno:
 *   - true si el comando se ejecutó correctamente (código de retorno 0).
 *   - false si hubo un error al ejecutar el comando.
 */
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

/**
 * Ejecuta una batería de pruebas automatizadas de ordenamiento externo.
 * Para diferentes tamaños de entrada, se realizan pruebas con los algoritmos MergeSort y QuickSort,
 * verificando la validez del resultado con un programa `check`, y registrando toda la salida
 * en el archivo `experimentacion.txt`.
 * 
 * Proceso:
 *   - Por cada tamaño de entrada (200MB, 400MB, ..., 800MB):
 *       - Se generan datos de prueba con `generate`.
 *       - Se ordenan con `MergeSort`.
 *       - Se valida el resultado con `check`.
 *       - Se repite el proceso usando `QuickSort`.
 * 
 * Parámetros:
 *   - Ninguno (rutas y parámetros están codificados en el cuerpo).
 * 
 * Retorno:
 *   - 0 si la simulación se ejecuta correctamente.
 */
int main() {
    const size_t MB = 1024 * 1024;
    const size_t A = 96;
    const std::string input_file = "/tmp/input.bin";
    const std::string output_file = "/tmp/output.bin";

    TeeStream out(std::cout, "experimentacion.txt");

    for (int m = 4; m <= 16; m += 4) {
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
