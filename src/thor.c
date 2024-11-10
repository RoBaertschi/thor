#include "common.h"

int main(int argc, char **argv) {
    log_register_file(stdout);
    log_debug("debug");
    log_info("info");
    log_warning("warning");
    log_error("error");
    log_info("about to fatal");
    log_fatal("fatal");
}
