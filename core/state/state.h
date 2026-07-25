// ============================================================
// STATE.H - Serialización de save states
// ============================================================
// StateWriter/StateReader: helpers binarios simples para volcar
// y restaurar el estado interno de cada componente del emulador.
// El formato es un stream plano little-endian (WASM/x86) sin
// alineación; cada componente escribe/lee sus campos en el mismo
// orden fijo.
// ============================================================

#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <vector>

class StateWriter
{
public:
    std::vector<uint8_t> data;

    void writeBytes(const void* src, size_t n) {
        const uint8_t* p = static_cast<const uint8_t*>(src);
        data.insert(data.end(), p, p + n);
    }

    template <typename T>
    void write(const T& value) {
        static_assert(std::is_trivially_copyable<T>::value,
                      "StateWriter::write requiere tipos triviales");
        writeBytes(&value, sizeof(T));
    }

    template <typename T, size_t N>
    void writeArray(const std::array<T, N>& arr) {
        static_assert(std::is_trivially_copyable<T>::value,
                      "StateWriter::writeArray requiere tipos triviales");
        writeBytes(arr.data(), N * sizeof(T));
    }

    // Vectores de tamaño variable (p.ej. RAM del cartucho):
    // se prefija el tamaño para validarlo al restaurar.
    void writeVector(const std::vector<uint8_t>& v) {
        write<uint32_t>(static_cast<uint32_t>(v.size()));
        writeBytes(v.data(), v.size());
    }
};

// StateReader NO lanza excepciones (Emscripten las compila con
// -fignore-exceptions por defecto y un throw abortaría el runtime).
// Ante datos truncados/corruptos marca failed() y devuelve ceros;
// el llamador debe comprobar failed() al terminar y descartar todo.
class StateReader
{
public:
    StateReader(const uint8_t* data, size_t size)
        : ptr(data), remaining(size) {}

    void readBytes(void* dst, size_t n) {
        if (failed_ || n > remaining) {
            failed_ = true;
            std::memset(dst, 0, n);
            return;
        }
        std::memcpy(dst, ptr, n);
        ptr += n;
        remaining -= n;
    }

    template <typename T>
    T read() {
        static_assert(std::is_trivially_copyable<T>::value,
                      "StateReader::read requiere tipos triviales");
        T value;
        readBytes(&value, sizeof(T));
        return value;
    }

    template <typename T, size_t N>
    void readArray(std::array<T, N>& arr) {
        static_assert(std::is_trivially_copyable<T>::value,
                      "StateReader::readArray requiere tipos triviales");
        readBytes(arr.data(), N * sizeof(T));
    }

    // El vector destino debe tener ya el tamaño correcto (lo fija la
    // ROM cargada); un tamaño distinto delata un state de otra ROM.
    void readVector(std::vector<uint8_t>& v) {
        uint32_t size = read<uint32_t>();
        if (failed_ || size != v.size()) {
            failed_ = true;
            return;
        }
        readBytes(v.data(), v.size());
    }

    bool   failed()    const { return failed_; }
    size_t bytesLeft() const { return remaining; }

private:
    const uint8_t* ptr;
    size_t remaining;
    bool failed_ = false;
};
