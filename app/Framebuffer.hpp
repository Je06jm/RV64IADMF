#ifndef APP_FRAMEBUFFER_HPP
#define APP_FRAMEBUFFER_HPP

#include "OpenGL.hpp"

#include <vector>

#include <Types.hpp>
#include <Memory.hpp>

inline Word framebuffer_width;
inline Word framebuffer_height;
inline Address framebuffer_address;

class MemoryFramebuffer : public MemoryRegion {
private:
    std::vector<Word> word_buffer;
    GLuint texture;
    GLuint vao;
    GLuint vbo;
    GLuint program;

    mutable std::mutex lock;

    const Word width, height;

    MemoryFramebuffer(Address base, Address size, Word width, Word height);

    inline Address CorrectAddress(Address address) const {
        auto x = (address >> 2) % width;
        auto y = (address >> 2) / width;

        return (height - y - 1) * width + x;
    }

public:
    ~MemoryFramebuffer();

    Long ReadLong(Address address) const override {
        return static_cast<Long>(ReadWord(address)) << 32 | static_cast<Long>(ReadWord(address + 4));
    }

    void WriteLong(Address address, Long value) override {
        WriteWord(address, static_cast<Word>(value >> 32));
        WriteWord(address + 4, static_cast<Word>(value));
    }

    Word ReadWord(Address address) const override {
        return word_buffer[CorrectAddress(address)];
    }

    void WriteWord(Address address, Word word) override {
        word_buffer[CorrectAddress(address)] = word;
    }

    void Lock() const override {
        lock.lock();
    }

    void Unlock() const override {
        lock.unlock();
    }

    void DrawBuffer() const;

    static std::shared_ptr<MemoryFramebuffer> Create(Address base, Word width, Word height);
};

#endif