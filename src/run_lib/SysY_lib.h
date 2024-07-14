#ifndef __SYLIB_H_
#define __SYLIB_H_

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
/* Input & output functions */
int getint(), getch(), getarray(int a[]);
float getfloat();
int getfarray(float a[]);

void putint(int a), putch(int a), putarray(int n, int a[]);
void putfloat(float a);
void putfarray(int n, float a[]);

void putf(char a[], ...);

/* Timing function implementation */
struct timeval _sysy_start, _sysy_end;
void starttime(int lineno);
void stoptime(int lineno);
int _sysy_l1[1024], _sysy_l2[1024];
int _sysy_h[1024], _sysy_m[1024], _sysy_s[1024], _sysy_us[1024];
int _sysy_idx;
__attribute((constructor)) void before_main();
__attribute((destructor)) void after_main();

#endif
