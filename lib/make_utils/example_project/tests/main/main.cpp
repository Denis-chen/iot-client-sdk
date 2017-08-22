#include <iostream>

extern "C"
{
int get_test1_c_val();
int get_test2_c_val();
}
std::string get_test1_cc_val();
std::string get_test2_cc_val();
std::string get_test1_cpp_val();
std::string get_test2_cpp_val();
std::string get_util_val();

using std::cout;
using std::endl;

int main(int argc, char *argv[])
{
    cout << "c1: " << get_test1_c_val() << endl;
    cout << "c2: " << get_test2_c_val() << endl;
    cout << "cc1: " << get_test1_cc_val() << endl;
    cout << "cc2: " << get_test2_cc_val() << endl;
    cout << "cpp1: " << get_test1_cpp_val() << endl;
    cout << "cpp2: " << get_test2_cpp_val() << endl;
    cout << "util: " << get_util_val() << endl;

    return 0;
}
