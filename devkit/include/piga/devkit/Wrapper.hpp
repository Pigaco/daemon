#pragma once 

extern "C" 
{
    /**
     * Initializes a plugin. 
     */
    static void* init();
    /**
     * Deinitializes a plugin. 
     * 
     * The data argument is the same data as after the init() function. 
     */
    static void deinit(void *data);
}
