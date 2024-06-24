#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <string.h>
#include <wait.h>

enum Commands {
    echo, ext, typ, pwd, cd, unknown=-1
};