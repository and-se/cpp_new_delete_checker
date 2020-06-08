/**
 * == EN ==
 * CHECKS new AND delete IN YOUR (!) C++ CODE FOR LEAKS AND WRONG USAGE.
 * 
 * USAGE
 * 1) Place this file in one folder with your 'main.cpp' file
 * 2) In your main.cpp write before all(!) other includes
 * #include "new_delete_checker.hpp"
 * 3) Run your program.
 * During the process, you will receive warnings about incorrect usage of new and delete.
 * Before the program ends, all direct (!) memory leaks will be printed.
 * 
 * == RU ==
 * Контролирует работу new и delete в Вашем (!) коде на предмет утечек памяти и некорректного использования.
 * 
 * Как подключить:
 * 1) Положить этот файл рядом с main.cpp
 * 2) В main.cpp написать до (!) всех остальных инклудов:
 * #include "new_delete_checker.hpp"
 * 3) Запустить программу на выполнение.
 * В процессе работы будут выдаваться предпруждения о некорректном использовании new и delete.
 * Перед окончанием работы программы будут выведены все прямые (!) утечки памяти.
 */

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>

namespace new_delete_checker_internal
{
    struct MemoryOperationResult
    {
        bool success;
        std::string error;
        
        MemoryOperationResult(std::string error)
        {
            this->error = error;
            this->success = error == "";
        }
    };    
    
    /// Allocated memory info storage
    class MemoryInfoManager
    {
        struct MemoryChunkInfoItem
        {
            void* ptr;
            std::size_t size;
            bool is_array;
            const char* file;
            int line;
            MemoryChunkInfoItem* next;
        };
        
        
        // manual list of info-items. We can't use STL, because it uses
        // 'new' and 'delete' in containers during insertion --> 
        // we go into infinite recursion
        MemoryChunkInfoItem* list_root = nullptr;
        
        public:
        ~MemoryInfoManager()
        {            
            // clear memory and show leaks
            auto cur = this->list_root;
            while(cur)
            {
                auto next = cur->next;
                print_leak(cur);
                std::free(cur);
                cur = next;
            }            
            
            this->list_root = nullptr;
        }
        
        /**
         * add record about allocated memory
         * @ptr - pointer to memory
         * @size - size in bytes
         * @is_array - is allocated using new[] (otherwise using new)
         * @file - file, where it's allocated
         * @line - line in file
         * */
        void add(void* ptr, std::size_t size, bool is_array, const char* file, int line)
        {
            // use malloc instead of new. Otherwise we will go into infinite recursion
            auto item = (MemoryChunkInfoItem*)std::malloc(sizeof(MemoryChunkInfoItem));
            item->ptr = ptr;
            item->size = size;
            item->is_array = is_array;
            item->file = file;
            item->line = line;
            
            item->next = this->list_root ? this->list_root : nullptr;
            this->list_root = item;            
        }
        
        /**
         * remove record about allocated memory
         * @ptr - pointer to memory
         * @ is_array - is deleted using delete[] (otherwise using delete)
         * @returns information with success or error message
         * */
        MemoryOperationResult remove(void* ptr, bool is_array)
        {
            auto removed = this->extract_info(ptr);
            
            std::stringstream error;
            
            if (!removed)
            {
                error << "Delete" << (is_array ? "[]" : "") << " of unknown pointer " << ptr;                
            }
            else if(removed->is_array && !is_array)
            {
                error << ptr << " (allocated at " 
                    << removed->file << ":" << removed->line 
                    << ") is ARRAY ---> use 'delete[]' instead of 'delete'";
            }
            else if (!removed->is_array && is_array)
            {
                error << ptr << " (allocated at "
                    << removed->file << ":" << removed->line
                    << ") as NOT ARRAY ---> use 'delete' instead of 'delete[]'";
            }
            
            std::free(removed);            
            return MemoryOperationResult(error.str());
        }
        
        private:
        /// cuts item from list
        MemoryChunkInfoItem* extract_info(void* ptr)
        {
            if (this->list_root)
            {
                MemoryChunkInfoItem* prev = nullptr;
                auto cur = this->list_root;
                
                while(cur)
                {
                    if (cur->ptr == ptr)
                    {
                        if (prev)
                        {
                            prev->next = cur->next;
                        }
                        else
                        {
                            this->list_root = cur->next;
                        }
                  
                        return cur;
                    }
                    
                    prev = cur;
                    cur = cur->next;
                }
            }
                    
            return nullptr;
        }
        
        void print_leak(MemoryChunkInfoItem* leak)
        {
            std::cerr << "LEAK " << leak->size << " bytes at " << leak->ptr
                << " (allocated at " << leak->file << ":" << leak->line << ")\n";
        }
    };
    
    /// Global memory map. Created on program start, deletes at normal program end.
    MemoryInfoManager memory_map;
    
    
    /// controlled memory allocation
    void* allocate_memory(std::size_t count, bool is_array, const char* file, int line, bool throw_on_fail=true)
    {        
        void* ptr = std::malloc(count);        
        if (!ptr && throw_on_fail)
        {            
            std::cerr << "Can't allocate " << count << " bytes!\n";
            throw std::bad_alloc();
        }
        
        memory_map.add(ptr, count, is_array, file, line);        
        
        return ptr;
    }
    
    /// controlled memory free
    MemoryOperationResult free_memory(void* ptr, bool is_array/*, const char* file, int line*/)
    {
        auto result = memory_map.remove(ptr, is_array);
        
        if (!result.success)
        {
            std::cerr << "WARN new_delete_checker: " << result.error << "\n";
        }
        
        std::free(ptr);
        return result;
    }
}

/// alias for internal namespace
namespace internal = new_delete_checker_internal;

////////// operator new and operator delete overrides ///////////

// new("file.cpp", 123) int --> operator new(4, "file.cpp", 123)
void* operator new(std::size_t count, const char* file, int line)
{    
    void* result = internal::allocate_memory(count, false, file, line);
    return result;
}

// new("file.cpp", 123) int[2] --> operator new[](8, "file.cpp", 123)
void* operator new[](std::size_t count, const char* file, int line)
{
    void* result = internal::allocate_memory(count, true, file, line);
    return result;
}

// for new in STL, which we can't override. We can't ignore it, because hook delete.
void* operator new(std::size_t count)
{
    void* result = internal::allocate_memory(count, false, "UNKNOWN", -1);
    return result;
}

// for new[] in STL, which we can't override. We can't ignore it, because hook delete[].
void* operator new[](std::size_t count)
{
    void* result = internal::allocate_memory(count, true, "UNKNOWN", -1);
    return result;
}

// delete ptr; Unforutnately, we can't provide file and line almost in all cases.
void operator delete(void* ptr/*, const char* file, int line*/)
{
    internal::free_memory(ptr, false/*, file, line*/);
}

// delete[] ptr; Unforutnately, we can't provide file and line almost in all cases.
void operator delete[](void* ptr/*, const char* file, int line*/)
{
    internal::free_memory(ptr, true/*, file, line*/);
}


// with preprocessor make all 'new' in your code 'new(cur_file, cur_line_in_file)'
// so, we know where memory is allocated.
// but it doesn't work for not our code, for example STL.
#define new new(__FILE__, __LINE__)

// Unfortunately, there's no way to add additional info into 'delete'.
// This's possible in few cases, but they are too specific.
//#define delete delete(__FILE__, __LINE__)


