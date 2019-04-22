#ifndef FUNCTION_MANAGER_HPP
#define FUNCTION_MANAGER_HPP
#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

template <typename>
struct function_manager;

template <typename R, typename... Args>
struct function_manager<R(Args...)> {
    void *memory_ptr;
    static const int PAGE_SIZE = 4096;
    bool built = false;

    struct fd_wrapper_t {
        int fd;
        fd_wrapper_t(char const *filename) { fd = open(filename, O_RDONLY); }
        fd_wrapper_t(fd_wrapper_t const &) = delete;
        ~fd_wrapper_t()
        {
            if (fd != -1 && close(fd) == -1) {
                    std::cerr << "Could not close file properly";
            }
        }
    };

    function_manager(char const *filename, size_t function_offset)
    {
        struct stat stat_info;
        if (stat(filename, &stat_info) == -1) {
            return;
        }
        size_t file_size = static_cast<size_t>(stat_info.st_size);
        if (file_size > PAGE_SIZE) {
            errno = EFBIG;
            return;
        }
        char buffer[PAGE_SIZE];
        fd_wrapper_t fd_wrapper(filename);
        if (fd_wrapper.fd == -1) {
            return;
        }
        if (read(fd_wrapper.fd, buffer, file_size) == -1) {
            return;
        }
        memory_ptr = mmap(nullptr, PAGE_SIZE, PROT_WRITE,
                          MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (memory_ptr == MAP_FAILED) {
            return;
        }
        memcpy(memory_ptr, buffer + function_offset,
               file_size - function_offset);
        if (mprotect(memory_ptr, PAGE_SIZE, PROT_NONE) == -1) {
            return;
        }
        built = true;
    }

    function_manager(function_manager const &) = delete;

    void change_bytes(size_t offset, unsigned long long value)
    {
        if (mprotect(memory_ptr, PAGE_SIZE, PROT_WRITE) == -1) {
            std::cerr << "Could not change source code." << std::endl;
            return;
        }
        unsigned long long *value_ptr = reinterpret_cast<unsigned long long *>(
            reinterpret_cast<size_t>(memory_ptr) + offset);
        *value_ptr = 0;
        *value_ptr = value;
        if (mprotect(memory_ptr, PAGE_SIZE, PROT_NONE) == -1) {
            std::cerr << "Could not protect source code." << std::endl;
        }
    }

    typedef R (*func_ptr)(Args...);

    R apply(Args &&... args) const
    {
        if (mprotect(memory_ptr, PAGE_SIZE, PROT_EXEC | PROT_READ) == -1) {
            throw std::runtime_error("Could not access source code for execution");
        }
        func_ptr function = reinterpret_cast<func_ptr>(memory_ptr);
        R result =  function(std::forward<Args>(args)...);
        if (mprotect(memory_ptr, PAGE_SIZE, PROT_NONE) == -1) {
            throw std::runtime_error("Could not protect source code");
        }
        return result;
    }

    ~function_manager() { 
        if (munmap(memory_ptr, PAGE_SIZE) == -1) {
            std::cerr << strerror(errno) << std::endl;
        }
     }
};

#endif
