#ifndef APP_FRAMEBUFFER_HPP
#define APP_FRAMEBUFFER_HPP

#include <Memory.hpp>
#include <OpenGL.hpp>

#include <vector>

inline size_t framebuffer_width;
inline size_t framebuffer_height;
inline uint32_t framebuffer_address;

class MemoryFramebuffer : public MemoryRegion {
private:
    std::vector<uint32_t> word_buffer;
    GLuint texture;
    GLuint vao;
    GLuint vbo;
    GLuint program;

    mutable std::mutex lock;

    const size_t width, height;

    MemoryFramebuffer(uint32_t base, uint32_t size, size_t width, size_t height);

    inline uint32_t CorrectAddress(uint32_t address) const {
        auto x = (address >> 2) % width;
        auto y = (address >> 2) / width;

        return (height - y - 1) * width + x;
    }

public:
    ~MemoryFramebuffer();

    uint32_t ReadWord(uint32_t address) const override {
        return word_buffer[CorrectAddress(address)];
    }

    void WriteWord(uint32_t address, uint32_t word) override {
        word_buffer[CorrectAddress(address)] = word;
    }

    void Lock() const override {
        lock.lock();
    }

    void Unlock() const override {
        lock.unlock();
    }

    void DrawBuffer() const;

    static std::shared_ptr<MemoryFramebuffer> Create(uint32_t base, size_t width, size_t height);
};

#endif