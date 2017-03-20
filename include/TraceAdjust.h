#ifndef TRACEADJUST_H
#define TRACEADJUST_H

/*
 * MIT License
 *
 * Copyright (c) 2017 Norman Dunbar
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <chrono>

// iostream
using std::cerr;
using std::cout;
using std::endl;

// fstream
using std::ifstream;
using std::getline;

// string
using std::string;
using std::stoull;  // 64 bit required for tim= values.
using std::stof;    // To extract fractions of seconds from timestamps.
using std::to_string;

// exception
using std::exception;

// chrono
using std::chrono::system_clock;
using std::chrono::duration;



// Error codes.
const int ERR_NOARGS=1;
const int ERR_NOFILE=2;
const int ERR_BADFILE=3;
const int ERR_BADTIM=4;



// Function prototypes.
string asString(const system_clock::time_point &tp);
system_clock::time_point makeTimePoint(int year, int month, int day, int hour, int minute, int second=0);



#endif // TRACEADJUST_H
