#include "function_manager.hpp"
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

const char FILENAME[] = "resources/hash.o";
const size_t DESIRED_FUNCTION_OFFSET = 0x40;
const size_t SALT_OFFSET = 0x12;
const std::string get_salt_command = ":salt";
const std::string change_salt_command = ":chsalt";
const std::string quit_command = ":q";
const char escape_char = '\\';

void print_number(std::string const &message, unsigned long long number)
{
    std::cout << message << std::uppercase << std::hex << number << std::endl;
}

void process(char const *filename, size_t function_offset, size_t change_offset)
{
    function_manager<unsigned long long(char const *)> manager{filename,
                                                               function_offset};
    if (!manager.built) {
        std::cerr << strerror(errno) << std::endl;
        return;
    }
    unsigned long long current_salt = 0xAFBFCFDFEF9F8F7F;
    while (true) {
        std::string input;
        getline(std::cin, input);
        if (!std::cin) {
            break;
        }
        if (input.size() >= 1 && input.front() == escape_char) {
            try {
                print_number("Hash result: 0x", manager.apply(input.c_str() + 1));
            } catch (std::runtime_error const& e) {
                std::cerr << e.what() << std::endl;
            }
        } else if (input.size() >= get_salt_command.size() &&
                   input.substr(0, get_salt_command.size()) ==
                       get_salt_command) {
            print_number("Current salt: 0x", current_salt);
        } else if (input.size() >= change_salt_command.size() &&
                   input.substr(0, change_salt_command.size()) ==
                       change_salt_command) {
            std::istringstream stream(input);
            std::string number;
            stream >> number >> number;
            try {
                current_salt = std::stoull(number);
                manager.change_bytes(change_offset, current_salt);
                print_number("Salt changed successfully: 0x", current_salt);
            } catch (std::logic_error const &) {
                std::cout << "Please, provide decimal number in range [0; 2^64)"
                          << std::endl;
            }
        } else if (input == quit_command) {
            break;
        } else {
            try {
                print_number("Hash result: 0x", manager.apply(input.c_str()));
            } catch (std::runtime_error const& e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }
}

const std::string message =
    R"BLOCK(
JITHash project for ITMO OS course.
This is a simple hasher of strings that you type.
Usage:
)BLOCK" +
    quit_command + " -- quits the program. \n" + 
    get_salt_command + " -- prints current salt.  \n" + 
    change_salt_command + " [number] -- changes current salt to given number. Number must be in range [0; 2^64) \n" +
    "random_string -- calculates hash of random_string.  \n" + 
    escape_char + "[string] -- escaped string, calculates hash of string. \n";

int main(int argc, char *argv[])
{
    std::cout << message << std::endl;
    process(FILENAME, DESIRED_FUNCTION_OFFSET, SALT_OFFSET);
}
