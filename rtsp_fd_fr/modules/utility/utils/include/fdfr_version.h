#include <string>
#include "logging.hpp"

class FDFRVersion {
public:
    static int get_major_version() { return 1; }
    static int get_minor_version() { return 0; }
    static int get_patch_version() { return 0; }
    static std::string get_full_version_string() { return "1.0.0"; }
    static void dump_version() { LOGD << get_full_version_string(); }
};
