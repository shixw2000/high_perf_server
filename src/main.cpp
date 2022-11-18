#include"testcase.h"


int main(int argc, char* argv[]) {
    int ret = 0;

    initLib();

    test_main(argc, argv);

    finishLib();
    return ret;
}

