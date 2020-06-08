
#include "new_delete_checker.hpp"
#include <map>
#include <string>
#include <cstdlib>

int main()
{
    int *wrong_delete_scalar = new int;
    double *wrong_delete_array = new double[10];
    
    delete wrong_delete_array;
    delete[] wrong_delete_scalar;
    
    int* normal = new int[120];
    
    //int* unknown = normal-3;
    
    delete[] normal;
    // delete normal; //TODO recognize double deletion (now recognized as delete unknown)
    //delete unknown; // delete unknown stops program...
    
    void* not_controlled = std::malloc(34);
    std::cout <<"!!!"<<not_controlled<<"!!!\n";
    
    std::string* scalar_leak = new std::string;
    double* array_leak = new double[100];

       
    
    std::map<int, int> map_calls_new_delete_while_insert;
    map_calls_new_delete_while_insert[0]=10;
    
    
    return 0;
}
