#include <noboost/pool/pool.hpp>
#include <chrono>
#include <iostream>

const int MAXLENGTH = 10000;

int main()
{
    boost::pool<> p(sizeof(int));
    int* vec1[MAXLENGTH];
    int* vec2[MAXLENGTH];

    auto clock_begin = std::chrono::steady_clock::now();
    for (int i = 0; i < MAXLENGTH; ++i)
    {
        vec1[i] = static_cast<int*>(p.malloc());
    }
    for (int i = 0; i < MAXLENGTH; ++i)
    {
        p.free(vec1[i]);
        vec1[i] = NULL;
    }

    auto clock_end = std::chrono::steady_clock::now();
    std::cout << "程序运行了 " << (clock_end - clock_begin).count() << " 个系统时钟" << std::endl;

    clock_begin = std::chrono::steady_clock::now();
    for (int i = 0; i < MAXLENGTH; ++i)
    {
        vec2[i] = new int;
    }
    for (int i = 0; i < MAXLENGTH; ++i)
    {
        delete vec2[i];
        vec2[i] = NULL;
    }

    clock_end = std::chrono::steady_clock::now();
    std::cout << "程序运行了 " << (clock_end - clock_begin).count() << " 个系统时钟" << std::endl;

    return 0;
}