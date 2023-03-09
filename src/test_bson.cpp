#include "builder.h"
using namespace std;

void f(int* x){
    cout<<x<<endl;
}
int test_bson_main() {
    // quadis::BufBuilder buf_builder;
    // cout<< sizeof(buf_builder)<<endl;
    // printf("%p\n", buf_builder.buf());
    // char* p = buf_builder.buf();
    // buf_builder.grow(1024);
    // // buf_builder.appendNum(4);
    // printf("%p %p\n", buf_builder.buf(), p);
    // // printf("%d\n",*(int*)(p));
    // buf_builder.kill();
    int a = 99;
    int &b = a;
    f(&a);
    f(&b);
    return 1;
}