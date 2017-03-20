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

// This utility will read an Oracle trace file and try to convert
// all the "tim=" values into a delta=seconds plus a wallclock timestamp.
// Hopefully!

#include "TraceAdjust.h"

int main(int argc, char *argv[])
{
    // Sign on.
    cerr << "TraceAdjust v" << version << endl << endl;

    // We need a single parameter, the trace file name.
    if (argc < 2) {
        cerr << "TraceAdjust: No arguments supplied. Cannot continue." << endl;
        return ERR_NOARGS;
    }

    // Try to open the file.
    ifstream *traceFile = new ifstream(argv[1]);
    if (!traceFile->good()) {
        // Oops!
        cerr << "TraceAdjust: Cannot open trace file '" << argv[1] << "'." << endl;
        return ERR_NOFILE;
    }

    // File is open. Can we process it?
    uint32_t lineNumber = 0;
    string traceLine;

    // First read, should start with "Trace file"
    getline(*traceFile, traceLine);
    lineNumber++;
    if (traceLine.substr(0, 10) != "Trace file") {
        cerr << "TraceAdjust: This is not an Oracle trace file. "
             << "'Trace file' missing from line 1." << endl;
        return ERR_BADFILE;
    }

    // We need the first line.
    cout << traceLine << endl;

    // Don't have these in the main loop! :-)
    uint64_t previousTim = 0;       // The previous tim value.
    int64_t runningDelta = 0;      // Running total of all deltas since the last timestamp record.
    float fraction = 0.0;           // Fractions of seconds.

    // Base timestamp for the whole trace file run.
    // Will be updated on each and every timestamp row from the trace.
    system_clock::time_point baseTimestamp;


    // Loop through the remainder of the file.
    while (traceFile->good()) {
        getline(*traceFile, traceLine);
        lineNumber++;

        // Here we need to try and get a (new) base timestamp from something like:
        // *** 2017-03-13 09:23:21.767
        // 0.........1.........2......
        // 012345678901234567890123456
        if (traceLine.substr(0,4) == "*** " &&
            traceLine.at(8) == '-' &&
            traceLine.at(11) == '-' &&
            traceLine.at(14) == ' ' &&
            traceLine.at(17) == ':' &&
            traceLine.at(20) == ':' &&
            traceLine.at(23) == '.') {

            // We have a new timestamp line. Extract it.
            try {
                int year = stoull(traceLine.substr(4, 4), NULL, 10);
                int month = stoull(traceLine.substr(9, 2), NULL, 10);
                int day = stoull(traceLine.substr(12, 2), NULL, 10);
                int hour = stoull(traceLine.substr(15, 2), NULL, 10);
                int minute = stoull(traceLine.substr(18, 2), NULL, 10);
                int second = stoull(traceLine.substr(21, 2), NULL, 10);

                // Fractions of a second. Start at the dot, but needs a leading digit.
                fraction = stof("0" + traceLine.substr(23), NULL);

                // Convert to a time_point;
                baseTimestamp = makeTimePoint(year, month, day, hour, minute, second);

                cout << traceLine << endl;
                cout << "*** TraceAdjust v" << version
                     << ": Base Timestamp Adjusted to '" << asString(baseTimestamp) << '\'' << endl;

                // Reset the running delta. This is the deltas since the last timestamp correction.
                runningDelta = 0;

                // We need to accumulate fractions of seconds.
                runningDelta += (fraction * 1e6);

                // We don't need previousTim now.
                previousTim = 0;

                // Nothing more to do here.
                continue;
            } catch (exception &e) {
                cerr << "TraceAdjust: EXCEPTION: Cannot extract new base timestamp "
                     << "from text '" << traceLine << "'." << endl;
                return ERR_BADTIM;
            }
        }

        // Find a tim= value.
        // Delta's can be negative!!
        uint64_t thisTim = 0;
        int64_t deltaTim = 0;

        string::size_type timPos = traceLine.find("tim=");
        if (timPos == string::npos) {
            // Not found, write the line out to cout.
            cout << traceLine << endl;
            continue;
        }

        // Found a tim. Extract the value and subtract the previous tim value
        // from that to get a delta - which is in microseconds. And might be negative.
        bool timOk = true;
        try {
            // At least on Windows, the tim value is wider than 32 bits.
            // We need stoull here to get a 64 bit value.
            thisTim = stoull(traceLine.substr(timPos+4), NULL, 10);
        } catch (exception &e) {
            cerr << "TraceAdjust: Exception: [ " << e.what() << "]." << endl;
            timOk = false;
        }

        // Did it extract ok?
        if (!timOk) {
            cerr << "TraceAdjust: Failed to extract time value from ["
                 << traceLine << "]. At line: "
                 << lineNumber << endl;

            traceFile->close();
            return ERR_BADTIM;
        }

        // Convert to delta. Times are in microseconds from 10g.
        // If this is the first tim we have seen, the delta is zero.
        if (previousTim) {
            deltaTim = thisTim - previousTim;
        } else {
            deltaTim = 0;
        }
        previousTim = thisTim;
        runningDelta += deltaTim;

        // Convert delta to duration, still in microseconds.
        // Need to add the runningDelta otherwise we never get out of the current second!
        std::chrono::microseconds deltaDuration(runningDelta);

        // And as seconds.
        int64_t runningSeconds = runningDelta / 1e6;
        std::chrono::seconds deltaSeconds(runningSeconds);

        // Then get the fractional seconds.
        std::chrono::microseconds deltaFraction = deltaDuration - deltaSeconds;
        string fraction = std::to_string(deltaFraction.count() / 1000000.0).substr(1);

        // Adjust baseTimestamp with the runningDelta.
        // And convert to a Local Timestamp.
        string timeStamp = asString(baseTimestamp + deltaSeconds).substr(4);
        int tsLength = timeStamp.size();

        timeStamp = timeStamp.substr(tsLength - 4, 4) + " " +
                    timeStamp.substr(0, tsLength - 5);

        // Insert the fractional seconds count.
        timeStamp = timeStamp.insert(timeStamp.size(), fraction);

        // Write out the current line, plus the delta, converting
        // the existing time value into a seconds.fraction value.
        string seconds = to_string(thisTim);
        seconds.insert(seconds.size() - 6, ".");

        // And write the new style trace line.
        // delta= uSecs since previous tim value.
        // dslt = running delta in uSecs since last timestamp.
        // local = 'yyyy Mon dd hh24:mi:ss.ffffff'
        cout << traceLine.substr(0, timPos + 4)
             << seconds
             << ",delta=" << deltaTim
             << ",dslt=" << runningDelta
             << ",local='" << timeStamp << '\'' << endl;

    }   // End of while{} loop.
}


// Convert a time_point to a date and time string.
// Blatantly "stolen" from the "The C++ Standard Library: A Tutorial and Reference"
// by Nicolai M. Josuttis.
//
// Thank you very much.
string asString (const system_clock::time_point &tp)
{
    // Convert to system time from timepoint.
    std::time_t time = system_clock::to_time_t(tp);

    // Convert from system time to calendar time.
    string dateTime("Error in ctime.");

    try {
        dateTime = std::ctime(&time);
    } catch (...) {
        // Ctime() exceptions arrive here!
        // Not via std::exception.
        cout << "I caught something, but I've no idea what!" << endl;
        return dateTime;
    }

    // Lose the additional linefeed character at the end.
    dateTime.resize(dateTime.size() - 1);
    return dateTime;
}

// Convert a data and time to a time_point.
// Blatantly "stolen" from the "The C++ Standard Library: A Tutorial and Reference"
// by Nicolai M. Josuttis.
//
// Thank you very much.
system_clock::time_point makeTimePoint(int year, int month, int day, int hour, int minute, int second)
{
    struct std::tm t;
    t.tm_sec = second;
    t.tm_min = minute;
    t.tm_hour = hour;
    t.tm_mday = day;
    t.tm_mon = month - 1;
    t.tm_year = year - 1900;
    t.tm_isdst = -1;

    std::time_t tt = std::mktime(&t);
    if (tt == -1 ) {
        throw "No valid system time.";
    }

    return system_clock::from_time_t(tt);
}
