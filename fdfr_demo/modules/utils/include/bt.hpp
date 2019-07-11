#pragma once

#include <execinfo.h>
#include <signal.h>
#include <iostream>

void print_backtrace() {
   void *buffer[100];
   int nptrs = backtrace(buffer, 100);
   std::cout << "backtrace: returned " << nptrs << " addresses" << std::endl;

   char **strings = backtrace_symbols(buffer, nptrs);
   if (strings == NULL) {
     return;
   }

   char prefix[32];
   for (int i = 0; i < nptrs; i++) {
     snprintf(prefix, sizeof(prefix), "#%02d ", i);
     std::cout << prefix << strings[i] << std::endl;
   }
}
