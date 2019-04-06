unsigned long long hash(char const* str) {
    unsigned long long result = 0;
    unsigned long long salt = 0xAFBFCFDFEF9F8F7FLL;
    for (char const *c = str; *c != 0; ++c) {
        result *= 128356133;
        result += (*c) + salt;
    }
    return result;
}
